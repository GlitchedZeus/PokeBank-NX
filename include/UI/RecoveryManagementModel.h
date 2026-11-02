#pragma once

// Owner item #10: "A recovery-management view that can review and remove leftover recovery files
// and export records, with confirmation. Never casual deletion of last copies."
//
// This header is the whole safety policy of that view, and it is pure: it never touches the file
// system. The file-system layer (UI/RecoveryManagementFs.h) only gathers facts into Items and
// performs a confirmed Plan, re-checking every item against this policy just before removing it.
//
// What may be removed (everything else is Protected, with a plain-language reason):
//   * ManualBackup / ExportRecord: a PokeBank NX backup or export record folder that holds exactly
//     the files PokeBank NX wrote, verified (manifest readable, every named file read back in full),
//     and that is neither the NEWEST copy nor the NEWEST VERIFIED copy of its source save. A history
//     with no current owner (recovery-only) keeps its newest copy too, so every source always keeps
//     at least one copy and its newest verified copy.
//   * PreviousGeneration (<file>.previous.<N>): written by DurableFile::replace, which moves the old
//     authoritative file aside before promoting the new one (src/Utils/DurableFile.cpp, replace);
//     DurableFile::recoverMissingTarget restores the newest validating one when <file> is missing.
//     So one is removable only when <file> exists and was read back in full, and never the most
//     recent .previous of that file (kept as the one-step-back copy).
//   * FailedWrite (<file>.failed.<N>): DurableFile preserves bytes here only when a write FAILED its
//     check (temp reread mismatch / validator refusal, or the promoted file failing reread; in that
//     case the previous generation is put back). recoverMissingTarget never promotes it ("Temp/failed
//     candidates are never promoted"). It is not an interrupted write (that is .tmp.<N>). It is still
//     kept while <file> is missing or unreadable: then the save has not been recovered yet.
//
// Never removable here: .tmp.<N> (an interrupted write that may hold newer data), Gen V ".pending"
// export reservations, folder-level recovery copies (.previous.<stamp> / .incomplete. / .unreadable.
// folders), anything under the Bank folder (Bank files and Bank recovery generations, Round 8),
// anything outside the app's own backup and export roots, and any path with an empty, "." or ".."
// part. The original emulator / game save is never an Item a plan can hold.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::RecoveryManagement {

enum class Kind : unsigned char {
    ManualBackup,        // backups/emulator-saves/<game>/<record>/ (BACKUP_MANIFEST.txt)
    ExportRecord,        // exports/genN/<record>/ or exports/gen5/account-<profile>/<record>/
    PreviousGeneration,  // <file>.previous.<N>  (DurableFile)
    FailedWrite,         // <file>.failed.<N>    (DurableFile)
    InterruptedWrite,    // <file>.tmp.<N>       (DurableFile)
    PendingExport,       // exports/gen5/account-<profile>/.<label>.pending
    OtherRecovery,       // folder-level or unrecognised recovery copies
    BankFile,            // anything under the Bank folder
};

enum class Status : unsigned char { Removable, Protected };

[[nodiscard]] constexpr const char* kindLabel(Kind kind) noexcept {
    switch (kind) {
        case Kind::ManualBackup: return "Backup";
        case Kind::ExportRecord: return "Export record";
        case Kind::PreviousGeneration: return "Earlier version (.previous)";
        case Kind::FailedWrite: return "Failed write (.failed)";
        case Kind::InterruptedWrite: return "Interrupted write (.tmp)";
        case Kind::PendingExport: return "Unfinished export";
        case Kind::OtherRecovery: return "Recovery copy";
        case Kind::BankFile: return "Bank";
    }
    return "Recovery copy";
}

[[nodiscard]] constexpr bool isRecord(Kind kind) noexcept {
    return kind == Kind::ManualBackup || kind == Kind::ExportRecord;
}

// The app's own roots. Removal is only ever allowed inside `backups` or `exports`; `banks` is
// listed read-only. Each is a path with no trailing slash (PokeBank::Paths::backupsRoot() etc.).
struct Roots {
    std::string backups, exports, banks;
};

