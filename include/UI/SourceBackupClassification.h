#pragma once

// Emulator-save backups for Product Home "+ -> Backups" (round 2, phase 7). Four things are kept
// explicitly apart, because only one of them may ever be deleted from here:
//
//   OriginalSource     the emulator's own save file (RetroArch / DraStic / melonDS). Immutable:
//                      PokeBank NX never writes, moves or deletes it.
//   AppBackup          original_backup.* inside a verified export record that PokeBank NX wrote
//                      under sdmc:/switch/PokeBank-NX/exports/genN/<record>/.
//   ExportedCopy       edited.* in the same record (the verified working-copy export).
//   StagedWorkingCopy  unsaved edits; they live only in the open editor (Review / Discard there).
//   ManualBackup       a byte-for-byte copy the user DELIBERATELY made from this screen (round 4,
//                      R2-EMUBACKUP/C07/S05). Stored under
//                      sdmc:/switch/PokeBank-NX/backups/emulator-saves/<gameId>/<timestamp>/ with a
//                      BACKUP_MANIFEST.txt naming the exact SOURCE_PATH. Created only by
//                      createBackup(); nothing here, and nothing on open, makes one automatically.
//
// An export record is found by the exact SOURCE_PATH its EDIT_MANIFEST.txt recorded (written by
// Integration/Common/VerifiedStagedExport from the session's exact source path); identity is never
// guessed from names or folders.
//
// Round 7 (7E G1): Gen V exports are written by Integration/Gen5/Gen5WorkingCopyExport, profile
// scoped: exports/gen5/account-<32 hex profile>/<gameId>_<timestamp>[-n]/ with EXPORT_MANIFEST.txt
// (GAME_ID, SOURCE_PATH, TIMESTAMP, ORIGINAL_BACKUP, EDITED_SAVE), original_backup.sav or .dsv and
// edited.sav. They are listed only for the CURRENT profile (Listing::profileIdentity): another
// profile's account folder is never read, opened or deleted from here. ".<label>.pending"
// reservations the writer left are not records and are never listed. The deletion unit is one whole export record, and only after an
// explicit Y in a visible decision (HomeQuit::decide). It is refused when the target is not a direct
// child of exports/genN, when the folder holds any file PokeBank NX did not write, or when the
// original emulator file is missing and this is the last remaining app backup of that save.
// Header-only POSIX (dirent/stat/unlink/rmdir) so it runs on host tests and libnx alike.

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <dirent.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "Source/ImmutableSourceSnapshot.h"

