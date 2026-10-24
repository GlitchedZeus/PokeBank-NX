#pragma once

// Round 5 (#322, R2-EMUBACKUP / C07 / S05): the one Save Backups browser shared by installed
// Switch titles and Gen I-V emulator games. The full-screen PKSE picker (BackupSelectionScreen)
// draws these rows; this header decides what each row says, what A does on it, whether Y may
// delete it, and the order (actions first, then every copy newest first by its real timestamp).
//
// Row kinds:
//   BrowseCurrent  the current save (installed title or emulator file), opened read-only
//   CreateBackup   "Create backup now": the only way a backup is made here
//   SwitchBackup   a profile/exact-game PKSE workspace folder PokeBank NX made
//   LegacyBackup   a pre-A08 title-only PKSE folder: no AccountUid, so ownership is unknown.
//                  It opens through an isolated copy and is never deleted from here.
//   ManualBackup / AppBackup / ExportedCopy   the emulator copies from SourceBackupClassification
//
// A on a copy opens THAT copy in an isolated staged session: never the live original, and never
// in place (the copy stays byte-identical). Pure and host-tested (tests/test_backup_browser_model.cpp).

#include "UI/SourceBackupClassification.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::BackupBrowser {

enum class RowKind : unsigned char {
    BrowseCurrent,
    CreateBackup,
    SwitchBackup,
    LegacyBackup,
    ManualBackup,
    AppBackup,
    ExportedCopy,
};

enum class Action : unsigned char { Browse, Create, Open, Unavailable };

struct Row {
    RowKind kind = RowKind::BrowseCurrent;
    std::string stamp;      // yyyymmdd_hhmmss[-n] (folder name, manifest TIMESTAMP or folder mtime)
    std::string name;       // a named Switch backup's folder name; empty for timestamp-named ones
    std::string path;       // the copy: a PKSE folder (Switch) or one save file (emulator)
    std::string recordDir;  // emulator record folder (delete unit)
    int listing = -1;       // emulator: index into the screen's SourceBackups listings
    int entry = -1;         // emulator: index into that listing's entries
    bool exists = true;
    bool newest = false;    // the newest dated copy in the list
};

[[nodiscard]] constexpr bool isCopy(RowKind kind) noexcept {
    return kind != RowKind::BrowseCurrent && kind != RowKind::CreateBackup;
}

