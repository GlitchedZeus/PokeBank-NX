// Round 7 (7E G1, owner Round 6 C12): "after exporting an edited Gen V save, the exported copy does
// not appear in Save Backups, unlike the other generations". Gen V exports are written by the real
// Integration/Gen5/Gen5WorkingCopyExport under exports/gen5/account-<profile>/<gameId>_<timestamp>/
// (EXPORT_MANIFEST.txt); SourceBackups now lists, opens and deletes them for the CURRENT profile only.
// Synthetic saves only (tests/fuzz_synthetic_saves.h qasynth::gen5). Checked here:
//   valid records listed (app backup + exported copy, newest first, game + timestamp), duplicate
//   timestamps as separate records, malformed / pending / foreign-profile records never listed, a
//   missing file listed as missing and refused on open, legacy (pre-.dsv) manifests still listed and
//   opened, the older exports/gen5/<record> EDIT_MANIFEST layout unchanged; A opens the exact export
//   through openSnapshot + openBackupTrainer with the staged edit present; delete removes exactly one
//   record of this profile and refuses another profile's, foreign files and the last copy; the original
//   source and every other record (and .pending / .previous / .failed / .incomplete folders) stay
//   byte-identical.
#include "fuzz_synthetic_saves.h"

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"
#include "Legacy/BackupReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Source/ImmutableSourceSnapshot.h"
#include "UI/SourceBackupOpen.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace SB = PokeBank::UIModel::SourceBackups;
namespace S = PokeVault::Source::ImmutableSnapshot;
namespace L = PokeVault::Legacy;
namespace G5 = PokeVault::Integration::Gen5;

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
void write(const fs::path& path, const std::vector<uint8_t>& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}
void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}
// Every file under `root` with its bytes: proves "untouched" for the whole export tree at once.
std::map<std::string, std::vector<uint8_t>> tree(const fs::path& root) {
    std::map<std::string, std::vector<uint8_t>> out;
    if (!fs::exists(root)) return out;
    for (const auto& e : fs::recursive_directory_iterator(root))
        out[e.path().string()] = e.is_regular_file() ? slurp(e.path()) : std::vector<uint8_t>{};
    return out;
}

constexpr const char* kGame = "black_nds";
const std::string kProfile = "0123456789abcdef0123456789abcdef";
const std::string kOther = "fedcba9876543210fedcba9876543210";

// One real Gen V export: open `source` read-only, stage Money, publish through Gen5WorkingCopyExport.
std::string exportOnce(const fs::path& exportsRoot, const fs::path& source, const std::string& profile,
                       const char* timestamp, uint32_t money) {
    S::File file;
    std::string error;
    check(S::read(source.string(), source.parent_path().string(), {}, file, error), "read source: " + error);
    auto trainer = L::openBackupTrainer(file, kGame, error);
    auto* t = dynamic_cast<L::Gen5ReadOnlyTrainer*>(trainer.get());
    check(t != nullptr, "open source as Gen V: " + error);
    if (!t) return {};
    auto& ws = t->stagedPokemon();
    auto* trainerEdit = ws.stagedTrainer(&error);
    check(trainerEdit && trainerEdit->setMoney(money, &error), "stage Money: " + error);
    G5::WorkingCopyExportRequest request;
    request.rootDirectory = (exportsRoot / "gen5").string();
    request.profileIdentity = profile;
    request.timestamp = timestamp;
    request.sourcePath = source.string();
    request.observedSourceBytes = file.bytes;
    request.applicationSha = "host";
    const auto result = G5::publishGen5WorkingCopy(ws, request);
    check(result.success, std::string("Gen V export ") + timestamp + ": " + result.error);
    return result.directory;
}

std::size_t count(const SB::Listing& l, SB::Kind kind) {
    return static_cast<std::size_t>(std::count_if(l.entries.begin(), l.entries.end(),
                                                   [&](const SB::Entry& e) { return e.kind == kind; }));
}
int indexOf(const SB::Listing& l, const std::string& dir, SB::Kind kind) {
    for (std::size_t i = 0; i < l.entries.size(); ++i)
        if (l.entries[i].recordDir == dir && l.entries[i].kind == kind) return static_cast<int>(i);
    return -1;
}
bool listed(const SB::Listing& l, const std::string& dir) {
    return std::any_of(l.entries.begin(), l.entries.end(), [&](const SB::Entry& e) { return e.recordDir == dir; });
}
// A opens the row exactly as Save Backups does: openSnapshot, then openBackupTrainer.
std::unique_ptr<Trainer::Trainer> openRow(const SB::Listing& l, int index, std::string& error) {
    S::File file;
    if (index < 0 || !SB::openSnapshot(l, static_cast<std::size_t>(index), kGame, file, error)) return {};
    return L::openBackupTrainer(file, kGame, error);
}
} // namespace

