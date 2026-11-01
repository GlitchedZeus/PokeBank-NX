#pragma once

// Generation I binding of the shared species-change move policy (SpeciesChangeMoves.h, Round 11):
// the draft's own moves that `species` can use in this exact Red / Blue / Yellow ruleset
// (Gen1MoveCompatibility, the same filter the Gen I move picker and Stage Add use) are kept with
// their PP and PP Ups; the rest are cleared. No move is ever added. Every Gen I species-apply path
// calls this one function: ux3ApplySpeciesPicker, ux2ApplySpeciesPicker, and both older
// applyPicker overlays (Gen1PokemonEditorOverlayUX.inc, Gen1PokemonEditorOverlay.cpp).

#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "UI/SpeciesChangeMoves.h"

#include <array>
#include <cstdint>

namespace PokeBank::UIModel::SpeciesMoveDefaults {

[[nodiscard]] inline Slots gen1(const PokeVault::Integration::Gen1::StagedPokemonEditor* editor,
                                uint16_t species, const std::array<uint8_t, 4>& moves,
                                const std::array<uint8_t, 4>& pp, const std::array<uint8_t, 4>& ppUps) {
    // No editor: no exact game, so nothing can be shown compatible and every slot is cleared.
    if (!editor) return {};
    const auto game = editor->metadata().sourceGame;
    return keepLearnable(moves, pp, ppUps, [game, species](uint16_t move) {
        return PokeVault::Integration::Gen1::MoveCompatibility::canLearnMove(game, species, move);
    });
}

} // namespace PokeBank::UIModel::SpeciesMoveDefaults
