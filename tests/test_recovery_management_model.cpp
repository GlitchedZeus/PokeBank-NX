// Owner item #10 (recovery management): the pure safety policy of the view
// (include/UI/RecoveryManagementModel.h). No file system: every fact is given as an Item.
// Checked here:
//   last-copy protection (the newest copy and the newest verified copy of every source stay),
//   a recovery-only history keeps its newest copy, .previous without a verified primary is
//   protected and the most recent .previous is kept, .failed needs a verified primary, .tmp /
//   pending / folder copies are protected, paths outside the roots or with "..", ".", empty parts
//   or an empty path are refused, Bank files are protected, a member outside its record is refused,
//   the default selection is empty, A toggles removable rows only, the plan holds exactly the
//   selected removable items (and never empties a source), rows are grouped by source with badges,
//   and the confirmation lists every file with its size.
#include "UI/RecoveryManagementModel.h"

#include <iostream>
#include <string>
#include <vector>

namespace RM = PokeBank::UIModel::RecoveryManagement;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

const RM::Roots kRoots{"sdmc:/switch/PokeBank-NX/backups", "sdmc:/switch/PokeBank-NX/exports",
                       "sdmc:/switch/PokeBank-NX/banks"};
const std::string kManual = kRoots.backups + "/emulator-saves/black_nds";
const std::string kSourceA = "sdmc:/roms/nds/Black.sav";
const std::string kSourceB = "sdmc:/roms/nds/White.sav";

RM::Item record(RM::Kind kind, const std::string& dir, const std::string& source, const std::string& stamp,
                bool verified = true, bool owned = true) {
    RM::Item item;
    item.kind = kind;
    item.path = dir;
    item.members = {dir + "/save.sav", dir + "/BACKUP_MANIFEST.txt"};
    item.size = 1000;
    item.source = source;
    item.sortKey = stamp + "|" + dir.substr(dir.find_last_of('/') + 1);
    item.verified = verified;
    item.onlyAppFiles = true;
    item.owned = owned;
    return item;
}
RM::Item sibling(RM::Kind kind, const std::string& primary, const std::string& suffix, const std::string& mtime,
                 bool primaryVerified = true) {
    RM::Item item;
    item.kind = kind;
    item.path = primary + suffix;
    item.members = {item.path};
    item.size = 2048;
    item.source = primary;
    item.sortKey = mtime;
    item.primaryVerified = primaryVerified;
    return item;
}
bool removable(const std::vector<RM::Verdict>& v, std::size_t i) { return v[i].status == RM::Status::Removable; }

void lastCopyProtection() {
    std::vector<RM::Item> items = {
        record(RM::Kind::ManualBackup, kManual + "/20261001_100000", kSourceA, "20261001_100000"),
        record(RM::Kind::ManualBackup, kManual + "/20261002_100000", kSourceA, "20261002_100000"),
        record(RM::Kind::ManualBackup, kManual + "/20261003_100000", kSourceA, "20261003_100000"),
        // A single copy of another save: its last copy.
        record(RM::Kind::ManualBackup, kRoots.backups + "/emulator-saves/white_nds/20260901_100000", kSourceB,
               "20260901_100000"),
    };
    auto v = RM::classify(items, kRoots);
    check(removable(v, 0) && removable(v, 1), "older verified copies are removable");
    check(!removable(v, 2), "the newest copy of a source is protected");
    check(v[2].reason.find("Newest") != std::string::npos, "newest reason is plain language");
    check(!removable(v, 3), "the only copy of a source is protected");

    // Newest copy unverified: it stays, AND the newest verified copy stays too.
    items[2].verified = false;
    items[2].note = "its backup file could not be read back in full.";
    v = RM::classify(items, kRoots);
    check(!removable(v, 2), "an unverified newest copy is protected");
    check(v[2].reason.find("could not be read back") != std::string::npos, "unverified reason names the problem");
    check(!removable(v, 1) && v[1].reason.find("Newest verified") != std::string::npos,
          "the newest VERIFIED copy is protected when the newest is unverified");
    check(removable(v, 0), "the oldest verified copy is still removable");

    // A folder with foreign files, a missing manifest, or the source inside is protected.
    items = {record(RM::Kind::ExportRecord, kRoots.exports + "/gen5/r1", kSourceA, "1"),
             record(RM::Kind::ExportRecord, kRoots.exports + "/gen5/r2", kSourceA, "2"),
             record(RM::Kind::ExportRecord, kRoots.exports + "/gen5/r3", kSourceA, "3"),
             record(RM::Kind::ExportRecord, kRoots.exports + "/gen5/r4", "", "0"),
             record(RM::Kind::ExportRecord, kRoots.exports + "/gen5/r5", kRoots.exports + "/gen5/r5/save.sav", "0"),
             record(RM::Kind::ExportRecord, kRoots.exports + "/gen5/r6", kRoots.exports + "/gen5/r6/save.sav", "1")};
    items[0].onlyAppFiles = false;
    items[4].sourceInside = true;
    items[5].sourceInside = true;
    v = RM::classify(items, kRoots);
    check(!removable(v, 0) && v[0].reason.find("did not write") != std::string::npos,
          "a record with foreign files is protected");
    check(removable(v, 1), "a clean older record is removable");
    check(!removable(v, 3) && v[3].reason.find("manifest") != std::string::npos, "no manifest source: protected");
    check(!removable(v, 4) && v[4].reason.find("original save") != std::string::npos,
          "a record holding the original save is protected");
}

