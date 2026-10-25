// Round 2 phase 7: "+ -> Backups" for emulator saves. Export records are written by the real
// VerifiedStagedExport (synthetic bytes only) for RetroArch (.srm), DraStic (.dsv) and melonDS
// (.sav) sources, then classified and deleted through SourceBackupClassification.h:
// - the original emulator file is read-only and never deletable, and is byte-identical afterwards;
// - each record shows an app backup and an exported copy, found only by its exact SOURCE_PATH;
// - deletion removes exactly one whole record and nothing else;
// - refused: foreign files in a record, a record outside exports/genN, the staged-edits row, and
//   the last remaining app backup when the emulator's own file is gone.
#include "Integration/Common/VerifiedStagedExport.h"
#include "UI/SourceBackupClassification.h"
#include "UI/WorkingCopyRefresh.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace B = PokeBank::UIModel::SourceBackups;
namespace C = PokeVault::Integration::Common;

namespace {

std::vector<uint8_t> slurp(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void write(const fs::path& path, const std::vector<uint8_t>& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

struct Emulator {
    const char* name;
    const char* sourceFile; // relative to the fake emulator save folder
    int gen;
    const char* gameId;
    const char* ext;
};

// One verified export of `source` into exports/gen<gen>, exactly as an editor session publishes it.
std::string exportOnce(const fs::path& exportsRoot, const Emulator& emu, const fs::path& source,
                       const char* stem, const char* timestamp, uint8_t marker) {
    const auto original = slurp(source);
    auto edited = original;
    edited[0x20] = marker;
    C::VerifiedExportRequest r;
    fs::create_directories(exportsRoot); // the app creates exports/ at start-up (PokeBankPaths)
    r.rootDirectory = (exportsRoot / ("gen" + std::to_string(emu.gen))).string();
    r.directoryStem = stem;
    r.generationLabel = "Generation " + std::to_string(emu.gen);
    r.fileExtension = emu.ext;
    r.sourcePath = source.string();
    r.gameId = emu.gameId;
    r.saveFormat = "SYNTHETIC";
    r.timestamp = timestamp;
    r.applicationSha = "host";
    r.originalBytes = original;
    r.editedBytes = edited;
    r.changeLines = {"Box 1 slot 1: edited"};
    r.validator = [n = original.size()](std::span<const uint8_t> b, std::string& e) {
        if (b.size() != n) { e = "size"; return false; }
        return true;
    };
    const auto result = C::publishVerifiedStagedExport(r);
    if (!result.success) std::cerr << "export failed: " << result.error << "\n";
    assert(result.success);
    return result.directory;
}

std::size_t countKind(const B::Listing& l, B::Kind k) {
    std::size_t n = 0;
    for (const auto& e : l.entries) n += e.kind == k;
    return n;
}

void perEmulator(const fs::path& root, const Emulator& emu) {
    const fs::path exportsRoot = root / "exports";
    const fs::path source = root / emu.name / emu.sourceFile;
    const fs::path otherSource = root / emu.name / "other-game.sav";
    write(source, std::vector<uint8_t>(0x400, 0x5A));
    write(otherSource, std::vector<uint8_t>(0x400, 0x33));
    const auto sourceBefore = slurp(source);

    const std::string older = exportOnce(exportsRoot, emu, source, emu.name, "20261016_090000", 1);
    const std::string newer = exportOnce(exportsRoot, emu, source, emu.name, "20261017_090000", 2);
    const std::string unrelated = exportOnce(exportsRoot, emu, otherSource, "other", "20261017_100000", 3);

    // Classification: source first (read-only, never deletable), records newest first, staged last.
    B::Listing l = B::list(exportsRoot.string(), source.string());
    assert(l.sourceExists);
    assert(l.entries.front().kind == B::Kind::OriginalSource && l.entries.front().path == source.string());
    assert(l.entries.back().kind == B::Kind::StagedWorkingCopy);
    assert(countKind(l, B::Kind::AppBackup) == 2 && countKind(l, B::Kind::ExportedCopy) == 2);
    assert(l.entries[1].recordDir == newer && l.entries[1].kind == B::Kind::AppBackup);
    assert(l.entries[2].recordDir == newer && l.entries[2].kind == B::Kind::ExportedCopy);
    assert(l.entries[3].recordDir == older);
    for (const auto& e : l.entries) assert(e.recordDir != unrelated); // exact SOURCE_PATH only
    assert(B::policyFor(B::Kind::OriginalSource).readOnly && !B::policyFor(B::Kind::OriginalSource).deletable);
    assert(B::canDelete(l, 0) == B::Refusal::OriginalSource);
    assert(B::canDelete(l, l.entries.size() - 1) == B::Refusal::StagedWorkingCopy);
    assert(B::deleteRecord(l, 0) == B::Refusal::OriginalSource);
    assert(slurp(source) == sourceBefore);

    // A file PokeBank NX did not write makes the record ambiguous: refused, untouched.
    write(fs::path(newer) / "notes.txt", {'x'});
    assert(B::canDelete(l, 1) == B::Refusal::UnexpectedFiles);
    assert(B::deleteRecord(l, 2) == B::Refusal::UnexpectedFiles && fs::exists(newer));
    fs::remove(fs::path(newer) / "notes.txt");

    // Deleting via the exported-copy row removes that whole record and nothing else.
    assert(B::canDelete(l, 2) == B::Refusal::None);
    assert(B::deleteRecord(l, 2) == B::Refusal::None);
    assert(!fs::exists(newer) && fs::exists(older) && fs::exists(unrelated));
    assert(slurp(source) == sourceBefore);
    l = B::list(exportsRoot.string(), source.string());
    assert(countKind(l, B::Kind::AppBackup) == 1);
    assert(B::deleteRecord(l, 2) == B::Refusal::None); // stale index into a fresh listing is still exact
    assert(!fs::exists(older));

    // Emulator file gone: the last remaining app backup of that save is never deleted.
    const std::string last = exportOnce(exportsRoot, emu, source, emu.name, "20261017_120000", 4);
    const auto beforeRemoval = B::list(exportsRoot.string(), source.string());
    fs::remove(source);
    assert(B::canDelete(beforeRemoval, 1) == B::Refusal::OnlyRemainingCopy); // Recheck live source at confirmation.
    l = B::list(exportsRoot.string(), source.string());
    assert(!l.sourceExists && !l.entries.front().exists);
    assert(B::canDelete(l, 1) == B::Refusal::OnlyRemainingCopy);
    assert(B::deleteRecord(l, 1) == B::Refusal::OnlyRemainingCopy && fs::exists(last));
    // With a second backup present, one of the two may go.
    write(source, sourceBefore);
    const std::string second = exportOnce(exportsRoot, emu, source, emu.name, "20261017_130000", 5);
    fs::remove(source);
    l = B::list(exportsRoot.string(), source.string());
    assert(B::canDelete(l, 1) == B::Refusal::None);
    assert(B::deleteRecord(l, 1) == B::Refusal::None && !fs::exists(second) && fs::exists(last));
    l = B::list(exportsRoot.string(), source.string());
    assert(B::canDelete(l, 1) == B::Refusal::OnlyRemainingCopy);
    write(source, sourceBefore);
}

void forgedRecordsAreRefused(const fs::path& root) {
    const fs::path exportsRoot = root / "exports";
    const fs::path source = root / "forged" / "save.sav";
    write(source, std::vector<uint8_t>(0x100, 0x11));
    // A manifest outside exports/genN that names the source: listed nowhere, deletable never.
    const fs::path outside = root / "elsewhere" / "rec";
    write(outside / "EDIT_MANIFEST.txt",
          {'S','O','U','R','C','E','_','P','A','T','H','='});
    B::Listing l = B::list(exportsRoot.string(), source.string());
    assert(l.entries.size() == 2); // source + staged only
    B::Entry fake{B::Kind::AppBackup, (outside / "original_backup.sav").string(), outside.string(), "", "", true};
    l.entries.insert(l.entries.begin() + 1, fake);
    assert(B::canDelete(l, 1) == B::Refusal::NotAnExportRecord);
    assert(B::deleteRecord(l, 1) == B::Refusal::NotAnExportRecord && fs::exists(outside));
    // A path-escaping record name is never treated as a record.
    B::Entry escape{B::Kind::AppBackup, "", (exportsRoot / "gen1" / "..").string(), "", "", true};
    l.entries[1] = escape;
    assert(B::canDelete(l, 1) == B::Refusal::NotAnExportRecord);
    // Reasons are precise and non-empty for every refusal.
    for (auto r : {B::Refusal::OriginalSource, B::Refusal::StagedWorkingCopy, B::Refusal::NotAnExportRecord,
                   B::Refusal::UnexpectedFiles, B::Refusal::OnlyRemainingCopy, B::Refusal::Missing})
        assert(std::string(B::refusalReason(r)).size() > 20);
}

// Round 4 (R2-EMUBACKUP / C07 / S05): deliberate, copy-only backups made from the Backups screen.
void deliberateBackups(const fs::path& root, const Emulator& emu) {
    const fs::path exportsRoot = root / "exports";
    const std::string manualRoot = B::emulatorBackupsRoot((root / "backups").string());
    const fs::path source = root / ("deliberate-" + std::string(emu.name)) / emu.sourceFile;
    std::vector<uint8_t> bytes(0x80000 + 0x7A);
    for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<uint8_t>(i * 31 + 7);
    write(source, bytes);
    const auto sourceTime = fs::last_write_time(source);

    // Opening / listing never creates a backup: nothing appears until the user asks.
    B::Listing l = B::list(exportsRoot.string(), source.string(), manualRoot);
    assert(countKind(l, B::Kind::ManualBackup) == 0);
    assert(!fs::exists(fs::path(manualRoot) / emu.gameId));

    // Create: an exact copy in <backups>/emulator-saves/<gameId>/<timestamp>/save<ext> + manifest.
    const B::CreateResult first = B::createBackup(manualRoot, emu.gameId, source.string(), "20261020_101500");
    if (!first.ok) std::cerr << first.error << "\n";
    assert(first.ok && first.error.empty());
    assert(fs::path(first.recordDir) == fs::path(manualRoot) / emu.gameId / "20261020_101500");
    assert(fs::path(first.backupPath).filename() == std::string("save") + emu.ext);
    assert(slurp(first.backupPath) == bytes);
    assert(fs::exists(fs::path(first.recordDir) / "BACKUP_MANIFEST.txt"));
    // Same second: a distinct record, never an overwrite.
    const B::CreateResult second = B::createBackup(manualRoot, emu.gameId, source.string(), "20261020_101500");
    assert(second.ok && second.recordDir != first.recordDir && slurp(first.backupPath) == bytes);
    const B::CreateResult newest = B::createBackup(manualRoot, emu.gameId, source.string(), "20261020_111500");
    assert(newest.ok);

    // The original emulator file is byte-identical and was never rewritten.
    assert(slurp(source) == bytes && fs::last_write_time(source) == sourceTime);

    // List: source first, then this save's backups newest first; staged last.
    l = B::list(exportsRoot.string(), source.string(), manualRoot);
    assert(countKind(l, B::Kind::ManualBackup) == 3);
    assert(l.entries.front().kind == B::Kind::OriginalSource);
    assert(l.entries[1].kind == B::Kind::ManualBackup && l.entries[1].recordDir == newest.recordDir);
    assert(l.entries.back().kind == B::Kind::StagedWorkingCopy);
    assert(B::policyFor(B::Kind::ManualBackup).deletable && !B::policyFor(B::Kind::ManualBackup).readOnly);
    // Without manualRoot (older callers) manual backups are simply not listed.
    assert(countKind(B::list(exportsRoot.string(), source.string()), B::Kind::ManualBackup) == 0);
    const std::string exported = exportOnce(exportsRoot, emu, source, emu.name, "20261020_115500", 0x99);
    const auto mixed = B::list(exportsRoot.string(), source.string(), manualRoot);
    assert(mixed.entries[1].recordDir == exported); // Export and manual backups share newest-first order.
    assert(mixed.entries[2].recordDir == exported);
    assert(mixed.entries[3].recordDir == newest.recordDir);
    assert(B::deleteRecord(mixed, 1) == B::Refusal::None);
    // Another save's backup is never listed here.
    const fs::path other = root / ("deliberate-" + std::string(emu.name)) / "other.sav";
    write(other, std::vector<uint8_t>(0x200, 0x44));
    assert(B::createBackup(manualRoot, emu.gameId, other.string(), "20261020_121500").ok);
    assert(countKind(B::list(exportsRoot.string(), source.string(), manualRoot), B::Kind::ManualBackup) == 3);

    // Delete (after Y): exactly that record; the others and the source stay.
    assert(B::canDelete(l, 1) == B::Refusal::None);
    assert(B::deleteRecord(l, 1) == B::Refusal::None);
    assert(!fs::exists(newest.recordDir) && fs::exists(first.recordDir) && fs::exists(second.recordDir));
    assert(slurp(source) == bytes && fs::last_write_time(source) == sourceTime);

    // A file PokeBank NX did not write: refused, untouched.
    l = B::list(exportsRoot.string(), source.string(), manualRoot);
    write(fs::path(l.entries[1].recordDir) / "notes.txt", {'x'});
    assert(B::canDelete(l, 1) == B::Refusal::UnexpectedFiles);
    assert(B::deleteRecord(l, 1) == B::Refusal::UnexpectedFiles && fs::exists(l.entries[1].recordDir));
    fs::remove(fs::path(l.entries[1].recordDir) / "notes.txt");

    // A forged entry outside <backups>/emulator-saves/<game>/<record> is never deletable.
    B::Listing forged = l;
    forged.entries[1].recordDir = (root / "elsewhere").string();
    assert(B::canDelete(forged, 1) == B::Refusal::NotAnExportRecord);
    forged.entries[1].recordDir = manualRoot + "/" + emu.gameId;   // the game folder itself
    assert(B::canDelete(forged, 1) == B::Refusal::NotAnExportRecord);

    // Emulator file gone: the last remaining copy is kept.
    fs::remove(source);
    l = B::list(exportsRoot.string(), source.string(), manualRoot);
    assert(countKind(l, B::Kind::ManualBackup) == 2);
    assert(B::deleteRecord(l, 1) == B::Refusal::None);
    l = B::list(exportsRoot.string(), source.string(), manualRoot);
    assert(B::canDelete(l, 1) == B::Refusal::OnlyRemainingCopy);
    assert(B::deleteRecord(l, 1) == B::Refusal::OnlyRemainingCopy && fs::exists(l.entries[1].recordDir));

    // Creating from a missing source, or with an unsafe game id, makes nothing.
    const auto before = std::distance(fs::recursive_directory_iterator(manualRoot), {});
    const B::CreateResult missing = B::createBackup(manualRoot, emu.gameId, source.string(), "20261020_131500");
    assert(!missing.ok && !missing.error.empty());
    write(source, bytes);
    assert(!B::createBackup(manualRoot, "../escape", source.string(), "20261020_131500").ok);
    assert(!B::createBackup(manualRoot, emu.gameId, source.string(), "../x").ok);
    assert(!B::createBackup("", emu.gameId, source.string(), "20261020_131500").ok);
    assert(std::distance(fs::recursive_directory_iterator(manualRoot), {}) == before);
    assert(slurp(source) == bytes);
}

// Round 4 (S05 / R2-EMUBACKUP): opening an installed Switch save refreshes the single Working copy;
// it never adds a backup unless the earlier copy held edits, and never loses those edits.
void workingCopyRefresh(const fs::path& root) {
    namespace W = PokeBank::UIModel::WorkingCopy;
    const fs::path game = root / "switch-backups" / "profile" / "sword";
    const fs::path working = game / "Working";
    const std::vector<uint8_t> installed(0x1000, 0x21);
    // The installed save as backupSaveData(..., false) would copy it.
    const auto makeFresh = [&]() {
        write(working / "main", installed);
        write(working / "backup" / "poke_trade", {9, 9});
        return working.string();
    };
    const auto visibleBackups = [&]() {
        std::size_t n = 0;
        for (const auto& e : fs::directory_iterator(game)) {
            const std::string name = e.path().filename().string();
            if (name != "Working" && name.find(".previous.") == std::string::npos) ++n;
        }
        return n;
    };

    // First open: a fresh Working copy, no backup.
    W::Result r = W::refresh(working.string(), "20261020_100000", makeFresh);
    assert(r.outcome == W::Outcome::Fresh && r.workingPath == working.string() && visibleBackups() == 0);

    // Re-open with an unchanged save: still no backup, nothing left behind.
    r = W::refresh(working.string(), "20261020_100100", makeFresh);
    assert(r.outcome == W::Outcome::UnchangedDropped && visibleBackups() == 0);
    assert(std::distance(fs::directory_iterator(game), {}) == 1);

    // The earlier Working copy holds edits: kept as a dated backup, the fresh copy is the save.
    write(working / "main", std::vector<uint8_t>(0x1000, 0x77));
    r = W::refresh(working.string(), "20261020_100200", makeFresh);
    assert(r.outcome == W::Outcome::EditsPreserved && visibleBackups() == 1);
    assert(fs::path(r.preservedPath) == game / "20261020_100200");
    assert(slurp(fs::path(r.preservedPath) / "main") == std::vector<uint8_t>(0x1000, 0x77));
    assert(slurp(working / "main") == installed);
    // Same stamp again with edits: a distinct folder, never an overwrite.
    write(working / "main", std::vector<uint8_t>(0x1000, 0x55));
    r = W::refresh(working.string(), "20261020_100200", makeFresh);
    assert(r.outcome == W::Outcome::EditsPreserved && fs::path(r.preservedPath) == game / "20261020_100200-2");
    assert(slurp(game / "20261020_100200" / "main") == std::vector<uint8_t>(0x1000, 0x77));

    // A fresh copy that fails puts the earlier Working copy back exactly.
    write(working / "main", std::vector<uint8_t>(0x1000, 0x66));
    r = W::refresh(working.string(), "20261020_100300", [] { return std::string(); });
    assert(r.outcome == W::Outcome::Failed && !r.error.empty());
    assert(slurp(working / "main") == std::vector<uint8_t>(0x1000, 0x66));
    for (const auto& e : fs::directory_iterator(game))
        assert(e.path().filename().string().find(".previous.") == std::string::npos);

    // SD-full/read failure after creating a partial destination still restores all earlier edits.
    r = W::refresh(working.string(), "20261020_100301", [&] {
        write(working / "main", {1, 2, 3});
        return std::string();
    });
    assert(r.outcome == W::Outcome::Failed && r.workingPath.empty());
    assert(slurp(working / "main") == std::vector<uint8_t>(0x1000, 0x66));
    assert(slurp(working / "backup" / "poke_trade") == std::vector<uint8_t>({9, 9}));
    // An unexpected callback destination is never opened or deleted; the old Working is restored.
    const fs::path foreign = root / "foreign";
    write(foreign / "main", {4, 5, 6});
    r = W::refresh(working.string(), "20261020_100302", [&] { return foreign.string(); });
    assert(r.outcome == W::Outcome::Failed && r.workingPath.empty());
    assert(slurp(foreign / "main") == std::vector<uint8_t>({4, 5, 6}));
    assert(slurp(working / "main") == std::vector<uint8_t>(0x1000, 0x66));

    // Tree comparison is exact: an extra file, or one changed byte, is a difference.
    write(root / "ta" / "x", {1, 2, 3});
    write(root / "tb" / "x", {1, 2, 3});
    assert(W::sameTree((root / "ta").string(), (root / "tb").string()));
    write(root / "tb" / "y", {});
    assert(!W::sameTree((root / "ta").string(), (root / "tb").string()));
    fs::remove(root / "tb" / "y");
    write(root / "tb" / "x", {1, 2, 4});
    assert(!W::sameTree((root / "ta").string(), (root / "tb").string()));
    // Unsafe inputs refuse without touching anything.
    assert(W::refresh("", "20261020_100400", makeFresh).outcome == W::Outcome::Failed);
    assert(W::refresh(working.string(), "../x", makeFresh).outcome == W::Outcome::Failed);
    assert(W::refresh(working.string(), "..", makeFresh).outcome == W::Outcome::Failed);
}


// Round 5 (#322): A on a Switch backup (owned or legacy) opens an isolated session copy in the
// exact profile/game folder; the backup itself, legacy or not, stays byte- and time-identical, and
// edits saved in an earlier session are kept as a dated backup instead of being overwritten.
void backupSessions(const fs::path& root) {
    namespace W = PokeBank::UIModel::WorkingCopy;
    const fs::path game = root / "sessions" / "profile" / "brilliant_diamond_switch";
    const fs::path owned = game / "20261020_100000";
    const fs::path legacy = root / "sessions" / "Pokemon Brilliant Diamond" / "20260921_101010";
    const std::vector<uint8_t> ownedMain(0x2000, 0x31), legacyMain(0x2000, 0x52);
    write(owned / "main", ownedMain);
    write(owned / "backup" / "poke", {1, 2});
    write(legacy / "main", legacyMain);
    write(legacy / "backup" / "poke", {3, 4});
    const auto ownedTime = fs::last_write_time(owned / "main");
    const auto legacyTime = fs::last_write_time(legacy / "main");
    int copies = 0;
    const auto copyTree = [&](const std::string& from, const std::string& to) {
        ++copies;
        fs::copy(from, to, fs::copy_options::recursive);
        return true;
    };
    const auto originals = [&] {
        assert(slurp(owned / "main") == ownedMain && slurp(owned / "backup" / "poke") == std::vector<uint8_t>({1, 2}));
        assert(slurp(legacy / "main") == legacyMain && slurp(legacy / "backup" / "poke") == std::vector<uint8_t>({3, 4}));
        assert(fs::last_write_time(owned / "main") == ownedTime && fs::last_write_time(legacy / "main") == legacyTime);
    };
    const fs::path session = game / W::kBackupSessionName;
    const auto datedFolders = [&] {
        std::size_t n = 0;
        for (const auto& e : fs::directory_iterator(game))
            if (e.is_directory() && e.path() != session && e.path() != owned) ++n;
        return n;
    };

    // Open the owned backup: a session copy, the backup untouched, no new backup.
    W::Result r = W::openBackupSession(game.string(), owned.string(), "20261024_100000", copyTree);
    assert(r.outcome == W::Outcome::Fresh && fs::path(r.workingPath) == session && r.error.empty());
    assert(slurp(session / "main") == ownedMain && W::readOrigin(session.string()) == owned.string());
    originals();
    // Open the legacy backup next (the earlier session held no edits): dropped, nothing new.
    r = W::openBackupSession(game.string(), legacy.string(), "20261024_100100", copyTree);
    assert(r.outcome == W::Outcome::UnchangedDropped && slurp(session / "main") == legacyMain);
    assert(datedFolders() == 0);
    originals();
    // Edits saved into the session, then another backup opened: the edits survive as a dated backup.
    write(session / "main", std::vector<uint8_t>(0x2000, 0x99));
    r = W::openBackupSession(game.string(), owned.string(), "20261024_100200", copyTree);
    assert(r.outcome == W::Outcome::EditsPreserved && fs::path(r.preservedPath) == game / "20261024_100200");
    assert(slurp(fs::path(r.preservedPath) / "main") == std::vector<uint8_t>(0x2000, 0x99));
    assert(slurp(session / "main") == ownedMain && datedFolders() == 1);
    originals();
    // A session with no origin record is kept, never dropped.
    fs::remove(W::originFileFor(session.string()));
    write(session / "main", ownedMain);
    r = W::openBackupSession(game.string(), owned.string(), "20261024_100300", copyTree);
    assert(r.outcome == W::Outcome::EditsPreserved && datedFolders() == 2);
    originals();
    // Refusals copy nothing: a missing backup, the session itself, an empty game folder.
    const int before = copies;
    assert(W::openBackupSession(game.string(), (game / "nope").string(), "20261024_100400", copyTree).outcome ==
           W::Outcome::Failed);
    assert(W::openBackupSession(game.string(), session.string(), "20261024_100400", copyTree).outcome ==
           W::Outcome::Failed);
    assert(W::openBackupSession("", owned.string(), "20261024_100400", copyTree).outcome == W::Outcome::Failed);
    assert(copies == before);
    // A failed copy restores the earlier session exactly.
    write(session / "main", std::vector<uint8_t>(0x2000, 0x11));
    r = W::openBackupSession(game.string(), legacy.string(), "20261024_100500",
                             [](const std::string&, const std::string&) { return false; });
    assert(r.outcome == W::Outcome::Failed && slurp(session / "main") == std::vector<uint8_t>(0x2000, 0x11));
    assert(W::readOrigin(session.string()) == owned.string());
    originals();
    // refresh() still rotates only Working and this session: never a backup folder itself.
    const auto never = [&] { ++copies; return std::string(); };
    assert(W::refresh(owned.string(), "20261024_100600", never).outcome == W::Outcome::Failed);
    assert(W::refresh(legacy.string(), "20261024_100600", never).outcome == W::Outcome::Failed);
    assert(copies == before);   // refused before any copy
    originals();
}
} // namespace

int main() {
    char pattern[] = "/tmp/pokebank_emu_backups_XXXXXX";
    assert(::mkdtemp(pattern));
    const fs::path root = pattern;
    perEmulator(root, {"retroarch", "saves/Pokemon - FireRed Version (USA).srm", 3, "firered_gba", ".srm"});
    perEmulator(root, {"drastic", "backup/POKEMON PL.dsv", 4, "platinum_nds", ".dsv"});
    perEmulator(root, {"melonds", "saves/Pokemon White 2.sav", 5, "white2_nds", ".sav"});
    forgedRecordsAreRefused(root);
    deliberateBackups(root, {"retroarch", "saves/Pokemon - FireRed Version (USA).srm", 3, "firered_gba", ".srm"});
    deliberateBackups(root, {"drastic", "backup/POKEMON B.dsv", 5, "black_nds", ".dsv"});
    deliberateBackups(root, {"melonds", "saves/Pokemon White 2.sav", 5, "white2_nds", ".sav"});
    workingCopyRefresh(root);
    backupSessions(root);
    fs::remove_all(root);
    std::cout << "Emulator backups: RetroArch / DraStic / melonDS sources read-only and unchanged, records by "
                 "exact SOURCE_PATH, whole-record deletion only, foreign files / escapes / last copy refused; deliberate "
                 "copy-only backups created / listed / deleted, none on open, source byte- and time-identical; Switch "
                 "Working copy refreshed without unnecessary backups, edits preserved, failures restore; backup sessions "
                 "leave owned and legacy backups byte-identical PASS\n";
}
