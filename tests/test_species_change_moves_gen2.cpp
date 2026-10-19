// R2-BUG-004, Generation II: Gold, Silver, Crystal through the real Session::setSpecies (the one
// function both Gen II species-apply paths call) and the real StagedEditor, ending in a strict
// export -> reparse. A species change brings the new species' native moves (pret pokegold /
// pokecrystal FillMoves over evos_attacks) with base PP and no PP Ups; the same species changes
// nothing; Discard restores; only the edited box's bytes change in the export.
#include "fixtures/gsc_pokemon_fixture.h"
#include "Pokemon/SpeciesMoveDefaults.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/SpeciesChangeLevelPolicy.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
namespace Moves = PokeBank::Moves;
using Rules::Session;
using Rules::SessionMode;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

std::array<uint8_t, 4> expectedMoves(bool crystal, uint16_t species, uint8_t level) {
    const auto d = Moves::defaultMoves(Moves::levelUpTableGen2(crystal), species, 0, level);
    require(d.known && d.count > 0, "Gen II learnset known");
    std::array<uint8_t, 4> out{};
    for (std::size_t i = 0; i < 4; ++i) out[i] = static_cast<uint8_t>(d.moves[i]);
    return out;
}

void checkNativeMoves(const PokemonRecord& p, bool crystal, const std::string& what) {
    require(p.moves == expectedMoves(crystal, p.species, p.level), what + ": native default moves");
    for (std::size_t i = 0; i < 4; ++i) {
        require(p.ppUps[i] == 0, what + ": PP Ups 0");
        require(p.pp[i] == StagedEditor::gen2MoveBasePP(p.moves[i]), what + ": base PP");
    }
}

void run(const L& layout, SourceGame game, std::string_view sourceId) {
    const bool crystal = game == SourceGame::Crystal;
    const auto raw = fixture(layout, true);
    auto parsed = parse(raw, game);
    require(static_cast<bool>(parsed), "fixture parses");
    std::string error;

    // Box 1 slot 0 is a non-current box, so its export bytes are isolated.
    auto baseEditor = StagedEditor::create(*parsed.save, error);
    require(static_cast<bool>(baseEditor), "editor");
    const auto normalized = baseEditor->finalizedBytes(error);
    auto entry = *baseEditor->boxedPokemon(1, 0, error);
    require(entry.species == 25, "fixture Pikachu");

    // Transitions with different learnsets, Edit and Create sessions.
    const uint16_t chain[] = {22, 1, 152, 133, 22, 249, 25};
    for (const auto mode : {SessionMode::Edit, SessionMode::Create}) {
        Session s;
        s.begin(entry, mode);
        s.working.moves = {84, 45, 98, 0};  // Thunder Shock, Growl, Quick Attack
        s.working.pp = {30, 40, 30, 0};
        s.working.ppUps = {3, 2, 1, 0};
        for (const uint16_t species : chain) {
            require(s.setSpecies(species, sourceId), "setSpecies " + std::to_string(species));
            require(s.working.level == PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, species),
                    "level policy unchanged");
            checkNativeMoves(s.working, crystal, std::string(sourceId) + " -> " + std::to_string(species));
            for (const auto move : s.working.moves)
                require(move != 84 || species == 25, "Thunder Shock never survives a change away from Pikachu");
            // Same species: nothing changes, even after hand-edited moves.
            require(s.setMove(3, 33), "hand-edit slot 4");
            const auto before = s.working;
            require(s.setSpecies(species, sourceId), "reselect");
            require(Rules::sameEditableRecord(before, s.working), "same species changes nothing");
        }
        // Unknown game: no table, so the slots are cleared -- never the old species' moves.
        Session unknown;
        unknown.begin(entry, mode);
        unknown.working.moves = {84, 45, 0, 0};
        require(unknown.setSpecies(22, "not_a_gen2_game"), "unknown-game species change");
        require(unknown.working.moves == (std::array<uint8_t, 4>{}) && unknown.working.pp == (std::array<uint8_t, 4>{}),
                "no data: empty slots");
    }

    // Pikachu -> Fearow, Keep, Review, export -> strict reparse; only box 1 bytes move.
    auto editor = StagedEditor::create(*parsed.save, error);
    Session s;
    s.begin(entry, SessionMode::Edit);
    require(s.setSpecies(22, sourceId), "Pikachu -> Fearow");
    const auto fearow = s.working;
    require(s.keep(*editor, 1, 0, error), "keep: " + error);
    require(!editor->pendingChanges().empty(), "Review lists the change");
    const auto staged = editor->boxedPokemon(1, 0, error);
    require(staged && staged->species == 22 && staged->moves == fearow.moves && staged->pp == fearow.pp &&
            staged->ppUps == fearow.ppUps, "staged record carries the new moves");
    const auto out = editor->finalizedBytes(error);
    require(!out.empty() && error.empty(), "export");
    require(differsOnlyInRange(normalized, out, boxStart(1), boxStart(1) + listLen(20, 32, 11)),
            "no byte outside the edited box changes");
    auto reloaded = parse(out, game);
    require(static_cast<bool>(reloaded), "strict reparse");
    auto check = StagedEditor::create(*reloaded.save, error);
    const auto back = check->boxedPokemon(1, 0, error);
    require(back && back->species == 22 && back->moves == fearow.moves && back->pp == fearow.pp &&
            back->ppUps == fearow.ppUps, "reparsed Fearow has its native moves");
    checkNativeMoves(*back, crystal, "reparsed");
    const auto other = check->boxedPokemon(0, 0, error);
    require(other && other->species == 25, "the other box is untouched");
    require(raw == fixture(layout, true), "source bytes never change");

    // Discard restores the original record exactly.
    editor->discard();
    require(!editor->hasPendingChanges(), "Discard clears Review");
    require(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), editor->originalBytes().begin()),
            "Discard restores every byte");
}

} // namespace

int main() {
    run(GS, SourceGame::Gold, "gold_gbc");
    run(GS, SourceGame::Silver, "silver_gbc");
    run(C, SourceGame::Crystal, "crystal_gbc");
    std::cout << "Gen II species change resets moves (Gold, Silver, Crystal; keep, Review, export, reparse, "
                 "Discard): PASS\n";
    return 0;
}
