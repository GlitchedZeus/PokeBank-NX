// Generation V species change, Create moves and history: Black, White, Black 2, White 2. Round 11
// (owner H28 / Extra 5 / Extra 6 / H25 / H07) replaces the round 3 R2-BUG-004 refill.
// Edit: an existing Pokemon's species change goes through exactly what applySpecies stages on its
// probe: Field::Species, then SpeciesMoveDefaults::stageMoves (Move, PP Ups, PP per slot) with
// speciesChangeMoves5 -- only the moves the new species can use in this exact game, with their PP
// and PP Ups, nothing added (test_species_change_moves_gen15 pins that the UI function calls it);
// the probe replaces the draft, Keep stages it, every Review row verifies by independent replay,
// and the export strictly reparses. Only species, ability, moves and the checksum differ; Met data
// and Pokerus are the record's own history and stay.
// Create: the shared draft starts with no moves; recreateCreateSpecies never adds one, resets a
// chosen Met Location that is no encounter of the new species (Pikachu -> Arbok) and keeps one that
// is (Pikachu -> Raichu, ancestry); Keep names every missing field. More Create coverage:
// tests/test_gen5_party_box_parity.cpp (sharedCreateRoute) for the same four games.
#include "Integration/Gen5/Gen5BlankSlot.h"
#include "Integration/Gen5/Gen5EditorFields.h"
#include "Integration/Gen5/Gen5MovePP.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5MoveCompatibility.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SpeciesChangeMoves.h"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>
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
namespace Reset = PokeBank::UIModel::SpeciesMoveDefaults;
namespace Compat = PokeVault::Integration::Gen5MoveCompatibility;
namespace Context = PokeVault::Integration::EncounterContext45;
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
    const auto usable = [&](uint16_t species, uint16_t move) {
        return Compat::classify(game, species, 0, move, false) != Compat::Availability::Unavailable;
    };
    require(ui.editDraft().stage(Field::Move, 1, 98, error) && ui.editDraft().stage(Field::Move, 2, 45, error) &&
            ui.editDraft().stage(Field::MovePP, 2, 30, error), "Quick Attack, Growl at 30 PP");

    // applySpecies on a probe, for each same-growth target the native record accepts.
    int accepted = 0;
    uint16_t last = 0;
    for (const uint16_t species : {22, 25, 19, 52, 504, 506, 509, 133}) {
        auto probe = ui.editDraft();
        const auto was = *probe.current();
        const uint16_t before = was.species();
        if (species != before && !probe.stage(Field::Species, 0, species, error)) continue;  // native refusal
        if (species != before) {
            require(Reset::stageMoves<Field::Move, Field::MovePPUps, Field::MovePP>(probe,
                        G::speciesChangeMoves5(game, species, 0, was), error),
                    "stage moves: " + error);
        }
        const auto now = *probe.current();
        if (species != before) {
            const auto wasMoves = was.moves(), nowMoves = now.moves();
            const auto wasPP = was.pp(), wasUps = was.ppUps(), nowPP = now.pp(), nowUps = now.ppUps();
            bool empty = false;
            for (size_t i = 0; i < 4; ++i) {
                const auto move = nowMoves[i];
                if (move == 0) { empty = true; require(nowPP[i] == 0 && nowUps[i] == 0, "empty slot 0/0"); continue; }
                require(!empty, "kept moves move up");
                const auto from = std::find(wasMoves.begin(), wasMoves.end(), move);
                require(from != wasMoves.end(), std::string(game) + ": no move is ever added");
                const auto at = static_cast<size_t>(from - wasMoves.begin());
                require(nowPP[i] == wasPP[at] && nowUps[i] == wasUps[at], "PP and PP Ups travel");
                require(usable(species, move), "kept move usable");
            }
            for (const auto move : wasMoves)
                if (move && usable(species, move))
                    require(std::find(nowMoves.begin(), nowMoves.end(), move) != nowMoves.end(),
                            "a usable move is kept");
            require(now.metLocation() == was.metLocation() && now.pokerusMode() == was.pokerusMode(),
                    "Edit keeps Met Location and Pokerus");
            ++accepted;
        } else {
            require(movesOf(now) == movesOf(*ui.draft().current()), "same species: moves untouched");
        }
        ui.editDraft() = std::move(probe);  // offer() with an unchanged PID applies the probe
        last = species;
    }
    require(accepted >= 3, std::string(game) + ": several species transitions");
    require(last == 133 && ui.draft().current()->species() == 133, "back to Eevee");

    // Eevee -> Fearow for the round trip, from Tackle / Quick Attack / Growl again.
    for (const auto& [slot, move] : {std::pair<size_t, uint32_t>{0, 33}, {1, 98}, {2, 45}})
        require(ui.editDraft().stage(Field::Move, slot, move, error), "re-teach: " + error);
    auto probe = ui.editDraft();
    const auto eevee = *probe.current();
    require(probe.stage(Field::Species, 0, 22, error), "Eevee -> Fearow: " + error);
    require(Reset::stageMoves<Field::Move, Field::MovePPUps, Field::MovePP>(probe,
                G::speciesChangeMoves5(game, 22, 0, eevee), error),
            "Fearow moves");
    ui.editDraft() = std::move(probe);
    const auto fearow = *ui.draft().current();
    require(fearow.moves()[0] != 0, "Fearow keeps at least one usable move");
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
    require(speciesRow && moveRow, "Review lists the species and the kept moves");
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