namespace PokeBank::UIModel::SourceBackups {

enum class Kind : unsigned char { OriginalSource, AppBackup, ExportedCopy, StagedWorkingCopy, ManualBackup };

struct Policy {
    bool readOnly;
    bool deletable;     // deletable as part of its export record (Y confirm)
    const char* label;
    const char* reason; // shown when the user asks for an action the row does not support
};

[[nodiscard]] constexpr Policy policyFor(Kind kind) noexcept {
    switch (kind) {
        case Kind::OriginalSource:
            return {true, false, "Original emulator save",
                    "Read only: PokeBank NX never changes, moves or deletes the emulator's own save."};
        case Kind::AppBackup:
            return {false, true, "App backup",
                    "PokeBank NX copy of the save taken before export. Y deletes this export record."};
        case Kind::ExportedCopy:
            return {false, true, "Exported copy",
                    "Verified working copy PokeBank NX exported. Y deletes this export record."};
        case Kind::StagedWorkingCopy:
            return {false, false, "Staged edits",
                    "Unsaved edits live only in the open editor; use Review or Discard there."};
        case Kind::ManualBackup:
            return {false, true, "Backup",
                    "A copy you made of the save. Y deletes this backup; the save itself is untouched."};
    }
    return {true, false, "", ""};
}

struct Entry {
    Kind kind = Kind::OriginalSource;
    std::string path;       // file path; empty for StagedWorkingCopy
    std::string recordDir;  // export record directory (AppBackup / ExportedCopy only)
    std::string timestamp;  // export TIMESTAMP (yyyymmdd_hhmmss), empty for the source
    std::string gameId;
    bool exists = false;
    std::string physicalStamp{};
    bool profileScoped = false;   // a Gen V account-<profile> record (EXPORT_MANIFEST.txt)
};

struct Listing {
    std::string exportsRoot;
    std::string manualRoot;   // <backups>/emulator-saves; empty = manual backups not listed
    std::string sourcePath;
    bool sourceExists = false;
    std::vector<Entry> entries; // [0] is the original source, then records newest first, then staged
    bool recoveryReadOnly=false; // history found without a current exact-source/profile assignment
    std::string profileIdentity; // 32 hex; empty = no Gen V profile records listed
};

enum class Refusal : unsigned char {
    None,
    OriginalSource,
    StagedWorkingCopy,
    NotAnExportRecord,
    UnexpectedFiles,
    OnlyRemainingCopy,
    Missing,
};

[[nodiscard]] constexpr const char* refusalReason(Refusal refusal) noexcept {
    switch (refusal) {
        case Refusal::None: return "";
        case Refusal::OriginalSource: return policyFor(Kind::OriginalSource).reason;
        case Refusal::StagedWorkingCopy: return policyFor(Kind::StagedWorkingCopy).reason;
        case Refusal::NotAnExportRecord:
            return "Not deleted: this is not a PokeBank NX export record.";
        case Refusal::UnexpectedFiles:
            return "Not deleted: this folder holds files PokeBank NX did not write.";
        case Refusal::OnlyRemainingCopy:
            return "Kept: the original emulator save was not found, so this backup is the only remaining copy.";
        case Refusal::Missing:
            return "Not deleted: the record is no longer on the SD card.";
    }
    return "";
}

namespace detail {

inline bool isFile(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}
inline bool isDir(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
// One path component the export writer could have produced: no separators, no dot names.
inline bool plainName(std::string_view name) {
    return !name.empty() && name != "." && name != ".." &&
           name.find('/') == std::string_view::npos && name.find('\\') == std::string_view::npos;
}
inline std::vector<std::string> children(const std::string& dir) {
    std::vector<std::string> names;
    if (DIR* d = ::opendir(dir.c_str())) {
        while (const dirent* e = ::readdir(d)) {
            const std::string name = e->d_name;
            if (name != "." && name != "..") names.push_back(name);
        }
        ::closedir(d);
    }
    std::sort(names.begin(), names.end());
    return names;
}
struct Manifest {
    std::string gameId, sourcePath, originalBackup, editedSave, timestamp;
    bool ok = false;
};
inline Manifest readManifest(const std::string& path) {
    Manifest m;
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return m;
    char line[1024];
    while (std::fgets(line, sizeof(line), f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        const auto eq = s.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = s.substr(0, eq), value = s.substr(eq + 1);
        if (key == "GAME_ID") m.gameId = value;
        else if (key == "SOURCE_PATH") m.sourcePath = value;
        else if (key == "ORIGINAL_BACKUP") m.originalBackup = value;
        else if (key == "EDITED_SAVE") m.editedSave = value;
        else if (key == "TIMESTAMP") m.timestamp = value;
    }
    std::fclose(f);
    m.ok = !m.sourcePath.empty() && plainName(m.originalBackup) && plainName(m.editedSave);
    return m;
}
inline constexpr const char* kManifestName = "EDIT_MANIFEST.txt";
// Gen5WorkingCopyExport's record manifest (same keys for the fields read here).
inline constexpr const char* kGen5ManifestName = "EXPORT_MANIFEST.txt";
// The profile component Gen5WorkingCopyExport writes: exactly 32 lowercase hex digits
// (Gen5WorkingCopyExport validProfile; all zero is the offline "Pokemon Saves" profile).
inline bool validProfile(std::string_view profile) {
    if (profile.size() != 32) return false;
    for (const char c : profile)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
// exports/gen5/account-<profile>, or "" for an invalid profile.
inline std::string gen5AccountDir(const std::string& exportsRoot, const std::string& profile) {
    return exportsRoot.empty() || !validProfile(profile) ? std::string{}
                                                         : exportsRoot + "/gen5/account-" + profile;
}
// A published Gen V record name (never a ".<label>.pending" reservation).
inline bool gen5RecordName(std::string_view name) { return plainName(name) && name.front() != '.'; }
inline std::string manifestNameFor(bool profileScoped) {
    return profileScoped ? kGen5ManifestName : kManifestName;
}
inline constexpr const char* kBackupManifestName = "BACKUP_MANIFEST.txt";

struct BackupManifest {
    std::string gameId, sourcePath, backupFile, timestamp;
    long long size = -1;
    bool ok = false;
};
inline BackupManifest readBackupManifest(const std::string& path) {
    BackupManifest m;
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return m;
    char line[1024];
    while (std::fgets(line, sizeof(line), f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        const auto eq = s.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = s.substr(0, eq), value = s.substr(eq + 1);
        if (key == "GAME_ID") m.gameId = value;
        else if (key == "SOURCE_PATH") m.sourcePath = value;
        else if (key == "BACKUP_FILE") m.backupFile = value;
        else if (key == "TIMESTAMP") m.timestamp = value;
        else if (key == "SIZE") m.size = std::strtoll(value.c_str(), nullptr, 10);
    }
    std::fclose(f);
    m.ok = !m.sourcePath.empty() && plainName(m.backupFile) && m.size >= 0;
    return m;
}
inline bool readAll(const std::string& path, std::vector<unsigned char>& out, std::size_t cap) {
    out.clear();
    std::FILE* f = std::fopen(path.c_str(), "rb");   // read-only: the source is never opened for write
    if (!f) return false;
    unsigned char chunk[16 * 1024];
    std::size_t got = 0;
    bool ok = true;
    while ((got = std::fread(chunk, 1, sizeof(chunk), f)) > 0) {
        out.insert(out.end(), chunk, chunk + got);
        if (out.size() > cap) { ok = false; break; }
    }
    if (std::ferror(f)) ok = false;
    std::fclose(f);
    return ok;
}
inline bool writeAll(const std::string& path, const std::vector<unsigned char>& bytes) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = bytes.empty() || std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    if (std::fflush(f) != 0) ok = false;
    if (ok) ok = ::fsync(::fileno(f)) == 0;
    if (std::fclose(f) != 0) ok = false;
    return ok;
}
inline bool makeDirs(const std::string& path) {
    if (path.empty()) return false;
    std::size_t pos = path.find('/', path.find(":/") == std::string::npos ? 1 : path.find(":/") + 2);
    while (true) {
        const std::string part = path.substr(0, pos);
        if (!part.empty() && !isDir(part) && ::mkdir(part.c_str(), 0777) != 0 && !isDir(part))
            return false;
        if (pos == std::string::npos) break;
        pos = path.find('/', pos + 1);
    }
    return isDir(path);
}
inline std::string extensionOf(const std::string& path) {
    const auto slash = path.find_last_of('/');
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return ".sav";
    std::string ext = path.substr(dot);
    for (char c : ext)
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '.')) return ".sav";
    return ext.size() <= 8 ? ext : ".sav";
}
inline constexpr int kFirstGen = 1, kLastGen = 5;

} // namespace detail

// Every export record whose manifest names exactly `sourcePath`, newest first, plus the immutable
// source row and the informational staged-edits row. With a valid `profileIdentity`, that profile's
// Gen V records (exports/gen5/account-<profile>/) are listed too; no other profile's.
[[nodiscard]] inline Listing list(const std::string& exportsRoot, const std::string& sourcePath,
                                  const std::string& manualRoot = {}, const std::string& profileIdentity = {}) {
    Listing out;
    out.exportsRoot = exportsRoot;
    out.manualRoot = manualRoot;
    out.sourcePath = sourcePath;
    out.profileIdentity = detail::validProfile(profileIdentity) ? profileIdentity : std::string{};
    out.sourceExists = !sourcePath.empty() && detail::isFile(sourcePath);
    out.entries.push_back({Kind::OriginalSource, sourcePath, {}, {}, {}, out.sourceExists});

    struct Record { detail::Manifest m; std::string dir; bool profileScoped = false; };
    std::vector<Record> records;
    if (!sourcePath.empty()) {
        for (int gen = detail::kFirstGen; gen <= detail::kLastGen; ++gen) {
            const std::string genRoot = exportsRoot + "/gen" + std::to_string(gen);
            for (const std::string& name : detail::children(genRoot)) {
                const std::string dir = genRoot + "/" + name;
                if (!detail::plainName(name) || !detail::isDir(dir)) continue;
                detail::Manifest m = detail::readManifest(dir + "/" + detail::kManifestName);
                if (m.ok && m.sourcePath == sourcePath) records.push_back({std::move(m), dir});
            }
        }
        // Gen V: this profile's account folder only.
        const std::string account = detail::gen5AccountDir(exportsRoot, out.profileIdentity);
        if (!account.empty() && detail::isDir(account))
            for (const std::string& name : detail::children(account)) {
                const std::string dir = account + "/" + name;
                if (!detail::gen5RecordName(name) || !detail::isDir(dir)) continue;
                detail::Manifest m = detail::readManifest(dir + "/" + detail::kGen5ManifestName);
                if (m.ok && !m.gameId.empty() && m.sourcePath == sourcePath)
                    records.push_back({std::move(m), dir, true});
            }
    }
    std::stable_sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.m.timestamp != b.m.timestamp ? a.m.timestamp > b.m.timestamp : a.dir < b.dir;
    });
    // Deliberate backups made from this screen: <manualRoot>/<gameId>/<timestamp>/BACKUP_MANIFEST.txt,
    // matched by the exact SOURCE_PATH recorded when the copy was made.
    struct ManualRecord { detail::BackupManifest m; std::string dir; };
    std::vector<ManualRecord> manual;
    if (!sourcePath.empty() && !manualRoot.empty()) {
        for (const std::string& game : detail::children(manualRoot)) {
            const std::string gameDir = manualRoot + "/" + game;
            if (!detail::plainName(game) || !detail::isDir(gameDir)) continue;
            for (const std::string& name : detail::children(gameDir)) {
                const std::string dir = gameDir + "/" + name;
                if (!detail::plainName(name) || !detail::isDir(dir)) continue;
                detail::BackupManifest m = detail::readBackupManifest(dir + "/" + detail::kBackupManifestName);
                if (m.ok && m.sourcePath == sourcePath) manual.push_back({std::move(m), dir});
            }
        }
    }
    std::stable_sort(manual.begin(), manual.end(), [](const ManualRecord& a, const ManualRecord& b) {
        return a.m.timestamp != b.m.timestamp ? a.m.timestamp > b.m.timestamp : a.dir < b.dir;
    });
    for (const ManualRecord& r : manual) {
        const std::string file = r.dir + "/" + r.m.backupFile;
        out.entries.push_back({Kind::ManualBackup, file, r.dir, r.m.timestamp, r.m.gameId,
                               detail::isFile(file)});
    }
    for (const Record& r : records) {
        const std::string backup = r.dir + "/" + r.m.originalBackup;
        const std::string edited = r.dir + "/" + r.m.editedSave;
        out.entries.push_back({Kind::AppBackup, backup, r.dir, r.m.timestamp, r.m.gameId,
                               detail::isFile(backup), {}, r.profileScoped});
        out.entries.push_back({Kind::ExportedCopy, edited, r.dir, r.m.timestamp, r.m.gameId,
                               detail::isFile(edited), {}, r.profileScoped});
    }
    // Manual backups and export records share one chronological history.
    std::stable_sort(out.entries.begin() + 1, out.entries.end(), [](const Entry& a, const Entry& b) {
        return a.timestamp != b.timestamp ? a.timestamp > b.timestamp : a.recordDir > b.recordDir;
    });
    for(auto& entry:out.entries)
        if(entry.exists)entry.physicalStamp=PokeVault::Source::ImmutableSnapshot::fileStamp(entry.path);
    out.entries.push_back({Kind::StagedWorkingCopy, {}, {}, {}, {}, false});
    return out;
}

