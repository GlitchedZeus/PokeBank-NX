#include "Integration/Gen1/Gen1MoveCompatibility.h"

#include <array>
#include "UI/Gen1PokemonDetailsPresentation.h"
#include <cassert>
#include <iostream>

using namespace PokeVault::Integration::Gen1;

int main() {
    for (auto game : {SourceGame::Red, SourceGame::Blue, SourceGame::Yellow}) {
        for (const auto& fixture : std::array<std::pair<uint16_t, std::array<uint16_t,4>>,3>{{
            {4,{10,45,0,0}}, {7,{33,39,0,0}}, {25,{84,45,0,0}}}}) {
            UI::Modals::Gen1PokemonDetailsPresentation view;
            view.species = fixture.first; view.moves = fixture.second;
            view.setMoveCompatibility(game);
            assert(view.moveCompatibilityChecked && view.moveCompatibilityCompatible);
            for (bool compatible : view.moveCompatible) assert(compatible);
            // Reusing a model must replace both per-row and aggregate state.
            view.species = 6; view.moves = {56,0,0,0};
            view.setMoveCompatibility(game);
            assert(!view.moveCompatibilityCompatible && !view.moveCompatible[0]);
            assert(view.moveCompatible[1] && view.moveCompatible[2] && view.moveCompatible[3]);
        }
    }

    // Empty move slots are valid normal-editor choices; out-of-range moves are not.
    assert(MoveCompatibility::canLearnMove(SourceGame::Red, 6, 0));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Red, 6, 166));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Red, 0, 53));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Red, 152, 53));

    // Pinned R/B/Y fixtures. Charizard can use Flamethrower, but Hydro Pump and
    // Waterfall are never normal Gen I choices for it. Fly differs by ruleset:
    // Yellow added the compatibility while Red/Blue did not have it.
    assert(MoveCompatibility::canLearnMove(SourceGame::Red, 6, 53));   // Flamethrower
    assert(MoveCompatibility::canLearnMove(SourceGame::Blue, 6, 53));
    assert(MoveCompatibility::canLearnMove(SourceGame::Yellow, 6, 53));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Red, 6, 56));  // Hydro Pump
    assert(!MoveCompatibility::canLearnMove(SourceGame::Blue, 6, 56));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Yellow, 6, 56));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Red, 6, 127)); // Waterfall
    assert(!MoveCompatibility::canLearnMove(SourceGame::Blue, 6, 127));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Yellow, 6, 127));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Red, 6, 19));  // Fly
    assert(!MoveCompatibility::canLearnMove(SourceGame::Blue, 6, 19));
    assert(MoveCompatibility::canLearnMove(SourceGame::Yellow, 6, 19));

    const auto redCharizard = MoveCompatibility::compatibleMoves(SourceGame::Red, 6);
    assert(!redCharizard.empty() && redCharizard.front() == 0);
    bool hasFlamethrower = false, hasHydroPump = false;
    for (const auto move : redCharizard) {
        if (move == 53) hasFlamethrower = true;
        if (move == 56) hasHydroPump = true;
    }
    assert(hasFlamethrower && !hasHydroPump);

    const std::array<uint8_t,4> compatible{53,0,0,0};
    const std::array<uint8_t,4> incompatible{53,56,0,0};
    assert(!MoveCompatibility::firstIncompatible(SourceGame::Red, 6, compatible));
    const auto bad = MoveCompatibility::firstIncompatible(SourceGame::Red, 6, incompatible);
    assert(bad && *bad == 1);

    std::cout << "Gen I move compatibility: PASS (R/B/Y pinned learnability fixtures)\n";
    return 0;
}