// Create (H25 / H28 / H07): a blank shared Create draft, species rebuilds and Keep's required fields.
void createHistory(G::SaveFamily family, std::string_view game, uint8_t version) {
    auto sav = fixture(family, version);
    sav[0x19400 + 0x1E] = 2;  // English trainer, as test_gen5_party_box_parity sharedCreateRoute
    const auto& blank = G::encryptedBlankBoxSlot5();
    for (size_t box = 0; box < L::BoxCount; ++box)
        for (size_t slot = 0; slot < L::BoxSlots; ++slot) {
            if (box == 0 && slot < 2) continue;
            std::copy(blank.begin(), blank.end(),
                      sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + box * L::BoxStride + slot * C::StoredSize));
        }
    const auto blocks = family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                            : std::span<const L::SaveBlock>(L::Black2White2);
    auto copy = std::span<uint8_t>(sav).subspan(0, family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize
                                                                                        : L::Black2White2CopySize);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
    require(static_cast<bool>(parsed), std::string(game) + " parses: " + error);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    const auto ok = [](std::string&) { return true; };
    const auto defaults = G::Gen5SharedScreenState::sharedCreateDefaults(ws, 0x00340000u, "Patrat", {2026, 10, 31}, error);
    require(defaults && defaults->moves == (std::array<G::CreateMove5, 4>{}) && defaults->draftWithoutMoves,
            std::string(game) + ": the Create default has no moves: " + error);
    // Only the shared editor's draft may be moveless; an ordinary native Create still needs move 1.
    auto strict = *defaults;
    strict.draftWithoutMoves = false;
    require(!G::validateCreate5(strict), "a moveless request without the draft flag is refused");
    auto half = *defaults;
    half.moves[0] = {33, 0, std::nullopt};
    require(!G::validateCreate5(half), "a draft-flagged request must be fully empty");

    // The first wild location of Patrat in this exact game (kWild45).
    std::optional<uint16_t> patratAt;
    for (const auto& entry : Context::kWild45)
        if (entry.sourceGameId == game && entry.species == 504) { patratAt = entry.location; break; }
    require(patratAt.has_value(), "Patrat is wild in every BW / B2W2 game");

    for (const uint16_t next : {151, 505}) {   // Mew: never wild; Watchog: Patrat ancestry
        G::Gen5SharedScreenState ui;
        const size_t slot = next == 151 ? 5 : 6;
        require(ui.openActions(ws, Slot{Region::Box, 0, slot}, error) && ui.activate(ws, Action::Add, error) &&
                ui.beginCreateDraft(ws, *defaults, ok, error), std::string(game) + " Create: " + error);
        require(movesOf(*ui.draft().current()) == (std::array<uint16_t, 4>{}), "Create starts with no moves");
        require(!ui.keepWithPresentation(ws, ok, error) &&
                error == "Choose a Met Location and at least one move before Keep", "Keep names both: " + error);
        require(ui.recreateCreateSpecies(ws, 504, false, "Patrat", "Pikachu", ok, error), "-> Patrat: " + error);
        require(movesOf(*ui.draft().current()) == (std::array<uint16_t, 4>{}), "a species change adds no move");
        require(ui.editDraft().stage(Field::MetLocation, 0, *patratAt, error), "met where Patrat lives: " + error);
        ui.noteCreateMetLocationChosen();
        bool reset = true;
        require(ui.recreateCreateSpecies(ws, next, false, next == 151 ? "Mew" : "Watchog", "Patrat", ok, error, reset),
                std::string(game) + " -> " + std::to_string(next) + ": " + error);
        const auto now = *ui.draft().current();
        require(now.species() == next && now.pokerusMode() == 0, "rebuilt; no Pokerus");
        if (next == 151) {
            require(reset && ui.createNeedsMetLocation() &&
                    now.metLocation() == G::Gen5SharedScreenState::kSharedCreateMetLocation,
                    std::string(game) + ": Mew was never met at Patrat's location -> placeholder, not chosen");
            require(!ui.keepWithPresentation(ws, ok, error) &&
                    error == "Choose a Met Location and at least one move before Keep", "Keep: " + error);
        } else {
            require(!reset && !ui.createNeedsMetLocation() && now.metLocation() == *patratAt,
                    std::string(game) + ": a Patrat location stays valid for Watchog");
            require(!ui.keepWithPresentation(ws, ok, error) && error == "Choose at least one move before Keep",
                    "Keep: " + error);
            require(ui.editDraft().stage(Field::Move, 0, 33, error), "Tackle: " + error);
            require(ui.keepWithPresentation(ws, ok, error) && !ui.createPending(), "Keep: " + error);
        }
    }
    for (const auto& change : ws.pendingReview())
        require(G::verifyStagedReview(change, &error).has_value(), "Review replays the moveless-draft certificate: " + error);
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    require(out.has_value(), std::string(game) + " export with a moveless draft certificate: " + error);
}

} // namespace

int main() {
    run(G::SaveFamily::BlackWhite, "white_nds", 20);
    run(G::SaveFamily::BlackWhite, "black_nds", 21);
    run(G::SaveFamily::Black2White2, "white2_nds", 22);
    run(G::SaveFamily::Black2White2, "black2_nds", 23);
    createHistory(G::SaveFamily::BlackWhite, "white_nds", 20);
    createHistory(G::SaveFamily::BlackWhite, "black_nds", 21);
    createHistory(G::SaveFamily::Black2White2, "white2_nds", 22);
    createHistory(G::SaveFamily::Black2White2, "black2_nds", 23);
    std::cout << "Gen V species change keeps usable moves; blank Create, Create history reset and required "
                 "fields (Black, White, Black 2, White 2; Edit probe, Keep, Review replay, export, reparse): PASS\n";
    return 0;
}
