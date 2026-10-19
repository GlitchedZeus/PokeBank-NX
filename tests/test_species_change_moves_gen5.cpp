// R2-BUG-004, Generation V: Black, White, Black 2, White 2, the Edit path. An existing Pokemon's
// species change goes through exactly what applySpecies stages on its probe: Field::Species, then
// SpeciesChangeMoves::stageMoves (Move, PP Ups, PP per slot) with the BW / B2W2 table (test_species_
// change_moves_gen15 pins that the UI function calls it); the probe replaces the draft, Keep stages
// it, every Review row verifies by independent replay, and the export strictly reparses with the
// new species' native moves (PKHeX lvlmove_bw / _b2w2), base PP, no PP Ups. Only species, ability,
// moves and the checksum differ from the original record. The Create rebuild path is covered by
// tests/test_gen5_party_box_parity.cpp (sharedCreateRoute) for the same four games.
#include "Integration/Gen5/Gen5BlankSlot.h"
#include "Integration/Gen5/Gen5EditorFields.h"
#include "Integration/Gen5/Gen5MovePP.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Pokemon/SpeciesMoveDefaults.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SpeciesChangeMoves.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Reset = PokeBank::UIModel::SpeciesChangeMoves;
namespace Moves = PokeBank::Moves;
using Action = Shared::Action;
using Slot = G::Gen5StagedPokemonWorkspace::Slot;
using Region = G::Gen5StagedPokemonWorkspace::Region;
using Field = G::StagedPokemon5Record::Field;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// tests/test_gen5_party_box_parity.cpp fixture: party Pikachu, Box 1 slot 1 Eevee, slot 2 Snivy.
void write32(std::vector<uint8_t>& dst, size_t off, uint32_t value) {
    C::write16(dst, off, static_cast<uint16_t>(value));
    C::write16(dst, off + 2, static_cast<uint16_t>(value >> 16));
}

// Synthetic SAV5: party Pikachu (Lv 50), Box 1 slot 1 Eevee with native
// met/egg dates, Box 1 slot 2 Snivy. Checksums stamped on the primary copy.
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version) {
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    const size_t tr = 0x19400;
    sav[tr + 4] = 'N'; sav[tr + 6] = 'X';
    C::write16(sav, tr + 0x14, 12345);
    C::write16(sav, tr + 0x16, 54321);
    sav[tr + 0x1F] = version;
    std::vector<uint8_t> pk(C::PartySize, 0);
    write32(pk, 0, 0x12345678);
    C::write16(pk, 8, 25);
    C::write16(pk, 12, 12345);
    C::write16(pk, 14, 54321);
    pk[0x8C] = 50;
    const auto party = C::encryptCandidate(pk);
    sav[L::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8));
    std::vector<uint8_t> eevee(C::StoredSize, 0);
    write32(eevee, 0, 0xABCDEF01);
    C::write16(eevee, 8, 133);
    eevee[0x17] = 2;                        // language
    eevee[0x5F] = version;                  // origin game
    eevee[0x78] = 10; eevee[0x79] = 3; eevee[0x7A] = 6;   // egg date 2010-03-06
    eevee[0x7B] = 11; eevee[0x7C] = 12; eevee[0x7D] = 25; // met date 2011-12-25
    C::write16(eevee, 0x7E, 60003);         // egg location (raw)
    C::write16(eevee, 0x80, 16);            // met location (raw)
    const auto box0 = C::encryptCandidate(eevee);
    std::copy(box0.begin(), box0.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset));
    std::vector<uint8_t> snivy(C::StoredSize, 0);
    write32(snivy, 0, 0x01020304);
    C::write16(snivy, 8, 495);
    const auto box1 = C::encryptCandidate(snivy);
    std::copy(box1.begin(), box1.end(),
              sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + C::StoredSize));
    const auto blocks = family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                            : std::span<const L::SaveBlock>(L::Black2White2);
    const size_t copySize = family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    auto copy = std::span<uint8_t>(sav).subspan(0, copySize);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}

std::array<uint16_t, 4> movesOf(const G::Pokemon5ReadOnly& p) {
    const auto m = p.moves();
    return {m[0], m[1], m[2], m[3]};
}

