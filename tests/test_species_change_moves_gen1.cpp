// R2-BUG-004, Generation I: Red, Blue, Yellow. The owner's hardware case (Yellow Create Pikachu
// Lv5 -> Fearow kept Thunder Shock / Growl, Thunder Shock flagged Invalid) through the real Gen I
// staged editor, with the request built by the same shared helpers ux3ApplySpeciesPicker uses
// (SpeciesChangeMoves::writeArrays / writeEdit, Gen I base PP). test_species_change_moves_gen15
// pins that the UI function calls them. Add and Edit, Review rows, Discard, export -> reparse,
// byte scope, the Create default, and Gen I move compatibility after the change.
#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Pokemon/SpeciesMoveDefaults.h"
#include "UI/SpeciesChangeLevelPolicy.h"
#include "UI/SpeciesChangeMoves.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen1;
namespace Moves = PokeBank::Moves;
namespace Reset = PokeBank::UIModel::SpeciesChangeMoves;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// tests/test_rby_pokemon_editor.cpp fixture: Pikachu in box 2 (the current box).
uint8_t sum(const std::vector<uint8_t>& v, size_t start, size_t n) {
    unsigned s = 0;
    for (size_t i = start; i < start + n; ++i) s += v[i];
    return (~s) & 255;
}
size_t bank(size_t b) { return (b < 6 ? 0x4000 : 0x6000) + (b % 6) * 0x462; }
void checksums(std::vector<uint8_t>& v) {
    v[0x3523] = sum(v, 0x2598, 0xF8B);
    for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
        v[b + 0x1A4C] = sum(v, b, 0x1A4C);
        for (size_t i = 0; i < 6; ++i) v[b + 0x1A4D + i] = sum(v, b + i * 0x462, 0x462);
    }
}
std::vector<uint8_t> fixture(SourceGame game) {
    std::vector<uint8_t> v(0x8000, 0);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83; v[0x259B] = 0x50;  // RED
    v[0x2605] = 0x12; v[0x2606] = 0x34;
    v[0x25CA] = v[0x27E7] = v[0x2F2D] = 0xFF;
    v[0x284C] = 0x82;
    v[0x29C3] = game == SourceGame::Yellow ? 0x54 : 0x99;
    for (size_t b = 0; b < 12; ++b) v[bank(b) + 1] = 0xFF;
    const size_t box = 0x30C0, body = box + 22;
    v[box] = 1; v[box + 1] = 0x54; v[box + 2] = 0xFF;  // Pikachu, internal 0x54
    v[body] = 0x54; v[body + 1] = 0; v[body + 2] = 20; v[body + 3] = 5;
    v[body + 5] = v[body + 6] = 23; v[body + 7] = 190; v[body + 8] = 33;
    v[body + 12] = 0x12; v[body + 13] = 0x34; v[body + 16] = 125;
    v[body + 27] = 0x88; v[body + 28] = 0x88; v[body + 29] = 35;
    std::fill_n(v.begin() + box + 682, 11, 0x50); v[box + 682] = 0x91; v[box + 683] = 0x84; v[box + 684] = 0x83;
    std::fill_n(v.begin() + box + 902, 11, 0x50);
    v[box + 902] = 0x8F; v[box + 903] = 0x88; v[box + 904] = 0x8A; v[box + 905] = 0x80;  // PIKA
    std::copy_n(v.begin() + box, 0x462, v.begin() + bank(2));
    checksums(v);
    return v;
}

const char* sourceId(SourceGame game) {
    return game == SourceGame::Yellow ? "yellow_gb" : game == SourceGame::Red ? "red_gb" : "blue_gb";
}

// The request ux3ApplySpeciesPicker builds for an Edit species change.
Reset::Slots slotsFor(SourceGame game, uint16_t species, uint8_t level) {
    return Reset::forSpecies(Moves::levelUpTableGen1(game == SourceGame::Yellow), species, 0, level,
                             [](uint16_t move) { return StagedPokemonEditor::moveBasePP(move); });
}

bool inAllowedRange(size_t i) {
    const auto in = [&](size_t a, size_t n) { return i >= a && i < a + n; };
    return in(0x30C0, 0x462) || in(bank(2), 0x462) || i == 0x3523 || in(0x4000 + 0x1A4C, 7) ||
           in(0x6000 + 0x1A4C, 7);
}

