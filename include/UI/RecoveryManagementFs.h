#pragma once

// File-system half of the recovery-management view (owner item #10). It only gathers facts for
// UI/RecoveryManagementModel.h (which decides what is Removable) and performs a confirmed Plan.
//
// What it reads (nothing else on the SD card is opened):
//   <backups>/emulator-saves/<game>/<record>/   deliberate emulator-save backups (BACKUP_MANIFEST.txt)
//   <exports>/gen1..gen5/<record>/              export records (EDIT_MANIFEST.txt)
//   <exports>/gen5/account-<profile>/<record>/  this profile's Gen V records (EXPORT_MANIFEST.txt);
//                                               another profile's folder is never read
//   <backups>/account-<profile>/...             this profile's Switch-save backup tree, for
//                                               DurableFile .previous/.failed/.tmp siblings only
//   <banks>/...                                 listed read-only (never removable)
// Links are never followed. A record or sibling is removed only after:
//   1. it was Removable when the user reviewed it and is in the confirmed Plan;
//   2. a fresh enumeration immediately before its removal still classifies it Removable, with the
//      same physical fingerprint (inode, size, times of every file) and the same members;
//   3. every member passes ImmutableSnapshot::safePath (each component lstat-ed, no link, a regular
//      file, no "."/".." part) under its root and, on the host, its canonical (realpath) form is
//      inside the canonical root. On the console the SD card is FAT, which has no links; the
//      per-component lstat check there is the canonical check.
// Header-only POSIX (dirent / stat / unlink / rmdir) so it runs on host tests and libnx alike.

#include "UI/RecoveryManagementModel.h"
#include "UI/SourceBackupClassification.h"
#include "Source/ImmutableSourceSnapshot.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace PokeBank::UIModel::RecoveryManagement::Fs {

struct Request {
    Roots roots;
    std::string profileIdentity;            // 32 lowercase hex, or empty (no profile folders read)
    std::vector<std::string> ownedSources;  // source saves a current game assignment owns
};

struct Listing {
    std::vector<Item> items;
    std::vector<Verdict> verdicts;
};

struct Outcome {
    std::string path;
    std::uint64_t size = 0;
    bool removed = false;
    std::string message;
};