void recoveryOnlyHistory() {
    std::vector<RM::Item> items = {
        record(RM::Kind::ExportRecord, kRoots.exports + "/gen5/account-x/black_nds_20261001_100000", kSourceA,
               "20261001_100000", true, false),
        record(RM::Kind::ExportRecord, kRoots.exports + "/gen5/account-x/black_nds_20261002_100000", kSourceA,
               "20261002_100000", true, false),
    };
    auto v = RM::classify(items, kRoots);
    check(removable(v, 0), "recovery-only: an older copy may go");
    check(!removable(v, 1), "recovery-only: the newest copy is kept");
    check(v[1].reason.find("recovery only") != std::string::npos, "recovery-only reason says so");
    // A recovery-only history of one copy keeps it.
    items.erase(items.begin());
    v = RM::classify(items, kRoots);
    check(!removable(v, 0), "recovery-only single copy is protected");
}

void durableSiblings() {
    const std::string main = kRoots.backups + "/account-0123/sw/Working/main";
    std::vector<RM::Item> items = {
        sibling(RM::Kind::PreviousGeneration, main, ".previous.0", "100"),
        sibling(RM::Kind::PreviousGeneration, main, ".previous.1", "200"),
        sibling(RM::Kind::PreviousGeneration, main, ".previous.2", "300"),
        sibling(RM::Kind::FailedWrite, main, ".failed.0", "150"),
        sibling(RM::Kind::InterruptedWrite, main, ".tmp.0", "400"),
    };
    auto v = RM::classify(items, kRoots);
    check(removable(v, 0) && removable(v, 1), "older .previous with a verified primary is removable");
    check(!removable(v, 2), "the most recent .previous is kept");
    check(removable(v, 3), ".failed with a verified primary is removable");
    check(!removable(v, 4), ".tmp is never removable");
    // The uniqueSibling generation number is not the age: mtime decides.
    items[0].sortKey = "999";
    v = RM::classify(items, kRoots);
    check(!removable(v, 0) && removable(v, 2), "newest .previous is chosen by time, not generation number");

    for (auto& item : items) item.primaryVerified = false;
    v = RM::classify(items, kRoots);
    for (std::size_t i = 0; i < items.size(); ++i)
        check(!removable(v, i), "primary missing/unreadable: sibling " + items[i].path + " is protected");
    check(v[1].reason.find("missing or unreadable") != std::string::npos, ".previous reason names the primary");
    check(v[3].reason.find("missing or unreadable") != std::string::npos, ".failed reason names the primary");

    RM::Item pending;
    pending.kind = RM::Kind::PendingExport;
    pending.path = kRoots.exports + "/gen5/account-x/.black_nds_1.pending";
    RM::Item folder;
    folder.kind = RM::Kind::OtherRecovery;
    folder.path = kRoots.backups + "/account-0123/sw/Working.previous.open.20261001_100000";
    folder.members = {folder.path};
    v = RM::classify({pending, folder}, kRoots);
    check(!removable(v, 0) && !removable(v, 1), "pending exports and folder-level copies are protected");
}