// Why entry `index` cannot be deleted right now (Refusal::None when it may be, after Y).
[[nodiscard]] inline Refusal canDelete(const Listing& listing, std::size_t index) {
    if(listing.recoveryReadOnly)return Refusal::NotAnExportRecord;
    if (index >= listing.entries.size()) return Refusal::NotAnExportRecord;
    const Entry& e = listing.entries[index];
    if (e.kind == Kind::OriginalSource) return Refusal::OriginalSource;
    if (e.kind == Kind::StagedWorkingCopy) return Refusal::StagedWorkingCopy;
    const auto root=e.kind==Kind::ManualBackup?listing.manualRoot:listing.exportsRoot;
    if(!PokeVault::Source::ImmutableSnapshot::safePath(e.recordDir,root,true) ||
       !PokeVault::Source::ImmutableSnapshot::safePath(e.path,root) ||
       listing.sourcePath.starts_with(e.recordDir+"/"))return Refusal::NotAnExportRecord;
    const auto safeChildren=[&](const std::string& dir) {
        for(const auto& name:detail::children(dir))
            if(!PokeVault::Source::ImmutableSnapshot::safePath(dir+"/"+name,root))return false;
        return true;
    };
    if(!e.physicalStamp.empty() && e.physicalStamp!=PokeVault::Source::ImmutableSnapshot::fileStamp(e.path))
        return Refusal::Missing;
    const auto lastCopyGuard = [&](const std::string& dir) {
        if (detail::isFile(listing.sourcePath)) return Refusal::None;
        // Without the emulator's own file, never delete the last remaining copy of this save.
        int others = 0;
        for (const Entry& other : listing.entries)
            if ((other.kind == Kind::AppBackup || other.kind == Kind::ManualBackup) && detail::isFile(other.path) &&
                other.recordDir != dir)
                ++others;
        return others == 0 ? Refusal::OnlyRemainingCopy : Refusal::None;
    };
    if (e.kind == Kind::ManualBackup) {
        // Exactly <manualRoot>/<plain gameId>/<plain record>, holding only what createBackup wrote.
        const std::string& dir = e.recordDir;
        const std::string prefix = listing.manualRoot + "/";
        if (listing.manualRoot.empty() || dir.size() <= prefix.size() ||
            dir.compare(0, prefix.size(), prefix) != 0)
            return Refusal::NotAnExportRecord;
        const std::string_view rest = std::string_view(dir).substr(prefix.size());
        const auto slash = rest.find('/');
        if (slash == std::string_view::npos || !detail::plainName(rest.substr(0, slash)) ||
            !detail::plainName(rest.substr(slash + 1)))
            return Refusal::NotAnExportRecord;
        if (!detail::isDir(dir)) return Refusal::Missing;
        // Validate exact record scope BEFORE enumerating a protected game/root
        // folder. Every child must still be safe before reading the manifest.
        if(!safeChildren(dir))return Refusal::UnexpectedFiles;
        const detail::BackupManifest m = detail::readBackupManifest(dir + "/" + detail::kBackupManifestName);
        if (!m.ok || m.sourcePath != listing.sourcePath) return Refusal::NotAnExportRecord;
        for (const std::string& name : detail::children(dir))
            if (name != detail::kBackupManifestName && name != m.backupFile) return Refusal::UnexpectedFiles;
        return lastCopyGuard(dir);
    }
    // The record must be exactly <exportsRoot>/genN/<plain name>, or for a Gen V record exactly
    // <exportsRoot>/gen5/account-<this listing's profile>/<record>; never anything else.
    const std::string& dir = e.recordDir;
    bool underGen = false;
    if (e.profileScoped) {
        const std::string account = detail::gen5AccountDir(listing.exportsRoot, listing.profileIdentity);
        const std::string prefix = account + "/";
        underGen = !account.empty() && dir.size() > prefix.size() && dir.compare(0, prefix.size(), prefix) == 0 &&
                   detail::gen5RecordName(std::string_view(dir).substr(prefix.size()));
    }
    for (int gen = detail::kFirstGen; gen <= detail::kLastGen && !underGen && !e.profileScoped; ++gen) {
        const std::string prefix = listing.exportsRoot + "/gen" + std::to_string(gen) + "/";
        underGen = listing.exportsRoot.size() > 0 && dir.size() > prefix.size() &&
                   dir.compare(0, prefix.size(), prefix) == 0 &&
                   detail::plainName(std::string_view(dir).substr(prefix.size()));
    }
    if (!underGen || dir == listing.sourcePath) return Refusal::NotAnExportRecord;
    if (listing.sourcePath.compare(0, dir.size() + 1, dir + "/") == 0) return Refusal::NotAnExportRecord;
    if (!detail::isDir(dir)) return Refusal::Missing;
    if(!safeChildren(dir))return Refusal::UnexpectedFiles;
    const std::string manifestName = detail::manifestNameFor(e.profileScoped);
    const detail::Manifest m = detail::readManifest(dir + "/" + manifestName);
    if (!m.ok || m.sourcePath != listing.sourcePath) return Refusal::NotAnExportRecord;
    for (const std::string& name : detail::children(dir))
        if (name != manifestName && name != m.originalBackup && name != m.editedSave)
            return Refusal::UnexpectedFiles;
    return lastCopyGuard(dir);
}