namespace detail {

namespace SB = PokeBank::UIModel::SourceBackups;
namespace Snap = PokeVault::Source::ImmutableSnapshot;

inline bool lstatOf(const std::string& path, struct stat& st) { return ::lstat(path.c_str(), &st) == 0; }
inline bool isRealDir(const std::string& path) {
    struct stat st{};
    return lstatOf(path, st) && S_ISDIR(st.st_mode);
}
inline bool isRealFile(const std::string& path) {
    struct stat st{};
    return lstatOf(path, st) && S_ISREG(st.st_mode);
}
inline std::uint64_t fileSize(const std::string& path) {
    struct stat st{};
    return lstatOf(path, st) && S_ISREG(st.st_mode) ? static_cast<std::uint64_t>(st.st_size) : 0;
}
inline std::vector<std::string> children(const std::string& dir) { return SB::detail::children(dir); }

// The file is a regular file under `root` (no link anywhere on its path) and reads back in full:
// exactly st_size bytes, unchanged while reading, and not empty.
inline bool readsBackInFull(const std::string& path, const std::string& root) {
    if (!Snap::safePath(path, root)) return false;
    const std::string before = Snap::fileStamp(path);
    struct stat st{};
    if (before.empty() || !lstatOf(path, st) || st.st_size <= 0) return false;
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    unsigned char chunk[16 * 1024];
    std::uint64_t total = 0;
    std::size_t got = 0;
    while ((got = std::fread(chunk, 1, sizeof(chunk), f)) > 0) total += got;
    const bool ok = !std::ferror(f) && total == static_cast<std::uint64_t>(st.st_size);
    const bool closed = std::fclose(f) == 0;
    return ok && closed && Snap::fileStamp(path) == before;
}

inline std::string sortableTime(const struct stat& st) {
    char buffer[48];
#ifdef __linux__
    std::snprintf(buffer, sizeof(buffer), "%020lld.%09ld", static_cast<long long>(st.st_mtime),
                  static_cast<long>(st.st_mtim.tv_nsec));
#else
    std::snprintf(buffer, sizeof(buffer), "%020lld", static_cast<long long>(st.st_mtime));
#endif
    return buffer;
}

// Physical fingerprint of a record folder: every child's name and stamp, in order.
inline std::string folderFingerprint(const std::string& dir) {
    std::string out;
    for (const std::string& name : children(dir)) {
        const std::string path = dir + "/" + name;
        out += name + "=" + Snap::fileStamp(path) + "|" + Snap::directoryStamp(path) + ";";
    }
    return out;
}

// "<base>.previous.<N>" / "<base>.failed.<N>" / "<base>.tmp.<N>" exactly as DurableFile's
// uniqueSibling names them (N decimal, below 1000000).
inline bool durableSibling(const std::string& name, Kind& kind, std::string& base, std::string& generation) {
    const auto dot = name.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= name.size() || name.size() - dot - 1 > 6) return false;
    generation = name.substr(dot + 1);
    if (!std::all_of(generation.begin(), generation.end(), [](char c) { return c >= '0' && c <= '9'; }))
        return false;
    const std::string head = name.substr(0, dot);
    const auto marker = [&](const char* suffix, Kind k) {
        const std::size_t n = std::strlen(suffix);
        if (head.size() <= n || head.compare(head.size() - n, n, suffix) != 0) return false;
        base = head.substr(0, head.size() - n);
        kind = k;
        return true;
    };
    return marker(".previous", Kind::PreviousGeneration) || marker(".failed", Kind::FailedWrite) ||
           marker(".tmp", Kind::InterruptedWrite);
}
inline bool recoveryMarked(const std::string& name) {
    return name.find(".previous.") != std::string::npos || name.find(".failed.") != std::string::npos ||
           name.find(".incomplete.") != std::string::npos || name.find(".unreadable.") != std::string::npos ||
           name.find(".tmp.") != std::string::npos;
}

inline bool owned(const Request& request, const std::string& source) {
    return std::find(request.ownedSources.begin(), request.ownedSources.end(), source) !=
           request.ownedSources.end();
}

// DurableFile leftovers (and folder-level recovery copies) under `dir`, never following a link and
// never entering another profile's Gen V or Switch-save folder.
inline void walkSiblings(const Request& request, const std::string& root, const std::string& dir, int depth,
                         std::vector<Item>& out) {
    const std::string otherGen5 = request.roots.exports + "/gen5";
    for (const std::string& name : children(dir)) {
        const std::string path = dir + "/" + name;
        struct stat st{};
        if (!SB::detail::plainName(name) || !lstatOf(path, st) || S_ISLNK(st.st_mode)) continue;
        if (S_ISDIR(st.st_mode)) {
            if (dir == otherGen5 && name.rfind("account-", 0) == 0 &&
                name != "account-" + request.profileIdentity)
                continue;
            if (name.front() == '.' && name.size() > 8 && name.compare(name.size() - 8, 8, ".pending") == 0)
                continue;   // listed as PendingExport by the export scan
            if (recoveryMarked(name)) {
                Item item;
                item.kind = Kind::OtherRecovery;
                item.path = path;
                item.source = dir;
                item.sortKey = sortableTime(st);
                out.push_back(std::move(item));
                continue;   // never entered: a folder-level copy is kept whole
            }
            if (depth > 0) walkSiblings(request, root, path, depth - 1, out);
            continue;
        }
        if (!S_ISREG(st.st_mode)) continue;
        Kind kind{};
        std::string base, generation;
        if (durableSibling(name, kind, base, generation) && !base.empty()) {
            Item item;
            item.kind = kind;
            item.path = path;
            item.members = {path};
            item.size = static_cast<std::uint64_t>(st.st_size);
            item.memberSizes = {item.size};
            item.source = dir + "/" + base;
            item.sortKey = sortableTime(st) + "#" + std::string(7 - std::min<std::size_t>(7, generation.size()), '0') +
                           generation;
            item.primaryVerified = readsBackInFull(item.source, root);
            item.fingerprint = Snap::fileStamp(path);
            out.push_back(std::move(item));
        } else if (recoveryMarked(name)) {
            Item item;
            item.kind = Kind::OtherRecovery;
            item.path = path;
            item.size = static_cast<std::uint64_t>(st.st_size);
            item.source = dir;
            item.sortKey = sortableTime(st);
            out.push_back(std::move(item));
        }
    }
}