void pathRefusals() {
    const auto single = [](const std::string& path) {
        RM::Item item = sibling(RM::Kind::FailedWrite, path, "", "1");
        item.path = path;
        item.members = {path};
        return item;
    };
    const std::vector<std::string> bad = {
        "",
        kRoots.backups + "/../config/settings.cfg.failed.0",
        kRoots.backups + "/./x.failed.0",
        kRoots.backups + "//x.failed.0",
        kRoots.backups + "/emulator-saves/x.failed.0/",
        "sdmc:/roms/nds/Black.sav.failed.0",                 // outside the roots
        "sdmc:/switch/PokeBank-NX/backupsX/x.failed.0",       // a sibling folder with the root as prefix
        kRoots.backups,                                       // the root itself
        "sdmc:/switch/PokeBank-NX/config/settings.cfg.failed.0",
        kRoots.backups + "\\x.failed.0",
    };
    for (const std::string& path : bad) {
        const auto v = RM::classify({single(path)}, kRoots);
        check(!removable(v, 0), "refused path: '" + path + "'");
        check(v[0].reason.find("Refused") != std::string::npos, "refusal reason given for '" + path + "'");
    }
    check(!RM::lexicallySafe("") && !RM::lexicallySafe("/a/../b") && RM::lexicallySafe("/tmp/a/b"),
          "lexicallySafe basics");

    // A record member outside its own folder, or a nested one, is refused.
    std::vector<RM::Item> items = {
        record(RM::Kind::ManualBackup, kManual + "/a", kSourceA, "1"),
        record(RM::Kind::ManualBackup, kManual + "/b", kSourceA, "2"),
        record(RM::Kind::ManualBackup, kManual + "/c", kSourceA, "3"),
    };
    items[0].members.push_back(kManual + "/b/save.sav");
    items[1].members.push_back(kManual + "/b/nested/x");
    auto v = RM::classify(items, kRoots);
    check(!removable(v, 0) && !removable(v, 1), "members outside the record's own folder are refused");
    items[0].members.clear();
    v = RM::classify(items, kRoots);
    check(!removable(v, 0), "a record with no recorded members is refused");

    // Bank files: protected, wherever the kind or path says Bank.
    RM::Item bank = single(kRoots.banks + "/bank.dat.previous.0");
    bank.kind = RM::Kind::PreviousGeneration;
    RM::Item bankFile;
    bankFile.kind = RM::Kind::BankFile;
    bankFile.path = kRoots.banks + "/bank.dat";
    v = RM::classify({bank, bankFile}, kRoots);
    check(!removable(v, 0) && v[0].reason.find("Bank") != std::string::npos, "a .previous under banks is protected");
    check(!removable(v, 1) && v[1].reason.find("Bank") != std::string::npos, "the Bank file is protected");
}