// Deletes the one export record behind entry `index` (manifest, original_backup.*, edited.*, then
// the empty folder). Re-validates first; touches nothing outside that record. Call only after Y.
[[nodiscard]] inline Refusal deleteRecord(const Listing& listing, std::size_t index) {
    const Refusal refusal = canDelete(listing, index);
    if (refusal != Refusal::None) return refusal;
    const std::string dir = listing.entries[index].recordDir;
    if (listing.entries[index].kind == Kind::ManualBackup) {
        const detail::BackupManifest bm = detail::readBackupManifest(dir + "/" + detail::kBackupManifestName);
        const std::string file = dir + "/" + bm.backupFile;
        if (detail::isFile(file)) ::unlink(file.c_str());
        ::unlink((dir + "/" + detail::kBackupManifestName).c_str());
        ::rmdir(dir.c_str());
        return detail::isDir(dir) ? Refusal::UnexpectedFiles : Refusal::None;
    }
    const std::string manifestName = detail::manifestNameFor(listing.entries[index].profileScoped);
    const detail::Manifest m = detail::readManifest(dir + "/" + manifestName);
    for (const std::string& name : {m.originalBackup, m.editedSave}) {
        const std::string path = dir + "/" + name;
        if (detail::isFile(path)) ::unlink(path.c_str());
    }
    ::unlink((dir + "/" + manifestName).c_str());
    ::rmdir(dir.c_str());
    return detail::isDir(dir) ? Refusal::UnexpectedFiles : Refusal::None;
}