int main() {
    char pattern[] = "/tmp/pokebank-gen5-backups-XXXXXX";
    const char* made = ::mkdtemp(pattern);
    if (!made) return EXIT_FAILURE;
    const fs::path root = made;
    const fs::path exportsRoot = root / "exports";
    fs::create_directories(exportsRoot);
    const fs::path source = root / "melonds" / "saves" / "Pokemon Black.sav";
    const fs::path otherSource = root / "melonds" / "saves" / "Pokemon Black (other).sav";
    const auto bytes = qasynth::gen5::save(qasynth::gen5::Family::BlackWhite, 21);
    write(source, bytes);
    write(otherSource, qasynth::gen5::save(qasynth::gen5::Family::BlackWhite, 21, 1, 0x0A0B0C0Du));
    std::string error;
    S::File sourceSnapshot;
    check(S::read(source.string(), source.parent_path().string(), {}, sourceSnapshot, error), "source snapshot");
    uint32_t originalMoney = 0;
    if (auto t = L::openBackupTrainer(sourceSnapshot, kGame, error)) originalMoney = t->money;

    // Real records: two for this profile (the second at the SAME timestamp: a "-2" duplicate record),
    // a newer one, and one for another profile.
    const std::string first = exportOnce(exportsRoot, source, kProfile, "20261026_090000", 111111);
    const std::string duplicate = exportOnce(exportsRoot, source, kProfile, "20261026_090000", 222222);
    const std::string newest = exportOnce(exportsRoot, source, kProfile, "20261026_100000", 333333);
    const std::string foreign = exportOnce(exportsRoot, source, kOther, "20261026_110000", 444444);
    const std::string foreignOther = exportOnce(exportsRoot, otherSource, kOther, "20261026_110000", 555555);
    const fs::path account = exportsRoot / "gen5" / ("account-" + kProfile);
    check(fs::path(first).parent_path() == account && fs::path(foreign).parent_path() != account,
          "exports land in their profile's account folder");
    check(first != duplicate && fs::path(duplicate).filename().string().find("-2") != std::string::npos,
          "a same-timestamp export is a separate -2 record");

    // Legacy (pre-.dsv) manifest: no SOURCE_CONTAINER / ORIGINAL_BACKUP_SHA256 lines, same layout.
    const fs::path legacy = account / "black_nds_20261020_080000";
    fs::create_directories(legacy);
    fs::copy_file(fs::path(first) / "original_backup.sav", legacy / "original_backup.sav");
    fs::copy_file(fs::path(first) / "edited.sav", legacy / "edited.sav");
    {
        std::ifstream in(fs::path(first) / "EXPORT_MANIFEST.txt");
        std::string line, out;
        while (std::getline(in, line))
            if (line.rfind("SOURCE_CONTAINER", 0) != 0 && line.rfind("SOURCE_FILE_", 0) != 0 &&
                line.rfind("ORIGINAL_BACKUP_SHA256", 0) != 0 && line.rfind("ORIGINAL_BACKUP_IS", 0) != 0 &&
                line.rfind("EDITED_CONTAINER", 0) != 0 && line.rfind("SOURCE_SHA256_SCOPE", 0) != 0)
                out += (line.rfind("TIMESTAMP=", 0) == 0 ? "TIMESTAMP=20261020_080000" : line) + "\n";
        writeText(legacy / "EXPORT_MANIFEST.txt", out);
    }
    // Malformed: no SOURCE_PATH. Missing: the edited file is gone. Pending: the writer's reservation.
    const fs::path malformed = account / "black_nds_20261021_080000";
    writeText(malformed / "EXPORT_MANIFEST.txt", "GAME_ID=black_nds\nTIMESTAMP=20261021_080000\n"
                                                 "ORIGINAL_BACKUP=original_backup.sav\nEDITED_SAVE=edited.sav\n");
    write(malformed / "edited.sav", bytes);
    const fs::path missing = account / "black_nds_20261022_080000";
    fs::create_directories(missing);
    fs::copy_file(fs::path(first) / "original_backup.sav", missing / "original_backup.sav");
    {
        std::ifstream in(fs::path(first) / "EXPORT_MANIFEST.txt");
        std::string line, out;
        while (std::getline(in, line))
            out += (line.rfind("TIMESTAMP=", 0) == 0 ? "TIMESTAMP=20261022_080000" : line) + "\n";
        writeText(missing / "EXPORT_MANIFEST.txt", out);
    }
    const fs::path pending = account / ".black_nds_20261026_120000.pending";
    fs::create_directories(pending);
    fs::copy_file(fs::path(first) / "EXPORT_MANIFEST.txt", pending / "EXPORT_MANIFEST.txt");
    // Recovery generations next to the records: never listed, never removed (no auto-delete).
    for (const char* kept : {"black_nds_20261019_080000.previous.1", "black_nds_20261019_080000.failed.1",
                             "black_nds_20261019_080000.incomplete.1"})
        writeText(account / kept / "note.bin", "recovery evidence");
    // The older generic layout exports/gen5/<record>/EDIT_MANIFEST.txt still lists (unchanged path).
    PokeVault::Integration::Common::VerifiedExportRequest generic;
    generic.rootDirectory = (exportsRoot / "gen5").string();
    generic.directoryStem = "generic";
    generic.generationLabel = "Generation 5";
    generic.fileExtension = ".sav";
    generic.sourcePath = source.string();
    generic.gameId = kGame;
    generic.saveFormat = "SYNTHETIC";
    generic.timestamp = "20261018_080000";
    generic.applicationSha = "host";
    generic.originalBytes = bytes;
    auto genericEdited = bytes;
    genericEdited[0x20] ^= 1;   // a change to export; this layout's validator is the test's own
    generic.editedBytes = genericEdited;
    generic.changeLines = {"host"};
    generic.validator = [](std::span<const uint8_t>, std::string&) { return true; };
    const auto genericResult = PokeVault::Integration::Common::publishVerifiedStagedExport(generic);
    check(genericResult.success, "generic gen5 export record: " + genericResult.error);

    const auto treeBefore = tree(exportsRoot);
    const auto sourceBefore = slurp(source);

    // ---- Listing ----
    const SB::Listing withProfile = SB::list(exportsRoot.string(), source.string(), {}, kProfile);
    check(withProfile.profileIdentity == kProfile, "the listing records its profile");
    for (const auto& dir : {first, duplicate, newest, legacy.string(), missing.string()})
        check(listed(withProfile, dir), "listed for this profile: " + dir);
    check(!listed(withProfile, foreign) && !listed(withProfile, foreignOther),
          "another profile's Gen V exports are never listed");
    check(!listed(withProfile, malformed.string()), "a malformed record (no SOURCE_PATH) is not listed");
    check(!listed(withProfile, pending.string()), "a .pending reservation is not listed");
    for (const auto& e : withProfile.entries)
        check(e.recordDir.find(".previous") == std::string::npos && e.recordDir.find(".failed") == std::string::npos &&
                  e.recordDir.find(".incomplete") == std::string::npos,
              "recovery generations are not listed");
    check(listed(withProfile, genericResult.directory), "the older exports/gen5/<record> layout still lists");
    // 5 profile records + 1 generic record, each an app backup and an exported copy.
    check(count(withProfile, SB::Kind::AppBackup) == 6 && count(withProfile, SB::Kind::ExportedCopy) == 6,
          "two rows per listed record");
    const int newestCopy = indexOf(withProfile, newest, SB::Kind::ExportedCopy);
    check(newestCopy == 1 || newestCopy == 2, "the newest export leads the history");
    for (const auto& e : withProfile.entries)
        if (e.profileScoped)
            check(e.gameId == kGame && (e.timestamp.size() == 15) && e.timestamp[8] == '_',
                  "a Gen V row carries its game and export date: " + e.timestamp);
    const int missingCopy = indexOf(withProfile, missing.string(), SB::Kind::ExportedCopy);
    check(missingCopy >= 0 && !withProfile.entries[static_cast<std::size_t>(missingCopy)].exists,
          "a record whose file is gone lists that row as missing");
    // Without a profile (or with an invalid one) nothing from an account folder is read.
    const SB::Listing noProfile = SB::list(exportsRoot.string(), source.string());
    check(!listed(noProfile, first) && listed(noProfile, genericResult.directory),
          "no profile: no account-folder records, the generic layout unchanged");
    check(!listed(SB::list(exportsRoot.string(), source.string(), {}, "../" + kOther), foreign),
          "an invalid profile string reads nothing");
    // historicalSources: this profile's Gen V source paths, never another profile's.
    const auto history = SB::historicalSources(exportsRoot.string(), {}, kGame, kProfile);
    check(std::find(history.begin(), history.end(), source.string()) != history.end(),
          "this profile's Gen V exports name their source");
    check(std::find(history.begin(), history.end(), otherSource.string()) == history.end(),
          "another profile's source path is not discovered");

    // ---- Open (A): the exact export, as an isolated staged session ----
    {
        std::string e;
        auto edited = openRow(withProfile, newestCopy, e);
        check(edited && edited->money == 333333, "A on the exported copy opens it with its staged edit (" + e + ")");
        if (auto* t = dynamic_cast<L::Gen5ReadOnlyTrainer*>(edited.get()))
            check(!t->hasStagedChanges(), "the opened export starts a fresh, empty staged session");
        auto backup = openRow(withProfile, indexOf(withProfile, newest, SB::Kind::AppBackup), e);
        check(backup && backup->money == originalMoney, "A on the app backup opens the original bytes");
        auto legacyCopy = openRow(withProfile, indexOf(withProfile, legacy.string(), SB::Kind::ExportedCopy), e);
        check(legacyCopy && legacyCopy->money == 111111, "a legacy Gen V export still opens (" + e + ")");
        check(!openRow(withProfile, missingCopy, e), "a missing file is refused on open");
        // A listing of another profile cannot open this profile's record through a forged index.
        SB::Listing forged = withProfile;
        forged.profileIdentity = kOther;
        check(!openRow(forged, newestCopy, e), "a record outside the listing's profile folder is refused on open");
    }
    check(slurp(source) == sourceBefore && S::unchanged(sourceSnapshot, error), "open leaves the original source untouched");
    check(tree(exportsRoot) == treeBefore, "listing and opening change no file under exports/");

    // ---- Delete (Y): exactly one record of this profile ----
    {
        SB::Listing forged = withProfile;
        forged.profileIdentity = kOther;
        check(SB::canDelete(forged, static_cast<std::size_t>(newestCopy)) == SB::Refusal::NotAnExportRecord,
              "a record outside the listing's profile folder is never deleted");
        SB::Listing escape = withProfile;
        escape.entries[static_cast<std::size_t>(newestCopy)].recordDir = foreign;
        check(SB::canDelete(escape, static_cast<std::size_t>(newestCopy)) == SB::Refusal::NotAnExportRecord,
              "an entry retargeted at another profile's record is refused");
        writeText(fs::path(newest) / "notes.txt", "x");
        check(SB::canDelete(withProfile, static_cast<std::size_t>(newestCopy)) == SB::Refusal::UnexpectedFiles,
              "a foreign file in a Gen V record refuses delete");
        fs::remove(fs::path(newest) / "notes.txt");
        check(SB::canDelete(withProfile, 0) == SB::Refusal::OriginalSource, "the original is never deletable");
        check(SB::canDelete(withProfile, static_cast<std::size_t>(newestCopy)) == SB::Refusal::None,
              "this profile's verified record may be deleted after Y");
        check(SB::deleteRecord(withProfile, static_cast<std::size_t>(newestCopy)) == SB::Refusal::None &&
                  !fs::exists(newest), "delete removes that record");
        auto after = tree(exportsRoot);
        auto expected = treeBefore;
        for (auto it = expected.begin(); it != expected.end();)
            it = it->first.rfind(newest, 0) == 0 ? expected.erase(it) : std::next(it);
        check(after == expected, "nothing else under exports/ changed (other records, other profile, "
                                 "pending and recovery folders)");
        check(slurp(source) == sourceBefore && S::unchanged(sourceSnapshot, error), "delete leaves the source untouched");
    }
    // The original gone: the last app backup of this profile's history is kept.
    {
        fs::remove(source);
        SB::Listing gone = SB::list(exportsRoot.string(), source.string(), {}, kProfile);
        for (const auto& dir : {first, duplicate, missing.string(), legacy.string()}) {
            const int i = indexOf(gone, dir, SB::Kind::AppBackup);
            if (i >= 0 && dir != legacy.string()) (void)SB::deleteRecord(gone, static_cast<std::size_t>(i));
            gone = SB::list(exportsRoot.string(), source.string(), {}, kProfile);
        }
        (void)SB::deleteRecord(gone, static_cast<std::size_t>(indexOf(gone, genericResult.directory, SB::Kind::AppBackup)));
        gone = SB::list(exportsRoot.string(), source.string(), {}, kProfile);
        const int last = indexOf(gone, legacy.string(), SB::Kind::AppBackup);
        check(last >= 0 && SB::canDelete(gone, static_cast<std::size_t>(last)) == SB::Refusal::OnlyRemainingCopy,
              "without the original, the last remaining Gen V backup is never deleted");
        check(fs::exists(foreign) && fs::exists(foreignOther), "another profile's records survive everything");
        for (const char* kept : {"black_nds_20261019_080000.previous.1", "black_nds_20261019_080000.failed.1",
                                 "black_nds_20261019_080000.incomplete.1"})
            check(fs::exists(account / kept / "note.bin"), std::string("recovery folder kept: ") + kept);
        check(fs::exists(pending / "EXPORT_MANIFEST.txt"), "the .pending reservation is kept");
    }
    fs::remove_all(root);
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Gen V exports in Save Backups: listed / opened / deleted for the current profile only (" << checks
              << " checks): PASS\n";
    return EXIT_SUCCESS;
}