void selectionAndPlan() {
    std::vector<RM::Item> items = {
        record(RM::Kind::ManualBackup, kManual + "/20261001_100000", kSourceA, "20261001_100000"),
        record(RM::Kind::ManualBackup, kManual + "/20261002_100000", kSourceA, "20261002_100000"),
        record(RM::Kind::ManualBackup, kManual + "/20261003_100000", kSourceA, "20261003_100000"),
        sibling(RM::Kind::FailedWrite, kRoots.backups + "/account-1/sw/Working/main", ".failed.0", "5"),
    };
    const auto v = RM::classify(items, kRoots);
    auto selection = RM::emptySelection(items.size());
    check(RM::selectedCount(selection) == 0, "default selection is empty");
    check(RM::buildPlan(items, v, selection).items.empty(), "empty selection: empty plan");
    check(!RM::toggle(selection, v, 2) && RM::selectedCount(selection) == 0, "A on a protected row selects nothing");
    check(!RM::toggle(selection, v, 99), "an out-of-range toggle is refused");
    check(RM::toggle(selection, v, 0) && RM::toggle(selection, v, 3), "A selects removable rows");
    auto plan = RM::buildPlan(items, v, selection);
    check(plan.items == std::vector<std::size_t>({0, 3}), "plan holds exactly the selected removable items");
    check(plan.bytes == 1000 + 2048 && plan.files == 3, "plan totals bytes and files");
    check(RM::toggle(selection, v, 0), "A again deselects");
    plan = RM::buildPlan(items, v, selection);
    check(plan.items == std::vector<std::size_t>({3}), "a deselected item leaves the plan");

    // Even a forged selection of a protected item never reaches the plan.
    selection.picked = {true, true, true, true};
    plan = RM::buildPlan(items, v, selection);
    check(plan.items == std::vector<std::size_t>({0, 1, 3}), "protected items never enter the plan");

    // Defence in depth: a verdict list that (wrongly) calls every record removable still cannot
    // empty a source.
    std::vector<RM::Verdict> forged(items.size(), RM::Verdict{RM::Status::Removable, "forged"});
    plan = RM::buildPlan(items, forged, selection);
    check(plan.items == std::vector<std::size_t>({3}), "a plan never removes every record of a source");

    // Rows: grouped by source with a heading, newest first, badges, sizes.
    selection = RM::emptySelection(items.size());
    RM::toggle(selection, v, 1);
    const auto rows = RM::rows(items, v, selection);
    check(rows.size() == 6, "two headings + four items");
    check(rows[0].item == -1 && rows[0].title == "Save: Black.sav" && rows[0].detail == kSourceA,
          "first group heading names the source save");
    check(rows[1].item == 2 && rows[1].badge == "Protected", "newest first, protected badge");
    check(rows[2].item == 1 && rows[2].badge == "Selected", "selected badge");
    check(rows[3].item == 0 && rows[3].badge == "Removable", "removable badge");
    check(rows[3].title.find("Backup") != std::string::npos && rows[3].title.find("1000 B") != std::string::npos,
          "row title shows kind and size");
    check(rows[4].item == -1 && rows[4].title == "Save: main", "second group heading");
    check(rows[5].item == 3 && rows[5].detail.find("never used") != std::string::npos, "row detail shows the reason");

    // Confirmation: every member file listed, with sizes and a total.
    selection = RM::emptySelection(items.size());
    RM::toggle(selection, v, 0);
    RM::toggle(selection, v, 3);
    plan = RM::buildPlan(items, v, selection);
    const auto lines = RM::confirmationLines(items, plan);
    check(lines.size() == 3, "one line per file to remove");
    check(lines[0] == kManual + "/20261001_100000/save.sav", "the exact file is named");
    check(lines[1].find("BACKUP_MANIFEST.txt") != std::string::npos && lines[1].find("1000 B in total") != std::string::npos,
          "a record's size is shown on its last file");
    check(lines[2].find("main.failed.0") != std::string::npos && lines[2].find("2.0 KiB") != std::string::npos,
          "a single file shows its size");
    check(RM::confirmationSummary(plan) == "Remove 2 copies (3 files, 3.0 KiB)?", "summary names counts and size");
    // With per-file sizes (the file-system layer records them) every line carries its own size.
    items[0].memberSizes = {900, 100};
    const auto sized = RM::confirmationLines(items, plan);
    check(sized.size() == 3 && sized[0] == kManual + "/20261001_100000/save.sav  (900 B)" &&
              sized[1] == kManual + "/20261001_100000/BACKUP_MANIFEST.txt  (100 B)",
          "each file is listed with its own size");
}

} // namespace

int main() {
    lastCopyProtection();
    recoveryOnlyHistory();
    durableSiblings();
    pathRefusals();
    selectionAndPlan();
    std::cout << "Recovery management model: " << checks << " checks, " << failures << " failure(s).\n";
    return failures == 0 ? 0 : 1;
}