struct Item {
    Kind kind = Kind::OtherRecovery;
    std::string path;                  // record folder, or the single file
    std::vector<std::string> members;  // files a removal deletes (records: manifest + its files)
    std::uint64_t size = 0;            // bytes of `members` (or of the file)
    std::vector<std::uint64_t> memberSizes;  // optional: bytes of each member, same order
    std::string source;                // group: SOURCE_PATH of a record, the primary file of a sibling
    std::string sortKey;               // within a group, a greater key is newer
    bool verified = false;             // records: manifest + every named file read back in full
    bool onlyAppFiles = false;         // records: the folder holds only what PokeBank NX wrote
    bool sourceInside = false;         // records: the original save lives inside this folder
    bool primaryVerified = false;      // .previous / .failed: <file> exists and was read in full
    bool owned = false;                // a current game assignment owns `source`
    std::string fingerprint;           // physical stamps when reviewed (FS layer)
    std::string note;                  // why `verified` is false, when it is
};

struct Verdict {
    Status status = Status::Protected;
    std::string reason;
};

namespace detail {

inline bool splitParts(std::string_view path, std::vector<std::string_view>& parts) {
    parts.clear();
    if (path.empty() || path.find('\\') != std::string_view::npos ||
        path.find('\0') != std::string_view::npos)
        return false;
    std::size_t start = 0;
    if (path.substr(0, 6) == "sdmc:/") start = 6;
    else if (path.front() == '/') start = 1;
    while (start <= path.size()) {
        const std::size_t slash = path.find('/', start);
        const std::size_t end = slash == std::string_view::npos ? path.size() : slash;
        const std::string_view part = path.substr(start, end - start);
        if (part.empty() || part == "." || part == "..") return false;
        parts.push_back(part);
        if (slash == std::string_view::npos) break;
        start = slash + 1;
    }
    return !parts.empty();
}

} // namespace detail

// A path whose every part is a real name: no empty part (no "//", no trailing "/"), no "." and
// no "..". This is the lexical half of the path check; the FS layer adds the physical half
// (every component lstat-ed, no links, canonical path inside the root).
[[nodiscard]] inline bool lexicallySafe(std::string_view path) {
    std::vector<std::string_view> parts;
    return detail::splitParts(path, parts);
}

// `path` is strictly inside `root` (never the root itself) and both are lexically safe.
[[nodiscard]] inline bool within(std::string_view path, std::string_view root) {
    if (!lexicallySafe(path) || !lexicallySafe(root)) return false;
    return path.size() > root.size() + 1 && path.substr(0, root.size()) == root &&
           path[root.size()] == '/';
}

// Empty when `item` sits where a removal may happen; otherwise why not.
[[nodiscard]] inline std::string locationRefusal(const Item& item, const Roots& roots) {
    if (item.path.empty()) return "Refused: the path is empty.";
    if (!lexicallySafe(item.path)) return "Refused: the path has an empty, \".\" or \"..\" part.";
    if (!roots.banks.empty() && (item.path == roots.banks || within(item.path, roots.banks)))
        return "Bank files and Bank recovery copies are never removed here.";
    const std::string* root = nullptr;
    if (!roots.backups.empty() && within(item.path, roots.backups)) root = &roots.backups;
    else if (!roots.exports.empty() && within(item.path, roots.exports)) root = &roots.exports;
    if (!root) return "Refused: outside PokeBank NX's backup and export folders.";
    if (item.members.empty()) return "Refused: nothing to remove was recorded for it.";
    for (const std::string& member : item.members) {
        if (!within(member, *root)) return "Refused: one of its files is outside PokeBank NX's folders.";
        // A single file removes only itself; a record removes only its own direct children.
        const bool self = member == item.path && !isRecord(item.kind);
        const bool child = isRecord(item.kind) && member.size() > item.path.size() + 1 &&
                           member.compare(0, item.path.size() + 1, item.path + "/") == 0 &&
                           member.find('/', item.path.size() + 1) == std::string::npos;
        if (!self && !child) return "Refused: one of its files is not part of this copy.";
    }
    return {};
}