// One record folder: manifest + the files it names, verified by reading each back in full.
inline Item recordItem(const Request& request, Kind kind, const std::string& root, const std::string& dir,
                       const std::string& manifestName) {
    Item item;
    item.kind = kind;
    item.path = dir;
    const std::string manifest = dir + "/" + manifestName;
    std::vector<std::string> named;
    if (kind == Kind::ManualBackup) {
        const SB::detail::BackupManifest m = SB::detail::readBackupManifest(manifest);
        item.source = m.ok ? m.sourcePath : std::string{};
        item.sortKey = m.timestamp + "|" + leafName(dir);
        if (m.ok) named.push_back(m.backupFile);
        if (!m.ok) item.note = "its backup manifest could not be read.";
        else if (!readsBackInFull(dir + "/" + m.backupFile, root)) item.note = "its backup file could not be read back in full.";
        else if (fileSize(dir + "/" + m.backupFile) != static_cast<std::uint64_t>(m.size))
            item.note = "its backup file is not the size its manifest recorded.";
        else item.verified = true;
    } else {
        const SB::detail::Manifest m = SB::detail::readManifest(manifest);
        const bool gen5 = manifestName == SB::detail::kGen5ManifestName;
        const bool ok = m.ok && (!gen5 || !m.gameId.empty());
        item.source = ok ? m.sourcePath : std::string{};
        item.sortKey = m.timestamp + "|" + leafName(dir);
        if (ok) named = {m.originalBackup, m.editedSave};
        if (!ok) item.note = "its export manifest could not be read.";
        else if (!readsBackInFull(dir + "/" + m.originalBackup, root) || !readsBackInFull(dir + "/" + m.editedSave, root))
            item.note = "its files could not be read back in full.";
        else item.verified = true;
    }
    item.verified = item.verified && Snap::safePath(manifest, root);
    // Data files first, the manifest last: an interrupted removal stays listed (and protected).
    for (const std::string& name : named) {
        const std::string path = dir + "/" + name;
        if (isRealFile(path) && std::find(item.members.begin(), item.members.end(), path) == item.members.end())
            item.members.push_back(path);
    }
    if (isRealFile(manifest)) item.members.push_back(manifest);
    std::vector<std::string> expected = named;
    expected.push_back(manifestName);
    std::sort(expected.begin(), expected.end());
    expected.erase(std::unique(expected.begin(), expected.end()), expected.end());
    item.onlyAppFiles = children(dir) == expected;
    for (const std::string& member : item.members) {
        item.memberSizes.push_back(fileSize(member));
        item.size += item.memberSizes.back();
    }
    item.sourceInside = !item.source.empty() &&
        (item.source == dir || item.source.compare(0, dir.size() + 1, dir + "/") == 0);
    item.owned = !item.source.empty() && owned(request, item.source);
    item.fingerprint = folderFingerprint(dir);
    return item;
}

