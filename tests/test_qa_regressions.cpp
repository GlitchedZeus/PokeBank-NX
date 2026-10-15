// PokeBank NX application QA: regression tests for QA findings that have been FIXED (Thread 4).
//
// Each case started life as a failing reproduction in qa-known-failures and moved here once the
// owning thread's fix landed. Run by `make qa-contracts` (GCC and Clang, plain + ASan/UBSan).
// Details: docs/qa/FINDINGS.md.
#include "test_qa_support.h"

#include "Integration/Common/ClassicStrictValidators.h"
#include "Integration/Common/VerifiedStagedExport.h"
#include "Encryption/Encryption5.h"
#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5GameCardPreview.h"
#include "Integration/Gen5/Gen5PersonalData.h"
#include "Integration/Gen5/Gen5PidCoordination.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"
#include "Legacy/LegacySourceBindings.h"
#include "Source/HomeCardSnapshot.h"
#include "Utils/CRC16.h"
#include "fuzz_synthetic_saves.h"

#include <iostream>
#include <span>

using namespace qacontract;
namespace Common = PokeVault::Integration::Common;
namespace G1 = PokeVault::Integration::Gen1;
namespace G5 = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace S = PokeVault::Source;

namespace {

// QA-F3 (Low, Thread 2: Integration/Common/VerifiedStagedExport.cpp; fixed in fa9cfcea).
// publishVerifiedStagedExport used to choose a destination name with stat() and publish much later
// with rename(2), which silently REPLACES an existing empty directory. A destination that appears
// between the uniqueness check and publication must now be left untouched (export fails closed or
// moves on). The window is widened deterministically through the caller's strict validator, which
// the exporter invokes after read-back; a racing empty directory AND a racing non-empty directory
// with a sentinel are both checked.
bool commonExportPublishIsExclusive(bool sentinel) {
    currentCase() = sentinel ? "QA-F3 racing non-empty destination" : "QA-F3 racing empty destination";
    const fs::path root = scratchRoot(sentinel ? "reg-f3-full" : "reg-f3-empty");
    auto original = qasynth::gen1::save(0x012345);
    auto edited = qasynth::gen1::save(0x054321);
    const fs::path destination = root / "red_gb_20261014_120000";
    int calls = 0;
    const auto strict = Common::gen1StrictValidator(G1::SourceGame::Red);

    Common::VerifiedExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = destination.filename().string();
    request.generationLabel = "Generation I";
    request.fileExtension = ".srm";
    request.gameId = "red_gb";
    request.timestamp = "20261014_120000";
    request.originalBytes = original;
    request.editedBytes = edited;
    request.validator = [&](std::span<const uint8_t> bytes, std::string& error) {
        // Second call = after the files were written and read back, before publication: another
        // writer creates the destination directory now.
        if (++calls == 2) {
            fs::create_directory(destination);
            if (sentinel) spit(destination / "edited.srm", Bytes{'o', 't', 'h', 'e', 'r'});
        }
        return strict(bytes, error);
    };
    const auto result = Common::publishVerifiedStagedExport(request);
    const bool replaced = result.success && result.directory == destination.string();
    QA_EXPECT(!replaced, "an existing destination directory was replaced; expected the export to fail closed "
                         "or to publish under a new unique name");
    if (sentinel)
        QA_EXPECT(slurp(destination / "edited.srm") == (Bytes{'o', 't', 'h', 'e', 'r'}),
                  "the racing writer's file was modified");
    // No temporary/pending directory or stray file may remain next to the racing destination.
    for (const auto& entry : fs::directory_iterator(root)) {
        const auto name = entry.path().filename().string();
        QA_EXPECT(name.find(".tmp") == std::string::npos && name.find(".pending") == std::string::npos,
                  "temporary directory left behind: " + name);
        if (!result.success) QA_EXPECT(entry.path() == destination, "unexpected entry after refusal: " + name);
    }
    fs::remove_all(root);
    return !replaced;
}

// QA-F4 (Low, Thread 2R: include/Integration/Gen5/Gen5StagedPokemonRecord.h, Field::Personality;
// fixed in bc83815, which checks each PID link independently).
// The PID replay guard treats "gender link" and "ability link" as ONE condition: a record is
// protected only when BOTH already hold. A Gen V origin record whose stored gender matches its PID
// but whose ability byte does not match its PID ability slot therefore gets no gender protection,
// and a replayed PID may silently desynchronise stored gender from the PID (PKHeX
// EntityGender.GetFromPID). Found by fuzz_gen5_edits (libFuzzer, 2026-10-15).
bool personalityReplayKeepsGenderLinkIndependently() {
    currentCase() = "QA-F4 PID replay must keep an existing gender link on its own";
    constexpr uint16_t eevee = 133;               // gender ratio 31 (variable)
    constexpr uint32_t pid = 0x11220090u;          // low byte 0x90 >= 31 -> male (0)
    qacontract::Bytes plain(C::StoredSize, 0);
    qasynth::w32(plain, 0x00, pid);
    qasynth::w16(plain, 0x08, eevee);
    qasynth::w16(plain, 0x0C, 12345);
    qasynth::w16(plain, 0x0E, 54321);
    qasynth::w32(plain, 0x10, 1000);
    plain[0x15] = 9;                               // ability byte NOT Eevee's slot ability
    plain[0x40] = 0;                               // stored gender male, matching the PID
    plain[0x5F] = 21;                              // Black: a Gen V origin
    plain[0x84] = 0x80 | 5;
    const auto encrypted = C::encryptCandidate(plain);
    const auto personal = G5::personal5(eevee, 0, false);
    QA_EXPECT(personal.info && G5::variableGenderRatio5(personal.info->genderRatio), "fixture species");
    QA_EXPECT(G5::genderFromPid5(pid, personal.info->genderRatio) == 0, "fixture gender is PID-linked");

    std::string error;
    auto record = G5::StagedPokemon5Record::create(encrypted, &error, false);
    QA_EXPECT(record.has_value(), "fixture record opens: " + error);
    if (!record) return false;
    const uint32_t breaking = 0x0009FE00u;         // low byte 0x00 < 31 -> female
    const bool accepted = record->stage(G5::StagedPokemon5Record::Field::Personality, 0, breaking, &error);
    bool linkKept = true;
    if (accepted) {
        const auto staged = C::decrypt(record->stagedBytes());
        const uint8_t storedGender = static_cast<uint8_t>((staged[0x40] >> 1) & 3);
        linkKept = G5::genderFromPid5(C::read32(staged, 0), personal.info->genderRatio) == storedGender;
    }
    QA_EXPECT(!accepted || linkKept,
              "a replayed PID was accepted although it breaks the record's existing PID/gender link");
    return !accepted || linkKept;
}

// QA-F5 (Low, Thread 2R: include/Integration/Gen5/Gen5GameCardPreview.h, homeCardSnapshot; fixed on #307,
// PartyPreviewSlot now carries pk->form()).
// The Gen V Home party strip passed form 0 for every member (PartyPreviewSlot has no form and
// homeCardSnapshot wrote `{species, level, 0, shiny}`), so SaveSelectScreen draws the base-form
// icon (SpriteManager::getIconSprite(species, form, shiny)) for alternate-form party members. The
// Gen IV snapshot passes the record's form (Gen4GameCardPreview.h), so Gen IV and Gen V disagree.
// PK5 form lives in the upper five bits of 0x40 (PKHeX PK5.Form; Pokemon5ReadOnly::form()).
// Real production path: inspectSourceFile -> LegacySourceBindings assignment -> HomeSnapshotCache
// -> openAssignedSource -> homeCardSnapshot, the same wiring as SaveSelectScreen.
// Save layout follows tests/test_home_card_gen5_snapshot.cpp (Gen5SaveBlockLayout.h, PKHeX SAV5).
bool gen5HomePartyStripKeepsForm() {
    currentCase() = "QA-F5 Gen V Home party strip keeps the party member's form";
    constexpr uint16_t basculin = 550;
    constexpr uint8_t blueStriped = 1;
    Bytes sav(L::FullSaveSize, 0);
    const size_t tr = 0x19400;
    sav[tr + 4] = 'N';
    sav[tr + 6] = 'X';
    C::write16(sav, tr + 0x14, 12345);
    C::write16(sav, tr + 0x16, 54321);
    C::write16(sav, tr + 0x24, 23);
    sav[tr + 0x1E] = 2;
    sav[tr + 0x1F] = 21;                           // Pokemon Black
    sav[L::PartyOffset + 4] = 1;
    Bytes pk(C::PartySize, 0);
    qasynth::w32(pk, 0x00, 0x12345690u);
    qasynth::w16(pk, 0x08, basculin);
    qasynth::w16(pk, 0x0C, 12345);
    qasynth::w16(pk, 0x0E, 54321);
    pk[0x40] = static_cast<uint8_t>(blueStriped << 3);
    pk[0x5F] = 21;
    pk[0x8C] = 30;
    const auto encrypted = C::encryptCandidate(pk);
    std::copy(encrypted.begin(), encrypted.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8));
    auto partition = std::span<uint8_t>(sav).subspan(0, L::BlackWhiteCopySize);
    for (const auto& b : L::BlackWhite) {
        const uint16_t crc = Utils::crc16ccitt(partition.data() + b.offset, b.length);
        C::write16(partition, b.checksumOffset, crc);
        C::write16(partition, b.mirrorOffset, crc);
    }

