// Generation I species change and Create moves: Red, Blue, Yellow. Round 11 (owner H28 / Extra 5 /
// Extra 6) replaces the round 3 R2-BUG-004 refill: a new Create draft starts with NO moves, and a
// species change never adds one. The owner's original hardware case (Yellow Pikachu Thunder Shock /
// Growl -> Fearow kept Thunder Shock, flagged Invalid) stays fixed: the incompatible Thunder Shock
// is cleared, the compatible Growl is kept with its own PP and PP Ups. Through the real Gen I staged
// editor, with the request built by the one Gen I binding every Gen I species-apply path calls
// (Gen1SpeciesChangeMoves.h gen1: Gen1MoveCompatibility for the editor's own game, then writeArrays /
// writeEdit). test_species_change_moves_gen15 pins that ux3 / ux2 ApplySpeciesPicker and both older
// applyPicker overlays call it. Add and Edit, Review rows, Discard, export -> reparse, byte scope,
// the blank Create default, and the at-least-one-move rule.
#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Pokemon/SpeciesMoveDefaults.h"
#include "UI/SpeciesChangeLevelPolicy.h"
#include "UI/CreateRequiredFields.h"
#include "UI/Gen1SpeciesChangeMoves.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen1;
namespace Moves = PokeBank::Moves;
namespace Reset = PokeBank::UIModel::SpeciesMoveDefaults;

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

    // Round 11: the Create default is Pikachu Lv5 with NO moves (the user picks them), and Add
    // with no move is refused by the shared Create rule before anything is staged.
    const BoxPokemonCreate createDefault;
    require(createDefault.species == 25 && createDefault.level == 5 &&
            createDefault.moves == (std::array<uint8_t, 4>{}) &&
            createDefault.pp == (std::array<uint8_t, 4>{}) && createDefault.ppUps == (std::array<uint8_t, 4>{}),
            "Create default starts blank");
    namespace Required = PokeBank::UIModel::CreateRequiredFields;
    require(Required::message(Required::forCreate(1, false, createDefault.moves), "Add") ==
            "Choose at least one move before Add", "blank Gen I Add names the missing move (no Met rule)");

    // Add draft: a blank Pikachu -> Fearow stays blank; nothing is filled in.
    BoxPokemonCreate draft;
    draft.level = PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId(game), 22);
    Reset::writeArrays(Reset::gen1(editor.get(), 22, draft.moves, draft.pp, draft.ppUps),
                       draft.moves, draft.pp, draft.ppUps);
    require(draft.moves == (std::array<uint8_t, 4>{}) && draft.pp == (std::array<uint8_t, 4>{}),
            "blank Create draft: a species change adds no move");
    // The user's own moves: Thunder Shock (Fearow cannot learn it) and Growl (Fearow can).
    draft.moves = {84, 45, 0, 0};
    draft.pp = {30, 38, 0, 0};
    draft.ppUps = {1, 2, 0, 0};
    Reset::writeArrays(Reset::gen1(editor.get(), 22, draft.moves, draft.pp, draft.ppUps),
                       draft.moves, draft.pp, draft.ppUps);
    draft.species = 22;
    draft.nickname = "FEAROW";
    require(draft.moves == (std::array<uint8_t, 4>{45, 0, 0, 0}) && draft.pp == (std::array<uint8_t, 4>{38, 0, 0, 0}) &&
            draft.ppUps == (std::array<uint8_t, 4>{2, 0, 0, 0}),
            "Thunder Shock cleared, Growl kept with its PP and PP Ups, nothing added");
    require(!MoveCompatibility::firstIncompatible(editor->metadata().sourceGame, draft.species, draft.moves),
            "no move is flagged Invalid after the change");
    require(!Required::forCreate(1, false, draft.moves).any(), "one move: Add allowed");
    const auto addSlot = editor->appendSlot(2, error);
    require(addSlot && editor->stageAdd(2, *addSlot, draft, error), "stage Add: " + error);
    const auto added = editor->boxedPokemon(2, *addSlot, error);
    require(added && added->species == 22 && added->moves == draft.moves && added->pp == draft.pp &&
            added->ppUps == draft.ppUps, "added Fearow");
    editor->discard();
    require(!editor->hasPendingChanges(), "Discard after Add");

    // Edit: teach the fixture Pikachu Thunder Shock / Growl / Thunderbolt / Thunder Wave first, then
    // walk several species. Each step keeps only moves the new species can use (a subset of the
    // previous moves, never a new one) with their PP and PP Ups, exactly as the UI stages it.
    BoxPokemonEdit teach;
    teach.moves = std::array<uint8_t, 4>{84, 45, 85, 86};
    teach.pp = std::array<uint8_t, 4>{30, 40, 15, 20};
    teach.ppUps = std::array<uint8_t, 4>{0, 1, 2, 3};
    require(editor->stageEdit(2, 0, teach, error), "teach moves: " + error);
    for (const uint16_t species : {26, 22, 1, 150, 129, 25}) {
        const auto was = *editor->boxedPokemon(2, 0, error);
        BoxPokemonEdit edit;
        edit.species = species;
        edit.level = PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId(game), species);
        const auto slots = Reset::gen1(editor.get(), species, was.moves, was.pp, was.ppUps);
        Reset::writeEdit(slots, edit);
        if (Required::forEdit(was.moves, *edit.moves).move) {
            // Nothing usable is left: the UI refuses this Keep (CreateRequiredFields) and stages nothing.
            for (const auto move : was.moves)
                require(move == 0 || !MoveCompatibility::canLearnMove(editor->metadata().sourceGame, species, move),
                        "refused only when no old move is usable");
            continue;
        }
        require(editor->stageEdit(2, 0, edit, error), "stage Edit " + std::to_string(species) + ": " + error);
        const auto now = *editor->boxedPokemon(2, 0, error);
        require(now.species == species && now.moves == *edit.moves && now.pp == *edit.pp &&
                now.ppUps == *edit.ppUps, "edited record carries the kept moves");
        for (std::size_t i = 0; i < 4; ++i) {
            if (now.moves[i] == 0) continue;
            const auto at = std::find(was.moves.begin(), was.moves.end(), now.moves[i]);
            require(at != was.moves.end(), "no move is ever added");
            const auto from = static_cast<std::size_t>(at - was.moves.begin());
            require(now.pp[i] == was.pp[from] && now.ppUps[i] == was.ppUps[from], "PP and PP Ups travel with the move");
        }
        require(!MoveCompatibility::firstIncompatible(editor->metadata().sourceGame, species, now.moves),
                "kept moves are compatible");
    }
    // Pikachu -> Raichu keeps all four electric moves; Raichu -> Magikarp would keep none (refused).
    editor->discard();
    require(editor->stageEdit(2, 0, teach, error), "teach again: " + error);
    {
        const auto was = *editor->boxedPokemon(2, 0, error);
        const auto raichu = Reset::gen1(editor.get(), 26, was.moves, was.pp, was.ppUps);
        require(raichu.moves == (std::array<uint16_t, 4>{84, 45, 85, 86}), "Raichu keeps Pikachu's moves");
        std::array<uint8_t, 4> raichuMoves{};
        for (std::size_t i = 0; i < 4; ++i) raichuMoves[i] = static_cast<uint8_t>(raichu.moves[i]);
        const auto magikarp = Reset::gen1(editor.get(), 129, raichuMoves, was.pp, was.ppUps);
        require(magikarp.moves == (std::array<uint16_t, 4>{}) &&
                Required::forEdit(was.moves, magikarp.moves).move &&
                Required::message(Required::forEdit(was.moves, magikarp.moves)) == "Choose at least one move before Keep",
                "Magikarp can use none of them: the change is refused with the shared message");
    }

    // Pikachu -> Fearow Edit, Review rows, export -> strict reparse, byte scope.
    editor->discard();
    require(editor->stageEdit(2, 0, teach, error), "teach: " + error);
    const auto taught = *editor->boxedPokemon(2, 0, error);
    BoxPokemonEdit edit;
    edit.species = 22;
    edit.level = PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId(game), 22);
    Reset::writeEdit(Reset::gen1(editor.get(), 22, taught.moves, taught.pp, taught.ppUps), edit);
    require(*edit.moves == (std::array<uint8_t, 4>{45, 0, 0, 0}), "Fearow keeps Growl only");
    require(editor->stageEdit(2, 0, edit, error), "stage Fearow: " + error);
    bool moveRow = false, ppRow = false;
    for (const auto& c : editor->pendingChanges()) {
        moveRow = moveRow || (c.key == "2/0/move1" && c.label == "Box 3 Slot 1: Move 1");
        ppRow = ppRow || c.key.starts_with("2/0/pp");
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
            back->ppUps == *edit.ppUps, "reparsed Fearow");
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
    std::cout << "Gen I blank Create and species change keeps only compatible moves (Red, Blue, Yellow; "
                 "Pikachu -> Fearow, Add, Edit, Review, export, reparse, Discard): PASS\n";
    return 0;
}