inline void bankFiles(const std::string& dir, int depth, std::vector<Item>& out) {
    for (const std::string& name : children(dir)) {
        const std::string path = dir + "/" + name;
        struct stat st{};
        if (!lstatOf(path, st) || S_ISLNK(st.st_mode)) continue;
        if (S_ISDIR(st.st_mode)) {
            if (depth > 0) bankFiles(path, depth - 1, out);
            continue;
        }
        if (!S_ISREG(st.st_mode)) continue;
        Item item;
        item.kind = Kind::BankFile;
        item.path = path;
        item.size = static_cast<std::uint64_t>(st.st_size);
        item.source = "Bank";
        item.sortKey = sortableTime(st);
        out.push_back(std::move(item));
    }
}

#ifndef __SWITCH__
inline std::string canonical(const std::string& path) {
    char buffer[PATH_MAX];
    return ::realpath(path.c_str(), buffer) ? std::string(buffer) : std::string{};
}
#endif

// The physical half of the path check for one file about to be removed.
inline bool physicallyInside(const std::string& path, const std::string& root) {
    if (!within(path, root) || !Snap::safePath(path, root)) return false;
#ifndef __SWITCH__
    const std::string real = canonical(path), realRoot = canonical(root);
    return !real.empty() && !realRoot.empty() && real.size() > realRoot.size() + 1 &&
           real.compare(0, realRoot.size() + 1, realRoot + "/") == 0;
#else
    return true;
#endif
}

inline const std::string* rootOf(const Request& request, const std::string& path) {
    if (!request.roots.backups.empty() && within(path, request.roots.backups)) return &request.roots.backups;
    if (!request.roots.exports.empty() && within(path, request.roots.exports)) return &request.roots.exports;
    return nullptr;
}

} // namespace detail

// Everything this view shows, classified by the pure model.
[[nodiscard]] inline Listing enumerate(const Request& request) {
    namespace SB = PokeBank::UIModel::SourceBackups;
    Listing out;
    const Roots& roots = request.roots;
    const bool profile = SB::detail::validProfile(request.profileIdentity);
    if (!roots.backups.empty() && lexicallySafe(roots.backups) && detail::isRealDir(roots.backups)) {
        const std::string manual = SB::emulatorBackupsRoot(roots.backups);
        if (detail::isRealDir(manual)) {
            for (const std::string& game : detail::children(manual)) {
                const std::string gameDir = manual + "/" + game;
                if (!SB::detail::plainName(game) || !detail::isRealDir(gameDir)) continue;
                for (const std::string& name : detail::children(gameDir)) {
                    const std::string dir = gameDir + "/" + name;
                    if (!SB::detail::plainName(name) || !detail::isRealDir(dir) ||
                        !detail::isRealFile(dir + "/" + SB::detail::kBackupManifestName))
                        continue;
                    out.items.push_back(detail::recordItem(request, Kind::ManualBackup, roots.backups, dir,
                                                           SB::detail::kBackupManifestName));
                }
            }
            detail::walkSiblings(request, roots.backups, manual, 3, out.items);
        }
        if (profile) {
            const std::string account = roots.backups + "/account-" + request.profileIdentity;
            if (detail::isRealDir(account)) detail::walkSiblings(request, roots.backups, account, 5, out.items);
        }
    }
    if (!roots.exports.empty() && lexicallySafe(roots.exports) && detail::isRealDir(roots.exports)) {
        for (int gen = SB::detail::kFirstGen; gen <= SB::detail::kLastGen; ++gen) {
            const std::string genRoot = roots.exports + "/gen" + std::to_string(gen);
            if (!detail::isRealDir(genRoot)) continue;
            for (const std::string& name : detail::children(genRoot)) {
                const std::string dir = genRoot + "/" + name;
                if (!SB::detail::plainName(name) || !detail::isRealDir(dir) ||
                    !detail::isRealFile(dir + "/" + SB::detail::kManifestName))
                    continue;
                out.items.push_back(detail::recordItem(request, Kind::ExportRecord, roots.exports, dir,
                                                       SB::detail::kManifestName));
            }
        }
        const std::string account = SB::detail::gen5AccountDir(roots.exports, request.profileIdentity);
        if (!account.empty() && detail::isRealDir(account)) {
            for (const std::string& name : detail::children(account)) {
                const std::string dir = account + "/" + name;
                if (!SB::detail::plainName(name) || !detail::isRealDir(dir)) continue;
                if (name.front() == '.') {
                    if (name.size() > 8 && name.compare(name.size() - 8, 8, ".pending") == 0) {
                        Item pending;
                        pending.kind = Kind::PendingExport;
                        pending.path = dir;
                        pending.source = account;
                        out.items.push_back(std::move(pending));
                    }
                    continue;
                }
                if (!detail::isRealFile(dir + "/" + SB::detail::kGen5ManifestName)) continue;
                out.items.push_back(detail::recordItem(request, Kind::ExportRecord, roots.exports, dir,
                                                       SB::detail::kGen5ManifestName));
            }
        }
        detail::walkSiblings(request, roots.exports, roots.exports, 3, out.items);
    }
    if (!roots.banks.empty() && lexicallySafe(roots.banks) && detail::isRealDir(roots.banks))
        detail::bankFiles(roots.banks, 2, out.items);
    out.verdicts = classify(out.items, roots);
    return out;
}

