// Owner item #10 (recovery management): the file-system layer (include/UI/RecoveryManagementFs.h)
// on a realistic tree in a private mkdtemp directory, end to end: enumerate -> select -> confirm
// (plan) -> remove. Synthetic bytes only; no real save is read.
//
// The tree: emulator-save backups made by the real SourceBackups::createBackup (three of one save),
// Gen III export records of another save (one with a foreign file), this profile's Gen V records of
// a save that no longer exists (recovery only) plus an unfinished ".pending" export, another
// profile's Gen V record, this profile's Switch-save Working folder with DurableFile .previous /
// .failed / .tmp leftovers, leftovers whose primary is missing, a folder-level recovery copy, a Bank
// file with its .previous generation, and decoys outside the roots reached through links.
// Proved: exactly the expected copies are removable; after removal exactly those files are gone and
// every other file (original saves, decoys, Bank, other profile, every protected copy) is
// byte-identical; a file changed between review and confirm is refused; a copy that became the
// newest is refused; a forged ".." path is refused. The temporary directory is removed.
#include "UI/RecoveryManagementFs.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace RM = PokeBank::UIModel::RecoveryManagement;
namespace RMF = PokeBank::UIModel::RecoveryManagement::Fs;
namespace SB = PokeBank::UIModel::SourceBackups;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
std::vector<uint8_t> slurp(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}
// Every file under `root` (links are recorded, never followed) with its bytes.
std::map<std::string, std::vector<uint8_t>> tree(const fs::path& root) {
    std::map<std::string, std::vector<uint8_t>> out;
    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (e.is_symlink()) out[e.path().string()] = {'L'};
        else out[e.path().string()] = e.is_regular_file() ? slurp(e.path()) : std::vector<uint8_t>{};
    }
    return out;
}
void age(const fs::path& path, int secondsAgo) {
    fs::last_write_time(path, fs::file_time_type::clock::now() - std::chrono::seconds(secondsAgo));
}
void exportRecord(const fs::path& dir, const std::string& source, const std::string& stamp, bool gen5) {
    write(dir / "original_backup.sav", "orig-" + stamp);
    write(dir / "edited.sav", "edit-" + stamp);
    write(dir / (gen5 ? "EXPORT_MANIFEST.txt" : "EDIT_MANIFEST.txt"),
          "GAME_ID=" + std::string(gen5 ? "black_nds" : "firered_gba") + "\nSOURCE_PATH=" + source +
              "\nTIMESTAMP=" + stamp + "\nORIGINAL_BACKUP=original_backup.sav\nEDITED_SAVE=edited.sav\n");
}
int indexOf(const RMF::Listing& l, const std::string& path) {
    for (std::size_t i = 0; i < l.items.size(); ++i)
        if (l.items[i].path == path) return static_cast<int>(i);
    return -1;
}
bool listedAnywhere(const RMF::Listing& l, const std::string& fragment) {
    for (const auto& item : l.items) {
        if (item.path.find(fragment) != std::string::npos) return true;
        for (const auto& m : item.members)
            if (m.find(fragment) != std::string::npos) return true;
    }
    return false;
}

const std::string kProfile = "0123456789abcdef0123456789abcdef";
const std::string kOther = "fedcba9876543210fedcba9876543210";
} // namespace