// Where deliberate emulator-save backups live: <backups root>/emulator-saves.
[[nodiscard]] inline std::string emulatorBackupsRoot(const std::string& backupsRoot) {
    return backupsRoot.empty() ? std::string{} : backupsRoot + "/emulator-saves";
}

struct CreateResult {
    bool ok = false;
    std::string recordDir;
    std::string backupPath;
    std::string error;
};

// Largest save this screen copies (a DS save is at most 0x80000 + footer; GBA 128 KiB).
inline constexpr std::size_t kMaxBackupBytes = 16u * 1024u * 1024u;

// The ONE way a backup is made: the user pressed A on "Create backup" in the Backups screen.
// Copies `sourcePath` byte for byte into <manualRoot>/<gameId>/<timestamp>[-n]/save<ext>, reads the
// copy back and compares, then writes BACKUP_MANIFEST.txt last (a record without a manifest is never
// listed). The source is opened read-only exactly once; nothing is ever written next to it.
[[nodiscard]] inline CreateResult createBackup(const std::string& manualRoot, const std::string& gameId,
                                               const std::string& sourcePath, const std::string& timestamp) {
    CreateResult out;
    if (manualRoot.empty() || !detail::plainName(gameId) || !detail::plainName(timestamp)) {
        out.error = "Backup refused: this game has no safe backup folder.";
        return out;
    }
    if (sourcePath.empty() || !detail::isFile(sourcePath)) {
        out.error = "Backup refused: the save file was not found on the SD card.";
        return out;
    }
    std::vector<unsigned char> bytes;
    if (!detail::readAll(sourcePath, bytes, kMaxBackupBytes) || bytes.empty()) {
        out.error = "Backup refused: the save file could not be read completely.";
        return out;
    }
    const std::string gameDir = manualRoot + "/" + gameId;
    if (!detail::makeDirs(gameDir)) {
        out.error = "Backup failed: could not create the backup folder (SD card full or read only?).";
        return out;
    }
    std::string name = timestamp;
    for (int n = 2; detail::isDir(gameDir + "/" + name) || detail::isFile(gameDir + "/" + name); ++n) {
        if (n > 999) { out.error = "Backup failed: too many backups with this timestamp."; return out; }
        name = timestamp + "-" + std::to_string(n);
    }
    const std::string dir = gameDir + "/" + name;
    if (::mkdir(dir.c_str(), 0777) != 0) {
        out.error = "Backup failed: could not create the backup folder.";
        return out;
    }
    const std::string fileName = "save" + detail::extensionOf(sourcePath);
    const std::string file = dir + "/" + fileName;
    std::vector<unsigned char> check;
    const bool copied = detail::writeAll(file, bytes) && detail::readAll(file, check, kMaxBackupBytes) &&
                        check == bytes;
    std::string manifest = "SOURCE_PATH=" + sourcePath + "\nGAME_ID=" + gameId + "\nBACKUP_FILE=" +
                           fileName + "\nTIMESTAMP=" + timestamp + "\nSIZE=" + std::to_string(bytes.size()) + "\n";
    const std::vector<unsigned char> manifestBytes(manifest.begin(), manifest.end());
    if (!copied || !detail::writeAll(dir + "/" + detail::kBackupManifestName, manifestBytes)) {
        ::unlink(file.c_str());
        ::unlink((dir + "/" + detail::kBackupManifestName).c_str());
        ::rmdir(dir.c_str());
        out.error = "Backup failed: the copy could not be written and verified (SD card full?).";
        return out;
    }
    out.ok = true;
    out.recordDir = dir;
    out.backupPath = file;
    return out;
}

} // namespace PokeBank::UIModel::SourceBackups