// Classifies every item. Pure: the result depends only on `items` and `roots`.
[[nodiscard]] inline std::vector<Verdict> classify(const std::vector<Item>& items, const Roots& roots) {
    std::vector<Verdict> out(items.size());
    // Newest and newest-verified record of each source; newest .previous of each primary file.
    std::map<std::string, std::size_t> newest, newestVerified, newestPrevious;
    const auto newer = [&](std::size_t a, std::size_t b) {   // a is newer than b
        return items[a].sortKey != items[b].sortKey ? items[a].sortKey > items[b].sortKey
                                                    : items[a].path > items[b].path;
    };
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item& item = items[i];
        if (isRecord(item.kind) && !item.source.empty()) {
            auto it = newest.find(item.source);
            if (it == newest.end() || newer(i, it->second)) newest[item.source] = i;
            if (item.verified) {
                auto v = newestVerified.find(item.source);
                if (v == newestVerified.end() || newer(i, v->second)) newestVerified[item.source] = i;
            }
        } else if (item.kind == Kind::PreviousGeneration && !item.source.empty()) {
            auto it = newestPrevious.find(item.source);
            if (it == newestPrevious.end() || newer(i, it->second)) newestPrevious[item.source] = i;
        }
    }
    const auto keep = [](std::string reason) { return Verdict{Status::Protected, std::move(reason)}; };
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item& item = items[i];
        Verdict& v = out[i];
        if (item.kind == Kind::BankFile) {
            v = keep("Bank files and Bank recovery copies are never removed here.");
            continue;
        }
        const std::string where = locationRefusal(item, roots);
        if (!where.empty()) { v = keep(where); continue; }
        switch (item.kind) {
            case Kind::ManualBackup:
            case Kind::ExportRecord: {
                if (item.source.empty()) { v = keep("Kept: its manifest could not be read."); break; }
                if (item.sourceInside) { v = keep("Kept: the original save is inside this folder."); break; }
                if (!item.onlyAppFiles) { v = keep("Kept: this folder holds files PokeBank NX did not write."); break; }
                if (!item.verified) {
                    v = keep(item.note.empty() ? "Kept: it could not be read back in full."
                                               : "Kept: " + item.note);
                    break;
                }
                if (newest.at(item.source) == i) {
                    v = keep(item.owned ? "Newest copy of this save; always kept."
                                        : "No game owns this save now (recovery only); its newest copy is always kept.");
                    break;
                }
                const auto verifiedIt = newestVerified.find(item.source);
                if (verifiedIt != newestVerified.end() && verifiedIt->second == i) {
                    v = keep("Newest verified copy of this save; always kept.");
                    break;
                }
                v = {Status::Removable, "Older copy; a newer verified copy of this save stays."};
                break;
            }
            case Kind::PreviousGeneration:
                if (item.source.empty() || !item.primaryVerified)
                    v = keep("Kept: the file it belongs to is missing or unreadable, so it may be needed to recover it.");
                else if (newestPrevious.at(item.source) == i)
                    v = keep("Kept: the most recent earlier version of this file.");
                else
                    v = {Status::Removable, "Older earlier version; the current file and a newer earlier version stay."};
                break;
            case Kind::FailedWrite:
                if (item.source.empty() || !item.primaryVerified)
                    v = keep("Kept: the file it belongs to is missing or unreadable; kept until it is recovered.");
                else
                    v = {Status::Removable, "A write that failed its check and was never used; the current file reads back in full."};
                break;
            case Kind::InterruptedWrite:
                v = keep("Kept: an interrupted write that may hold newer data.");
                break;
            case Kind::PendingExport:
                v = keep("Kept: an export that did not finish.");
                break;
            case Kind::OtherRecovery:
            case Kind::BankFile:
                v = keep("Kept: a recovery copy this view does not remove.");
                break;
        }
    }
    return out;
}

// Selection: one flag per item; the default selects nothing.
struct Selection {
    std::vector<bool> picked;
};
[[nodiscard]] inline Selection emptySelection(std::size_t count) { return Selection{std::vector<bool>(count, false)}; }

// A toggles a Removable item only. Returns false (and changes nothing) for a protected item.
inline bool toggle(Selection& selection, const std::vector<Verdict>& verdicts, std::size_t index) {
    if (index >= verdicts.size() || index >= selection.picked.size() ||
        verdicts[index].status != Status::Removable)
        return false;
    selection.picked[index] = !selection.picked[index];
    return true;
}

[[nodiscard]] inline std::size_t selectedCount(const Selection& selection) {
    return static_cast<std::size_t>(std::count(selection.picked.begin(), selection.picked.end(), true));
}

struct Plan {
    std::vector<std::size_t> items;   // indices into the reviewed listing, in listing order
    std::uint64_t bytes = 0;
    std::size_t files = 0;
};