void run(G::SaveFamily family, std::string_view game, uint8_t version) {
    const bool b2w2 = family == G::SaveFamily::Black2White2;
    const auto sav = fixture(family, version);
    const auto pristine = sav;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
    require(static_cast<bool>(parsed), std::string(game) + " parses: " + error);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    G::Gen5SharedScreenState ui;
    require(ui.openActions(ws, Slot{Region::Box, 0, 0}, error) && ui.activate(ws, Action::Edit, error),
            std::string(game) + " open Edit: " + error);
    require(ui.draft().current()->species() == 133, "fixture Eevee");
    require(ui.black2White2() == b2w2, "the draft knows its BW / B2W2 family");
    // Gen V refuses an unnamed species change (no automatic rename); the UI asks for a name first.
    require(ui.editDraft().stageName(Field::NicknameText, "EEVEE", error), "name: " + error);
    require(ui.editDraft().stage(Field::Move, 0, 33, error) && ui.editDraft().stage(Field::MovePPUps, 0, 3, error),
            "old moves with PP Ups");
    const auto original = *ui.draft().current();
    const auto originalDecoded = std::vector<uint8_t>(original.decodedBytes().begin(), original.decodedBytes().end());
    const uint8_t level = static_cast<uint8_t>(G::EditorFields::level(original, b2w2));
    const auto table = Reset::gen5Table(b2w2);
    require(table == Moves::levelUpTableForGen45Version(version), "surface table == origin-version table");

    // applySpecies on a probe, for each same-growth target the native record accepts.
    int accepted = 0;
    uint16_t last = 0;
    for (const uint16_t species : {22, 25, 19, 52, 504, 506, 509, 133}) {
        auto probe = ui.editDraft();
        const uint16_t before = probe.current()->species();
        if (species != before && !probe.stage(Field::Species, 0, species, error)) continue;  // native refusal
        if (species != before) {
            require(Reset::stageMoves<Field::Move, Field::MovePPUps, Field::MovePP>(probe,
                        Reset::forSpecies(table, species, 0, level,
                            [](uint16_t move) { return G::basePP5(move); }), error),
                    "stage moves: " + error);
        }
        const auto now = *probe.current();
        const auto expected = Moves::defaultMoves(table, species, 0, level);
        if (species != before) {
            require(expected.known && movesOf(now) == expected.moves,
                    std::string(game) + " -> " + std::to_string(species) + ": native default moves");
            for (size_t i = 0; i < 4; ++i)
                require(now.ppUps()[i] == 0 && now.pp()[i] == G::basePP5(now.moves()[i]), "base PP, no PP Ups");
            ++accepted;
        } else {
            require(movesOf(now) == movesOf(*ui.draft().current()), "same species: moves untouched");
        }
        ui.editDraft() = std::move(probe);  // offer() with an unchanged PID applies the probe
        last = species;
    }
    require(accepted >= 3, std::string(game) + ": several species transitions");
    require(last == 133 && ui.draft().current()->species() == 133, "back to Eevee");

    // Eevee -> Fearow for the round trip.
    auto probe = ui.editDraft();
    require(probe.stage(Field::Species, 0, 22, error), "Eevee -> Fearow: " + error);
    require(Reset::stageMoves<Field::Move, Field::MovePPUps, Field::MovePP>(probe,
                Reset::forSpecies(table, 22, 0, level, [](uint16_t move) { return G::basePP5(move); }), error),
            "Fearow moves");
    ui.editDraft() = std::move(probe);
    const auto fearow = *ui.draft().current();
    // Only species, ability, the four moves / PP / PP Ups and the checksum differ.
    const auto& now = fearow.decodedBytes();
    for (size_t i = 0; i < originalDecoded.size(); ++i) {
        if (now[i] == originalDecoded[i]) continue;
        const bool allowed = (i >= 0x06 && i < 0x0A) || i == 0x15 || (i >= 0x28 && i < 0x38);
        require(allowed, std::string(game) + ": PK5 byte " + std::to_string(i) + " changed unexpectedly");
    }
    require(ui.keep(ws, error), std::string(game) + " keep: " + error);
    const auto review = ws.pendingReview();
    require(!review.empty(), "Review lists the change");
    bool speciesRow = false, moveRow = false;
    for (const auto& change : review) {
        const auto verified = G::verifyStagedReview(change, &error);
        require(verified.has_value(), "Review rows replay to the staged bytes: " + error);
        for (const auto& row : verified->fields) {
            speciesRow = speciesRow || (row.field == Field::Species && row.before == 133 && row.after == 22);
            moveRow = moveRow || (row.field == Field::Move && row.after == fearow.moves()[row.stat]);
        }
    }
    require(speciesRow && moveRow, "Review lists the species and the new moves");
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    require(out && out->changedRecords == 1, std::string(game) + " export: " + error);
    const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, G::SaveCopySelection::Primary);
    require(static_cast<bool>(reopened), "strict reparse: " + error);
    const auto back = reopened->boxPokemon(0, 0);
    require(back && back->valid() && back->species() == 22, "reparsed Fearow");
    require(movesOf(*back) == movesOf(fearow) && back->pp() == fearow.pp() && back->ppUps() == fearow.ppUps(),
            "reparsed moves, PP, PP Ups");
    require(back->pid() == original.pid() && back->ivs() == original.ivs() && back->evs() == original.evs() &&
            back->metLocation() == original.metLocation() && back->tid() == original.tid(),
            "PID, IVs, EVs, met data, TID untouched");
    require(reopened->boxPokemon(0, 1)->species() == 495, "Snivy untouched");
    require(sav == pristine, "source bytes never change");
}

} // namespace

int main() {
    run(G::SaveFamily::BlackWhite, "white_nds", 20);
    run(G::SaveFamily::BlackWhite, "black_nds", 21);
    run(G::SaveFamily::Black2White2, "white2_nds", 22);
    run(G::SaveFamily::Black2White2, "black2_nds", 23);
    std::cout << "Gen V species change resets moves (Black, White, Black 2, White 2; Edit probe, Keep, Review "
                 "replay, export, reparse): PASS\n";
    return 0;
}