int main() {
    char templ[] = "/tmp/pokebank-recovery-XXXXXX";
    const char* made = ::mkdtemp(templ);
    if (!made) { std::cerr << "mkdtemp failed\n"; return 1; }
    const fs::path base = made;
    const fs::path app = base / "app", roms = base / "roms", outside = base / "outside";
    RMF::Request request;
    request.roots = {(app / "backups").string(), (app / "exports").string(), (app / "banks").string()};
    request.profileIdentity = kProfile;

    // Original saves: never part of any plan.
    const fs::path black = roms / "Black.sav", red = roms / "FireRed.sav", missing = roms / "Gone.sav";
    write(black, std::string(512, 'B'));
    write(red, std::string(256, 'R'));
    request.ownedSources = {black.string(), red.string()};

    // 1. Three deliberate backups of Black.sav through the real writer.
    const std::string manual = SB::emulatorBackupsRoot(request.roots.backups);
    std::vector<std::string> blackBackups;
    for (const char* stamp : {"20261001_100000", "20261002_100000", "20261003_100000"}) {
        const auto r = SB::createBackup(manual, "black_nds", black.string(), stamp);
        check(r.ok, std::string("createBackup ") + stamp + ": " + r.error);
        blackBackups.push_back(r.recordDir);
    }
    // 2. Gen III export records of FireRed.sav: older (foreign file), middle, newest.
    const fs::path gen3 = app / "exports" / "gen3";
    exportRecord(gen3 / "firered_20260901_100000", red.string(), "20260901_100000", false);
    write(gen3 / "firered_20260901_100000" / "notes.txt", "mine");
    exportRecord(gen3 / "firered_20260902_100000", red.string(), "20260902_100000", false);
    exportRecord(gen3 / "firered_20260903_100000", red.string(), "20260903_100000", false);
    // 3. This profile's Gen V records of a save that is gone (recovery only), and a pending export.
    const fs::path account5 = app / "exports" / "gen5" / ("account-" + kProfile);
    exportRecord(account5 / "black_nds_20261010_100000", missing.string(), "20261010_100000", true);
    exportRecord(account5 / "black_nds_20261011_100000", missing.string(), "20261011_100000", true);
    write(account5 / ".black_nds_20261012_100000.pending" / "edited.sav", "half");
    // Another profile's Gen V records: never read, never removed.
    exportRecord(app / "exports" / "gen5" / ("account-" + kOther) / "black_nds_20261001_100000", missing.string(),
                 "20261001_100000", true);
    exportRecord(app / "exports" / "gen5" / ("account-" + kOther) / "black_nds_20261002_100000", missing.string(),
                 "20261002_100000", true);
    write(app / "exports" / "gen5" / ("account-" + kOther) / "x.failed.0", "other");
    // 4. This profile's Switch-save Working folder with DurableFile leftovers.
    const fs::path working = app / "backups" / ("account-" + kProfile) / "0100ABF008968000" / "Working";
    write(working / "main", std::string(1024, 'M'));
    write(working / "main.previous.0", std::string(1000, 'p'));
    write(working / "main.previous.1", std::string(1001, 'q'));
    write(working / "main.failed.0", std::string(900, 'f'));
    write(working / "main.tmp.0", std::string(1024, 't'));
    age(working / "main.previous.0", 600);   // older, though its generation number is lower
    age(working / "main.previous.1", 60);
    // Leftovers whose primary is missing (an unrecovered save): protected.
    write(working / "save2.previous.0", "p2");
    write(working / "save2.failed.0", "f2");
    // A folder-level recovery copy (WorkingCopy refresh could not rename it): protected, not entered.
    write(working.parent_path() / "Working.previous.open.20261001_100000" / "main.previous.0", "inside");
    // Another profile's Switch tree: never read.
    write(app / "backups" / ("account-" + kOther) / "sw" / "Working" / "main", "o");
    write(app / "backups" / ("account-" + kOther) / "sw" / "Working" / "main.previous.0", "o0");
    // 5. Bank: listed read-only.
    write(app / "banks" / "legacy-pkse" / "bank.dat", std::string(64, 'K'));
    write(app / "banks" / "legacy-pkse" / "bank.dat.previous.0", std::string(64, 'k'));
    // 6. Decoys outside the roots, reachable only through links planted inside them.
    write(outside / "decoy", "decoy");
    write(outside / "decoy.previous.0", "decoy0");
    write(outside / "decoy.previous.1", "decoy1");
    write(outside / "decoy.failed.0", "decoyf");
    fs::create_directory_symlink(outside, fs::path(manual) / "linked_game");
    fs::create_directory_symlink(outside, working / "escape");
    fs::create_symlink(outside / "decoy.failed.0", working / "linked.failed.0");

    const auto before = tree(base);
    const RMF::Listing listing = RMF::enumerate(request);

    // Nothing outside the roots, nothing through a link, no other profile, no original save.
    check(!listedAnywhere(listing, outside.string()), "nothing outside the roots is listed");
    check(!listedAnywhere(listing, "linked") && !listedAnywhere(listing, "escape"), "links are never followed");
    check(!listedAnywhere(listing, kOther), "another profile's folders are never read");
    for (const auto& item : listing.items)
        for (const auto& m : item.members)
            check(m.rfind(roms.string(), 0) != 0, "no original save is a member: " + m);

    const auto verdictOf = [&](const std::string& path) -> const RM::Verdict* {
        const int i = indexOf(listing, path);
        return i < 0 ? nullptr : &listing.verdicts[static_cast<std::size_t>(i)];
    };
    const auto removable = [&](const std::string& path) {
        const auto* v = verdictOf(path);
        return v && v->status == RM::Status::Removable;
    };
    const auto protectedRow = [&](const std::string& path) {
        const auto* v = verdictOf(path);
        return v && v->status == RM::Status::Protected;
    };
    const std::set<std::string> expectedRemovable = {
        blackBackups[0], blackBackups[1],
        (gen3 / "firered_20260902_100000").string(),
        (account5 / "black_nds_20261010_100000").string(),
        (working / "main.previous.0").string(),
        (working / "main.failed.0").string(),
    };
    std::set<std::string> actualRemovable;
    for (std::size_t i = 0; i < listing.items.size(); ++i)
        if (listing.verdicts[i].status == RM::Status::Removable) actualRemovable.insert(listing.items[i].path);
    check(actualRemovable == expectedRemovable, "exactly the expected copies are removable");
    for (const auto& p : actualRemovable) check(expectedRemovable.count(p) == 1, "unexpected removable: " + p);

    for (const auto& p : expectedRemovable) check(removable(p), "removable as expected: " + p);
    check(protectedRow(blackBackups[2]), "newest backup of Black.sav is protected");
    check(protectedRow((gen3 / "firered_20260901_100000").string()), "export record with a foreign file is protected");
    check(protectedRow((gen3 / "firered_20260903_100000").string()), "newest FireRed record is protected");
    check(protectedRow((account5 / "black_nds_20261011_100000").string()), "recovery-only newest is protected");
    check(protectedRow((account5 / ".black_nds_20261012_100000.pending").string()), "pending export listed, protected");
    check(protectedRow((working / "main.previous.1").string()), "most recent .previous is protected");
    check(protectedRow((working / "main.tmp.0").string()), ".tmp is protected");
    check(protectedRow((working / "save2.previous.0").string()), ".previous without its primary is protected");
    check(protectedRow((working / "save2.failed.0").string()), ".failed without its primary is protected");
    check(protectedRow((working.parent_path() / "Working.previous.open.20261001_100000").string()),
          "folder-level recovery copy is protected");
    check(!listedAnywhere(listing, "Working.previous.open.20261001_100000/main.previous.0"),
          "a folder-level copy is not entered");
    check(protectedRow((app / "banks" / "legacy-pkse" / "bank.dat").string()), "the Bank file is listed, protected");
    check(protectedRow((app / "banks" / "legacy-pkse" / "bank.dat.previous.0").string()),
          "the Bank recovery generation is listed, protected");
    const int blackOld = indexOf(listing, blackBackups[0]);
    check(blackOld >= 0 && listing.items[static_cast<std::size_t>(blackOld)].owned &&
              listing.items[static_cast<std::size_t>(blackOld)].members.size() == 2 &&
              listing.items[static_cast<std::size_t>(blackOld)].size > 512,
          "a backup record lists its two files and their size");

    // Review: default nothing selected; select every removable row; confirm.
    auto selection = RM::emptySelection(listing.items.size());
    check(RM::buildPlan(listing.items, listing.verdicts, selection).items.empty(), "nothing selected by default");
    for (std::size_t i = 0; i < listing.items.size(); ++i) RM::toggle(selection, listing.verdicts, i);
    const RM::Plan plan = RM::buildPlan(listing.items, listing.verdicts, selection);
    check(plan.items.size() == expectedRemovable.size(), "the plan holds every selected removable copy");
    check(tree(base) == before, "enumerate and review change nothing on disk");

    const auto outcomes = RMF::removePlanned(request, listing, plan);
    check(outcomes.size() == plan.items.size(), "one outcome per planned copy");
    for (const auto& o : outcomes) check(o.removed, "removed: " + o.path + " (" + o.message + ")");

    // Exactly the planned files are gone; everything else is byte-identical.
    std::set<std::string> gone;
    for (const std::size_t i : plan.items) {
        gone.insert(listing.items[i].path);
        for (const auto& m : listing.items[i].members) gone.insert(m);
    }
    const auto after = tree(base);
    for (const auto& [path, bytes] : before) {
        if (gone.count(path)) check(!after.count(path), "removed as planned: " + path);
        else {
            const auto it = after.find(path);
            check(it != after.end() && it->second == bytes, "untouched: " + path);
        }
    }
    check(after.size() + gone.size() == before.size(), "nothing else appeared or vanished");
    check(slurp(black) == std::vector<uint8_t>(512, 'B') && slurp(red) == std::vector<uint8_t>(256, 'R'),
          "original saves are byte-identical");
    check(fs::exists(outside / "decoy.previous.0") && fs::exists(outside / "decoy.failed.0"), "decoys survive");

    // Second pass: everything left is protected; every source keeps its newest copy.
    RMF::Listing again = RMF::enumerate(request);
    for (std::size_t i = 0; i < again.items.size(); ++i)
        check(again.verdicts[i].status == RM::Status::Protected, "after cleanup protected: " + again.items[i].path);

    // Changed between review and confirm: refused, file untouched.
    const auto third = SB::createBackup(manual, "black_nds", black.string(), "20261004_100000");
    check(third.ok, "a fourth backup");
    RMF::Listing review = RMF::enumerate(request);
    const int target = indexOf(review, blackBackups[2]);
    check(target >= 0 && review.verdicts[static_cast<std::size_t>(target)].status == RM::Status::Removable,
          "the former newest is removable once a newer backup exists");
    selection = RM::emptySelection(review.items.size());
    RM::toggle(selection, review.verdicts, static_cast<std::size_t>(target));
    const RM::Plan one = RM::buildPlan(review.items, review.verdicts, selection);
    const fs::path changedFile = fs::path(blackBackups[2]) / "save.sav";
    const auto originalBytes = slurp(changedFile);
    {
        std::ofstream rewrite(changedFile, std::ios::binary | std::ios::in | std::ios::out);
        rewrite.put('X');   // same size, new contents and times
    }
    auto changedBytes = slurp(changedFile);
    auto result = RMF::removePlanned(request, review, one);
    check(result.size() == 1 && !result[0].removed, "a copy changed after review is refused");
    check(result.size() == 1 && (result[0].message.find("changed") != std::string::npos ||
                                 result[0].message.find("protected") != std::string::npos),
          "the refusal says why: " + (result.empty() ? std::string{} : result[0].message));
    check(fs::exists(changedFile) && slurp(changedFile) == changedBytes, "the changed copy is untouched");
    check(changedBytes != originalBytes, "the test really changed the file");

    // Became the newest between review and confirm (the newer copy was removed elsewhere): refused.
    {
        std::ofstream restore(changedFile, std::ios::binary | std::ios::trunc);
        restore.write(reinterpret_cast<const char*>(originalBytes.data()),
                      static_cast<std::streamsize>(originalBytes.size()));
    }
    review = RMF::enumerate(request);
    const int target2 = indexOf(review, blackBackups[2]);
    selection = RM::emptySelection(review.items.size());
    check(target2 >= 0 && RM::toggle(selection, review.verdicts, static_cast<std::size_t>(target2)),
          "selectable again after restore");
    const RM::Plan two = RM::buildPlan(review.items, review.verdicts, selection);
    fs::remove_all(third.recordDir);
    result = RMF::removePlanned(request, review, two);
    check(result.size() == 1 && !result[0].removed && result[0].message.find("protected") != std::string::npos,
          "a copy that became the newest is refused at confirm");
    check(fs::exists(fs::path(blackBackups[2]) / "save.sav"), "the now-newest copy stays");

    // A forged plan entry with ".." that escapes to the decoy is refused and the decoy stays.
    RMF::Listing forged;
    RM::Item escape;
    escape.kind = RM::Kind::FailedWrite;
    escape.path = manual + "/../../../outside/decoy.failed.0";
    escape.members = {escape.path};
    forged.items = {escape};
    forged.verdicts = {RM::Verdict{RM::Status::Removable, "forged"}};
    RM::Plan forgedPlan;
    forgedPlan.items = {0};
    result = RMF::removePlanned(request, forged, forgedPlan);
    check(result.size() == 1 && !result[0].removed, "a forged '..' path is refused");
    check(slurp(outside / "decoy.failed.0") == std::vector<uint8_t>({'d', 'e', 'c', 'o', 'y', 'f'}),
          "the decoy behind '..' is untouched");
    // And the model refuses that path on its own.
    check(RM::classify(forged.items, request.roots)[0].status == RM::Status::Protected,
          "the model refuses the '..' path");
    // An index past the reviewed list is refused.
    RM::Plan stale;
    stale.items = {999};
    result = RMF::removePlanned(request, review, stale);
    check(result.size() == 1 && !result[0].removed, "a stale index is refused");

    std::error_code ec;
    fs::remove_all(base, ec);
    check(!fs::exists(base), "temporary directory cleaned up");
    std::cout << "Recovery management (file system, mkdtemp tree): " << checks << " checks, " << failures
              << " failure(s).\n";
    return failures == 0 ? 0 : 1;
}