// Exactly the selected Removable items. As a second line of defence a plan never empties a
// source: if the selection somehow covers every record of a source, those records are dropped.
[[nodiscard]] inline Plan buildPlan(const std::vector<Item>& items, const std::vector<Verdict>& verdicts,
                                    const Selection& selection) {
    Plan plan;
    std::map<std::string, std::size_t> records, planned;
    for (std::size_t i = 0; i < items.size(); ++i)
        if (isRecord(items[i].kind)) ++records[items[i].source];
    std::vector<std::size_t> picked;
    for (std::size_t i = 0; i < items.size() && i < verdicts.size() && i < selection.picked.size(); ++i)
        if (selection.picked[i] && verdicts[i].status == Status::Removable) {
            picked.push_back(i);
            if (isRecord(items[i].kind)) ++planned[items[i].source];
        }
    for (const std::size_t i : picked) {
        if (isRecord(items[i].kind) && planned[items[i].source] >= records[items[i].source]) continue;
        plan.items.push_back(i);
        plan.bytes += items[i].size;
        plan.files += items[i].members.size();
    }
    return plan;
}

[[nodiscard]] inline std::string sizeText(std::uint64_t bytes) {
    char buffer[32];
    if (bytes < 1024) std::snprintf(buffer, sizeof(buffer), "%llu B", static_cast<unsigned long long>(bytes));
    else if (bytes < 1024ull * 1024ull)
        std::snprintf(buffer, sizeof(buffer), "%.1f KiB", static_cast<double>(bytes) / 1024.0);
    else std::snprintf(buffer, sizeof(buffer), "%.1f MiB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    return buffer;
}

[[nodiscard]] inline std::string leafName(const std::string& path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

// One drawn row of the view: a source heading, or an item.
struct Row {
    int item = -1;            // -1: a heading row (not selectable)
    std::string title, detail, badge;
};

// Rows grouped by source (headings in source order), each group newest first.
[[nodiscard]] inline std::vector<Row> rows(const std::vector<Item>& items, const std::vector<Verdict>& verdicts,
                                           const Selection& selection) {
    std::vector<std::size_t> order(items.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    const auto group = [&](std::size_t i) {
        return items[i].kind == Kind::BankFile ? std::string("\x7f" "Bank") : items[i].source;
    };
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        if (group(a) != group(b)) return group(a) < group(b);
        if (items[a].sortKey != items[b].sortKey) return items[a].sortKey > items[b].sortKey;
        return items[a].path < items[b].path;
    });
    std::vector<Row> out;
    std::string current;
    bool first = true;
    for (const std::size_t i : order) {
        const std::string g = group(i);
        if (first || g != current) {
            first = false;
            current = g;
            Row heading;
            if (items[i].kind == Kind::BankFile) {
                heading.title = "Bank (read only)";
                heading.detail = "Bank files and their recovery copies are listed only.";
            } else if (items[i].source.empty()) {
                heading.title = "Unknown save";
                heading.detail = "No source recorded";
            } else {
                heading.title = "Save: " + leafName(items[i].source);
                heading.detail = items[i].source;
            }
            out.push_back(std::move(heading));
        }
        Row row;
        row.item = static_cast<int>(i);
        row.title = std::string(kindLabel(items[i].kind)) + " - " + leafName(items[i].path) + " (" +
                    sizeText(items[i].size) + ")";
        const bool removable = i < verdicts.size() && verdicts[i].status == Status::Removable;
        row.detail = i < verdicts.size() ? verdicts[i].reason : std::string{};
        const bool picked = i < selection.picked.size() && selection.picked[i];
        row.badge = picked ? "Selected" : removable ? "Removable" : "Protected";
        out.push_back(std::move(row));
    }
    return out;
}

// The confirmation's lines: every file a plan removes, with its size, then the total.
[[nodiscard]] inline std::vector<std::string> confirmationLines(const std::vector<Item>& items, const Plan& plan) {
    std::vector<std::string> lines;
    for (const std::size_t i : plan.items) {
        if (i >= items.size() || items[i].members.empty()) continue;
        const auto& item = items[i];
        if (item.memberSizes.size() == item.members.size()) {
            for (std::size_t m = 0; m < item.members.size(); ++m)
                lines.push_back(item.members[m] + "  (" + sizeText(item.memberSizes[m]) + ")");
            continue;
        }
        for (const std::string& member : item.members) lines.push_back(member);
        lines.back() += "  (" + sizeText(item.size) + (item.members.size() > 1 ? " in total)" : ")");
    }
    return lines;
}

[[nodiscard]] inline std::string confirmationSummary(const Plan& plan) {
    return "Remove " + std::to_string(plan.items.size()) + (plan.items.size() == 1 ? " copy" : " copies") +
           " (" + std::to_string(plan.files) + (plan.files == 1 ? " file, " : " files, ") +
           sizeText(plan.bytes) + ")?";
}

} // namespace PokeBank::UIModel::RecoveryManagement