// "yyyymmdd_hhmmss" with an optional "-n" suffix (two copies made in the same second).
[[nodiscard]] inline bool isStamp(std::string_view s) noexcept {
    if (s.size() < 15) return false;
    for (std::size_t i = 0; i < 15; ++i) {
        const bool digit = std::isdigit(static_cast<unsigned char>(s[i])) != 0;
        if (i == 8 ? s[i] != '_' : !digit) return false;
    }
    if (s.size() == 15) return true;
    if (s[15] != '-' || s.size() == 16) return false;
    for (std::size_t i = 16; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

// "2026-10-23 14:02:11" (and " #2" for a same-second suffix); anything else is shown verbatim.
[[nodiscard]] inline std::string formatStamp(std::string_view s) {
    if (!isStamp(s)) return std::string(s);
    std::string out;
    out.reserve(24);
    out.append(s.substr(0, 4)).append("-").append(s.substr(4, 2)).append("-").append(s.substr(6, 2));
    out.append(" ").append(s.substr(9, 2)).append(":").append(s.substr(11, 2)).append(":").append(s.substr(13, 2));
    if (s.size() > 15) out.append(" #").append(s.substr(16));
    return out;
}

[[nodiscard]] constexpr const char* badge(RowKind kind) noexcept {
    switch (kind) {
        case RowKind::BrowseCurrent: return "CURRENT";
        case RowKind::CreateBackup:  return "NEW COPY";
        case RowKind::SwitchBackup:  return "BACKUP";
        case RowKind::LegacyBackup:  return "LEGACY";
        case RowKind::ManualBackup:  return "BACKUP";
        case RowKind::AppBackup:     return "PRE-EXPORT";
        case RowKind::ExportedCopy:  return "EXPORT";
    }
    return "";
}

// What the copy is, in words (the secondary line names it, the primary line is its date).
[[nodiscard]] constexpr const char* typeName(RowKind kind) noexcept {
    switch (kind) {
        case RowKind::BrowseCurrent: return "Current save";
        case RowKind::CreateBackup:  return "Create backup now";
        case RowKind::SwitchBackup:  return "Backup";
        case RowKind::LegacyBackup:  return "Legacy PKSE backup";
        case RowKind::ManualBackup:  return "Backup";
        case RowKind::AppBackup:     return "Copy saved before an export";
        case RowKind::ExportedCopy:  return "Exported copy";
    }
    return "";
}

[[nodiscard]] constexpr Action actionFor(const Row& row) noexcept {
    switch (row.kind) {
        case RowKind::BrowseCurrent: return row.exists ? Action::Browse : Action::Unavailable;
        case RowKind::CreateBackup:  return row.exists ? Action::Create : Action::Unavailable;
        default:                     return row.exists ? Action::Open : Action::Unavailable;
    }
}

[[nodiscard]] constexpr const char* actionLabel(Action action) noexcept {
    switch (action) {
        case Action::Browse:      return "Browse";
        case Action::Create:      return "Create";
        case Action::Open:        return "Open";
        case Action::Unavailable: return "Details";
    }
    return "Details";
}

// Y may offer to delete only app-owned copies. The emulator's own save, the installed title, the
// action rows and every legacy folder are never offered (the emulator delete also re-checks the
// record on disk and the last-copy guard in SourceBackups::canDelete).
[[nodiscard]] constexpr bool deletable(RowKind kind) noexcept {
    return kind == RowKind::SwitchBackup || kind == RowKind::ManualBackup ||
           kind == RowKind::AppBackup || kind == RowKind::ExportedCopy;
}

// Primary line: the action, or the copy's date and time (named backups keep their name).
[[nodiscard]] inline std::string primaryLabel(const Row& row) {
    switch (row.kind) {
        case RowKind::BrowseCurrent: return "Current save";
        case RowKind::CreateBackup:  return "Create backup now";
        default: break;
    }
    if (!row.name.empty()) return row.name;
    return row.stamp.empty() ? std::string("Date unknown") : formatStamp(row.stamp);
}

// Secondary line. `saveName` is "DraStic save" / "installed save"; it is never empty in practice.
[[nodiscard]] inline std::string detailLine(const Row& row, std::string_view saveName) {
    const std::string save(saveName.empty() ? std::string_view("save") : saveName);
    switch (row.kind) {
        case RowKind::BrowseCurrent:
            return row.exists ? "Opens the " + save + " read-only. Opening never makes a backup."
                              : "The " + save + " was not found.";
        case RowKind::CreateBackup:
            return "Copies the " + save + " into PokeBank NX backups. The save is not changed.";
        case RowKind::LegacyBackup:
            return "Legacy PKSE backup: owner unknown, kept read-only and never deleted here.";
        default: break;
    }
    std::string out = typeName(row.kind);
    if (!row.name.empty() && !row.stamp.empty()) out += " - " + formatStamp(row.stamp);
    if (!row.exists) return out + " - missing from the SD card";
    if (row.newest) out += " - newest";
    out += ". A opens a staged copy; this backup is not changed.";
    return out;
}

// Provenance banner shown in the editor opened from a copy.
[[nodiscard]] inline std::string openedBanner(const Row& row) {
    const std::string when = row.name.empty() ? formatStamp(row.stamp)
                                              : row.name + (row.stamp.empty() ? "" : " (" + formatStamp(row.stamp) + ")");
    if (row.kind == RowKind::LegacyBackup) return "Opened from legacy backup - " + when;
    if (row.kind == RowKind::ExportedCopy) return "Opened from exported copy - " + when;
    return "Opened from backup - " + when;
}

// Same-second ties: one fixed kind order, then path, so no two copies ever collapse or swap.
[[nodiscard]] constexpr int tieRank(RowKind kind) noexcept {
    switch (kind) {
        case RowKind::ManualBackup: return 0;
        case RowKind::SwitchBackup: return 1;
        case RowKind::ExportedCopy: return 2;
        case RowKind::AppBackup:    return 3;
        case RowKind::LegacyBackup: return 4;
        default:                    return 5;
    }
}

// Actions keep their place at the top; copies follow newest first by timestamp. Copies with no
// readable timestamp go last (alphabetical). The first dated copy is marked newest.
inline void order(std::vector<Row>& rows) {
    auto actionsEnd = std::stable_partition(rows.begin(), rows.end(),
                                            [](const Row& r) { return !isCopy(r.kind); });
    std::stable_sort(actionsEnd, rows.end(), [](const Row& a, const Row& b) {
        const bool ad = isStamp(a.stamp), bd = isStamp(b.stamp);
        if (ad != bd) return ad;
        if (ad) {
            const std::string_view as = std::string_view(a.stamp).substr(0, 15);
            const std::string_view bs = std::string_view(b.stamp).substr(0, 15);
            if (as != bs) return as > bs;
            if (a.stamp.size() != b.stamp.size() || a.stamp != b.stamp) {
                // "-n" suffixes: the later copy of the same second first.
                const int an = a.stamp.size() > 16 ? std::atoi(a.stamp.c_str() + 16) : 1;
                const int bn = b.stamp.size() > 16 ? std::atoi(b.stamp.c_str() + 16) : 1;
                if (an != bn) return an > bn;
            }
        } else if (a.stamp != b.stamp) {
            return a.stamp < b.stamp;
        }
        if (tieRank(a.kind) != tieRank(b.kind)) return tieRank(a.kind) < tieRank(b.kind);
        return a.path < b.path;
    });
    bool marked = false;
    for (auto it = actionsEnd; it != rows.end(); ++it) {
        it->newest = !marked && isStamp(it->stamp) && it->exists;
        if (it->newest) marked = true;
    }
}

// Emulator rows from the exact-source listings (SourceBackups::list): per save, its Current row
// and Create row, then every copy of every listed save merged newest first.
[[nodiscard]] inline std::vector<Row> emulatorRows(const std::vector<SourceBackups::Listing>& listings) {
    namespace SB = SourceBackups;
    std::vector<Row> rows;
    for (int li = 0; li < static_cast<int>(listings.size()); ++li) {
        const SB::Listing& listing = listings[static_cast<std::size_t>(li)];
        Row current;
        current.kind = RowKind::BrowseCurrent;
        current.path = listing.sourcePath;
        current.listing = li;
        current.entry = 0;
        current.exists = listing.sourceExists;
        rows.push_back(current);
        Row create = current;
        create.kind = RowKind::CreateBackup;
        create.entry = -1;
        rows.push_back(create);
    }
    for (int li = 0; li < static_cast<int>(listings.size()); ++li) {
        const auto& entries = listings[static_cast<std::size_t>(li)].entries;
        for (int ei = 0; ei < static_cast<int>(entries.size()); ++ei) {
            const SB::Entry& e = entries[static_cast<std::size_t>(ei)];
            Row r;
            switch (e.kind) {
                case SB::Kind::ManualBackup: r.kind = RowKind::ManualBackup; break;
                case SB::Kind::AppBackup:    r.kind = RowKind::AppBackup; break;
                case SB::Kind::ExportedCopy: r.kind = RowKind::ExportedCopy; break;
                default: continue;   // the source (its own row) and the staged-edits note
            }
            r.stamp = e.timestamp;
            r.path = e.path;
            r.recordDir = e.recordDir;
            r.listing = li;
            r.entry = ei;
            r.exists = e.exists;
            rows.push_back(std::move(r));
        }
    }
    order(rows);
    return rows;
}

} // namespace PokeBank::UIModel::BackupBrowser