void run(SourceGame game) {
    const auto raw = fixture(game);
    const auto source = parse(raw, game);
    require(static_cast<bool>(source), "fixture parses");
    std::string error;
    auto editor = StagedPokemonEditor::create(*source.save, error);
    require(static_cast<bool>(editor), "editor");
    const auto normalized = editor->finalizedBytes(error);
    const auto before = *editor->boxedPokemon(2, 0, error);
    require(before.species == 25, "fixture Pikachu");

    // Create default stays the accepted Pikachu Lv5 Thunder Shock / Growl, and it IS the
    // native default (pret base_stats/pikachu.asm).
    const BoxPokemonCreate createDefault;
    require(createDefault.species == 25 && createDefault.level == 5 &&
            createDefault.moves == (std::array<uint8_t, 4>{84, 45, 0, 0}) &&
            createDefault.pp == (std::array<uint8_t, 4>{30, 40, 0, 0}), "Create default unchanged");
    const auto pikachu = slotsFor(game, 25, 5);
    require(pikachu.moves == (std::array<uint16_t, 4>{84, 45, 0, 0}), "native Pikachu Lv5 = Create default");

    // Add draft: Pikachu Lv5 -> Fearow, as the Add branch does (level policy, then moves).
    BoxPokemonCreate draft;
    draft.level = PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId(game), 22);
    Reset::writeArrays(slotsFor(game, 22, draft.level), draft.moves, draft.pp, draft.ppUps);
    draft.species = 22;
    draft.nickname = "FEAROW";
    require(std::find(draft.moves.begin(), draft.moves.end(), 84) == draft.moves.end(),
            "Thunder Shock is gone after Pikachu -> Fearow");
    require(!MoveCompatibility::firstIncompatible(editor->metadata().sourceGame, draft.species, draft.moves),
            "no move is flagged Invalid after the change");
    require(draft.moves[0] == 64 && draft.moves[1] == 45 && draft.moves[2] == 43, "Fearow: Peck, Growl, Leer");
    for (size_t i = 0; i < 4; ++i) {
        require(draft.ppUps[i] == 0, "PP Ups 0");
        require(draft.pp[i] == StagedPokemonEditor::moveBasePP(draft.moves[i]), "base PP");
    }
    const auto addSlot = editor->appendSlot(2, error);
    require(addSlot && editor->stageAdd(2, *addSlot, draft, error), "stage Add: " + error);
    const auto added = editor->boxedPokemon(2, *addSlot, error);
    require(added && added->species == 22 && added->moves == draft.moves && added->pp == draft.pp, "added Fearow");
    editor->discard();
    require(!editor->hasPendingChanges(), "Discard after Add");

    // Edit: several transitions with different learnsets; each stages exactly what the UI does.
    for (const uint16_t species : {22, 1, 150, 129, 25}) {
        BoxPokemonEdit edit;
        edit.species = species;
        edit.level = PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId(game), species);
        Reset::writeEdit(slotsFor(game, species, *edit.level), edit);
        require(editor->stageEdit(2, 0, edit, error), "stage Edit " + std::to_string(species) + ": " + error);
        const auto now = *editor->boxedPokemon(2, 0, error);
        require(now.species == species && now.moves == *edit.moves && now.pp == *edit.pp &&
                now.ppUps == (std::array<uint8_t, 4>{}), "edited record carries the native moves");
        require(!MoveCompatibility::firstIncompatible(editor->metadata().sourceGame, species, now.moves),
                "native moves are compatible");
    }

    // Pikachu -> Fearow Edit, Review rows, export -> strict reparse, byte scope.
    editor->discard();
    BoxPokemonEdit edit;
    edit.species = 22;
    edit.level = PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId(game), 22);
    Reset::writeEdit(slotsFor(game, 22, *edit.level), edit);
    require(editor->stageEdit(2, 0, edit, error), "stage Fearow: " + error);
    bool moveRow = false, ppRow = false;
    for (const auto& c : editor->pendingChanges()) {
        moveRow = moveRow || (c.key == "2/0/move1" && c.label == "Box 3 Slot 1: Move 1");
        ppRow = ppRow || c.key.starts_with("2/0/pp");  // PP 1 equals the fixture's 35 = Peck's base PP
    }
    require(moveRow && ppRow, "Review lists the move and PP changes");
    const auto out = editor->finalizedBytes(error);
    require(!out.empty() && error.empty(), "export: " + error);
    for (size_t i = 0; i < out.size(); ++i)
        if (out[i] != normalized[i]) require(inAllowedRange(i), "byte " + std::to_string(i) + " outside the edited box changed");
    const auto reparsed = parse(out, game);
    require(static_cast<bool>(reparsed), "strict reparse");
    auto check = StagedPokemonEditor::create(*reparsed.save, error);
    const auto back = check->boxedPokemon(2, 0, error);
    require(back && back->species == 22 && back->moves == *edit.moves && back->pp == *edit.pp &&
            back->ppUps == (std::array<uint8_t, 4>{}), "reparsed Fearow");
    require(back->trainerId == before.trainerId && back->originalTrainer == before.originalTrainer &&
            back->dvs == before.dvs &&
            back->statExperience == before.statExperience, "trainer ID, DVs and Stat Exp untouched");
    editor->discard();
    require(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), editor->originalBytes().begin()),
            "Discard restores every byte");
    require(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()), "source bytes never change");
}

} // namespace

int main() {
    run(SourceGame::Red);
    run(SourceGame::Blue);
    run(SourceGame::Yellow);
    std::cout << "Gen I species change resets moves (Red, Blue, Yellow; Pikachu -> Fearow, Add, Edit, Review, "
                 "export, reparse, Discard): PASS\n";
    return 0;
}
