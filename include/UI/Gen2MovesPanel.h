#pragma once
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "UI/ClassicMovesPanel.h"

// Gen II side of the classic shared-Moves-panel adapter (UI/ClassicMovesPanel.h).
namespace UI::Gen2MovesPanel {

using Status = ClassicMovesPanel::Status;
using ClassicMovesPanel::hitSlot;

// Max PP is pret/pokecrystal ComputeMaxPP (StagedEditor::gen2MoveMaxPP): 40 PP + 3 PP Ups = 61.
inline std::array<SharedMovesPanel::Slot, 4> slots(const std::array<uint8_t, 4>& moves,
                                                   const std::array<uint8_t, 4>& pp,
                                                   const std::array<uint8_t, 4>& ppUps,
                                                   const std::array<Status, 4>& status) {
    std::array<uint8_t, 4> maxPP{};
    for (std::size_t i = 0; i < 4; ++i)
        maxPP[i] = PokeVault::Integration::Gen2::StagedEditor::gen2MoveMaxPP(moves[i], ppUps[i]);
    return ClassicMovesPanel::slots(moves, pp, ppUps, maxPP, status);
}

inline void draw(PKSEFramebuffer& fb, int panelX, int panelY, int panelW,
                 const std::array<SharedMovesPanel::Slot, 4>& rows, int focusedSlot) {
    ClassicMovesPanel::draw(fb, panelX, panelY, panelW, rows, Enums::GameVersion::GSC, focusedSlot);
}

} // namespace UI::Gen2MovesPanel
