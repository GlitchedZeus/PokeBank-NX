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
//
// An export record is found by the exact SOURCE_PATH its EDIT_MANIFEST.txt recorded (written by
// Integration/Common/VerifiedStagedExport from the session's exact source path); identity is never
// guessed from names or folders. The deletion unit is one whole export record, and only after an
// explicit Y in a visible decision (HomeQuit::decide). It is refused when the target is not a direct
// child of exports/genN, when the folder holds any file PokeBank NX did not write, or when the
// original emulator file is missing and this is the last remaining app backup of that save.
// Header-only POSIX (dirent/stat/unlink/rmdir) so it runs on host tests and libnx alike.

#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace PokeBank::UIModel::SourceBackups {

enum class Kind : unsigned char { OriginalSource, AppBackup, ExportedCopy, StagedWorkingCopy };

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
};

struct Listing {
    std::string exportsRoot;
    std::string sourcePath;
    bool sourceExists = false;
    std::vector<Entry> entries; // [0] is the original source, then records newest first, then staged
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
            return "Not deleted: the export record is no longer on the SD card.";
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
inline constexpr int kFirstGen = 1, kLastGen = 5;

} // namespace detail

// Every export record whose manifest names exactly `sourcePath`, newest first, plus the immutable
// source row and the informational staged-edits row.
[[nodiscard]] inline Listing list(const std::string& exportsRoot, const std::string& sourcePath) {
    Listing out;
    out.exportsRoot = exportsRoot;
    out.sourcePath = sourcePath;
    out.sourceExists = !sourcePath.empty() && detail::isFile(sourcePath);
    out.entries.push_back({Kind::OriginalSource, sourcePath, {}, {}, {}, out.sourceExists});

    struct Record { detail::Manifest m; std::string dir; };
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
    }
    std::stable_sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.m.timestamp != b.m.timestamp ? a.m.timestamp > b.m.timestamp : a.dir < b.dir;
    });
    for (const Record& r : records) {
        const std::string backup = r.dir + "/" + r.m.originalBackup;
        const std::string edited = r.dir + "/" + r.m.editedSave;
        out.entries.push_back({Kind::AppBackup, backup, r.dir, r.m.timestamp, r.m.gameId,
                               detail::isFile(backup)});
        out.entries.push_back({Kind::ExportedCopy, edited, r.dir, r.m.timestamp, r.m.gameId,
                               detail::isFile(edited)});
    }
    out.entries.push_back({Kind::StagedWorkingCopy, {}, {}, {}, {}, false});
    return out;
}

// Why entry `index` cannot be deleted right now (Refusal::None when it may be, after Y).
[[nodiscard]] inline Refusal canDelete(const Listing& listing, std::size_t index) {
    if (index >= listing.entries.size()) return Refusal::NotAnExportRecord;
    const Entry& e = listing.entries[index];
    if (e.kind == Kind::OriginalSource) return Refusal::OriginalSource;
    if (e.kind == Kind::StagedWorkingCopy) return Refusal::StagedWorkingCopy;
    // The record must be exactly <exportsRoot>/genN/<plain name>; never anything else.
    const std::string& dir = e.recordDir;
    bool underGen = false;
    for (int gen = detail::kFirstGen; gen <= detail::kLastGen && !underGen; ++gen) {
        const std::string prefix = listing.exportsRoot + "/gen" + std::to_string(gen) + "/";
        underGen = listing.exportsRoot.size() > 0 && dir.size() > prefix.size() &&
                   dir.compare(0, prefix.size(), prefix) == 0 &&
                   detail::plainName(std::string_view(dir).substr(prefix.size()));
    }
    if (!underGen || dir == listing.sourcePath) return Refusal::NotAnExportRecord;
    if (listing.sourcePath.compare(0, dir.size() + 1, dir + "/") == 0) return Refusal::NotAnExportRecord;
    if (!detail::isDir(dir)) return Refusal::Missing;
    const detail::Manifest m = detail::readManifest(dir + "/" + detail::kManifestName);
    if (!m.ok || m.sourcePath != listing.sourcePath) return Refusal::NotAnExportRecord;
    for (const std::string& name : detail::children(dir))
        if (name != detail::kManifestName && name != m.originalBackup && name != m.editedSave)
            return Refusal::UnexpectedFiles;
    if (!listing.sourceExists) {
        // Without the emulator's own file, never delete the last app backup of this save.
        int otherBackups = 0;
        for (const Entry& other : listing.entries)
            if (other.kind == Kind::AppBackup && other.exists && other.recordDir != dir) ++otherBackups;
        if (otherBackups == 0) return Refusal::OnlyRemainingCopy;
    }
    return Refusal::None;
}

// Deletes the one export record behind entry `index` (manifest, original_backup.*, edited.*, then
// the empty folder). Re-validates first; touches nothing outside that record. Call only after Y.
[[nodiscard]] inline Refusal deleteRecord(const Listing& listing, std::size_t index) {
    const Refusal refusal = canDelete(listing, index);
    if (refusal != Refusal::None) return refusal;
    const std::string dir = listing.entries[index].recordDir;
    const detail::Manifest m = detail::readManifest(dir + "/" + detail::kManifestName);
    for (const std::string& name : {m.originalBackup, m.editedSave}) {
        const std::string path = dir + "/" + name;
        if (detail::isFile(path)) ::unlink(path.c_str());
    }
    ::unlink((dir + "/" + detail::kManifestName).c_str());
    ::rmdir(dir.c_str());
    return detail::isDir(dir) ? Refusal::UnexpectedFiles : Refusal::None;
}

} // namespace PokeBank::UIModel::SourceBackups