    const int before = failures();
    const auto root = scratchRoot("known-f5");
    const std::string path = (root / "Pokemon Black.sav").string();
    writeSource(path, sav);
    const auto probe = G5::inspectSourceFile(path, "DraStic");
    QA_EXPECT(probe.ready(), "synthetic save is a ready Gen V source: " + probe.diagnostic);
    PokeVault::Legacy::LegacySourceBindings bindings((root / "bindings").string());
    QA_EXPECT(bindings.load(), "bindings load");
    QA_EXPECT(bindings.assignFileAndSave(probe.sourceIdentity, {"profile", "black_nds", path, "DraStic", "BW"}),
              "assign save to black_nds");
    const auto assigned = bindings.resolveFileForGame("profile", "black_nds");
    QA_EXPECT(assigned.status == PokeVault::Legacy::AssignedFileStatus::Ready, "assignment resolves");
    if (failures() != before) return false;
    S::HomeSnapshotCache cache;
    const auto& snapshot = cache.refresh(S::assignedSnapshotRequest("profile", "black_nds", assigned),
        S::statRegularFile, [&] { return G5::homeCardSnapshot(G5::openAssignedSource(bindings, "profile", "black_nds")); });
    QA_EXPECT(snapshot.current() && snapshot.partyCount == 1 && snapshot.party[0].species == basculin,
              "snapshot is current with the Basculin party member");
    QA_EXPECT(snapshot.party[0].form == blueStriped,
              "Home party strip form " + std::to_string(snapshot.party[0].form) + " != record form 1");
    return snapshot.current() && snapshot.party[0].form == blueStriped;
}

} // namespace

int main() {
    std::cout << "Regression tests for fixed QA findings:\n";
    const bool empty = commonExportPublishIsExclusive(false);
    const bool full = commonExportPublishIsExclusive(true);
    std::cout << "  QA-F3 export publish exclusivity (racing empty dir): " << (empty ? "PASS" : "FAIL") << "\n";
    std::cout << "  QA-F3 export publish exclusivity (racing non-empty dir): " << (full ? "PASS" : "FAIL") << "\n";
    const bool f4 = personalityReplayKeepsGenderLinkIndependently();
    std::cout << "  QA-F4 Gen V PID replay keeps an existing gender link: " << (f4 ? "PASS" : "FAIL") << "\n";
    const bool f5 = gen5HomePartyStripKeepsForm();
    std::cout << "  QA-F5 Gen V Home party strip keeps form: " << (f5 ? "PASS" : "FAIL") << "\n";
    if (failures() != 0) {
        std::cerr << failures() << " regression(s) of fixed QA findings\n";
        return 1;
    }
    return 0;
}