// Removes the planned items one by one. Each is re-checked against a fresh enumeration right
// before its removal and refused if anything about it changed. Returns one Outcome per item.
[[nodiscard]] inline std::vector<Outcome> removePlanned(const Request& request, const Listing& reviewed,
                                                        const Plan& plan) {
    std::vector<Outcome> outcomes;
    for (const std::size_t index : plan.items) {
        Outcome outcome;
        if (index >= reviewed.items.size() || index >= reviewed.verdicts.size()) {
            outcome.message = "Refused: not in the reviewed list.";
            outcomes.push_back(std::move(outcome));
            continue;
        }
        const Item& seen = reviewed.items[index];
        outcome.path = seen.path;
        outcome.size = seen.size;
        if (reviewed.verdicts[index].status != Status::Removable) {
            outcome.message = "Refused: it was protected when reviewed.";
            outcomes.push_back(std::move(outcome));
            continue;
        }
        const Listing now = enumerate(request);
        std::size_t match = now.items.size();
        for (std::size_t i = 0; i < now.items.size(); ++i)
            if (now.items[i].path == seen.path && now.items[i].kind == seen.kind) { match = i; break; }
        if (match == now.items.size()) {
            outcome.message = "Refused: it is no longer on the SD card.";
        } else if (now.verdicts[match].status != Status::Removable) {
            outcome.message = "Refused: now protected. " + now.verdicts[match].reason;
        } else if (now.items[match].fingerprint != seen.fingerprint || now.items[match].members != seen.members ||
                   now.items[match].memberSizes != seen.memberSizes ||
                   now.items[match].size != seen.size || now.items[match].source != seen.source) {
            outcome.message = "Refused: it changed since you reviewed it. Nothing was removed.";
        } else {
            const std::string* root = detail::rootOf(request, seen.path);
            bool safe = root != nullptr;
            for (const std::string& member : seen.members)
                safe = safe && detail::physicallyInside(member, *root);
            if (safe && isRecord(seen.kind))
                safe = PokeVault::Source::ImmutableSnapshot::safePath(seen.path, *root, true);
            if (!safe) {
                outcome.message = "Refused: a path check failed (outside PokeBank NX's folders or a link).";
            } else {
                bool ok = true;
                for (const std::string& member : seen.members) ok = ::unlink(member.c_str()) == 0 && ok;
                if (isRecord(seen.kind)) ok = ::rmdir(seen.path.c_str()) == 0 && ok;
                struct stat st{};
                outcome.removed = ok && ::lstat(seen.path.c_str(), &st) != 0;
                outcome.message = outcome.removed ? "Removed." : "Could not be removed completely.";
            }
        }
        outcomes.push_back(std::move(outcome));
    }
    return outcomes;
}

} // namespace PokeBank::UIModel::RecoveryManagement::Fs
