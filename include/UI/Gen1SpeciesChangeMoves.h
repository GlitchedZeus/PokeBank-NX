#pragma once

// Generation I binding of the shared species-change move policy (R2-BUG-004, SpeciesChangeMoves.h):
// the new species' native default moveset at `level` (pret pokered / pokeyellow WriteMonMoves:
// base-stats "level 1 learnset" + evos_moves; Yellow has its own table), Gen I base PP (pret
// moves.asm via StagedPokemonEditor::moveBasePP), no PP Ups. Every Gen I species-apply path calls
// this one function: ux3ApplySpeciesPicker, ux2ApplySpeciesPicker, and both older applyPicker
// overlays (Gen1PokemonEditorOverlayUX.inc, Gen1PokemonEditorOverlay.cpp).

#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "UI/SpeciesChangeMoves.h"

#include <cstdint>

namespace PokeBank::UIModel::SpeciesMoveDefaults {

[[nodiscard]] inline Slots gen1(const PokeVault::Integration::Gen1::StagedPokemonEditor* editor,
                                uint16_t species, uint8_t level) {
    const bool yellow = editor &&
        editor->metadata().sourceGame == PokeVault::Integration::Gen1::SourceGame::Yellow;
    return forSpecies(::PokeBank::Moves::levelUpTableGen1(yellow), species, 0, level, [](uint16_t move) {
        return PokeVault::Integration::Gen1::StagedPokemonEditor::moveBasePP(move);
    });
}

} // namespace PokeBank::UIModel::SpeciesMoveDefaults
