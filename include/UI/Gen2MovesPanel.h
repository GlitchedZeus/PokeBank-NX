#pragma once
#include "Enums/GameVersion.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "UI/SharedMovesPanel.h"

#include <array>
#include <cstdint>

// Gen II adapter for the ONE shared MOVES panel (UI/SharedMovesPanel.h). It supplies only native
// data: the four move ids, current PP, PP Ups and each slot's compatibility status. Layout, focus
// outline and columns are the shared panel's, so Gen II rows sit exactly where Gen III-V rows do,
// and the touch hit test uses the same rows the panel draws.
namespace UI::Gen2MovesPanel {

enum class Status : uint8_t { Ok, Preserved, Invalid };

struct StatusView { const char* text; Color color; };

constexpr StatusView statusView(Status status) noexcept {
    switch (status) {
        case Status::Ok: return {"OK", Colors::Success};
        case Status::Preserved: return {"Preserved", Colors::Warning};
        default: return {"Invalid", Colors::Error};
    }
}

// Gen II max PP follows pret/pokecrystal ComputeMaxPP (each PP Up adds min(7, base/5)), not the
// Gen III+ base*1.6 rule, so the panel is always given the native maximum.
inline std::array<SharedMovesPanel::Slot, 4> slots(const std::array<uint8_t, 4>& moves,
                                                   const std::array<uint8_t, 4>& pp,
                                                   const std::array<uint8_t, 4>& ppUps,
                                                   const std::array<Status, 4>& status) {
    using PokeVault::Integration::Gen2::StagedEditor;
    std::array<SharedMovesPanel::Slot, 4> out{};
    for (std::size_t i = 0; i < 4; ++i) {
        auto& slot = out[i];
        slot.move = moves[i];
        slot.pp = pp[i];
        slot.ppUps = ppUps[i];
        if (moves[i] == 0) continue;
        slot.maxPP = StagedEditor::gen2MoveMaxPP(moves[i], ppUps[i]);
        const auto view = statusView(status[i]);
        slot.status = view.text;
        slot.statusColor = view.color;
    }
    return out;
}

// Slot under a touch point, or -1. Same rectangles the shared panel outlines.
inline int hitSlot(int px, int py, int panelX, int panelY, int panelW) noexcept {
    const auto l = SharedMovesPanel::layout(panelX, panelY, panelW);
    if (px < l.focusX() || px >= l.focusX() + l.focusW()) return -1;
    for (int slot = 0; slot < 4; ++slot)
        if (py >= l.rowTop(slot) && py < l.rowTop(slot) + SharedMovesPanel::Layout::kRowHeight)
            return slot;
    return -1;
}

inline void draw(PKSEFramebuffer& fb, int panelX, int panelY, int panelW,
                 const std::array<SharedMovesPanel::Slot, 4>& rows, int focusedSlot) {
    SharedMovesPanel::draw(fb, SharedMovesPanel::layout(panelX, panelY, panelW), rows,
                           Enums::GameVersion::GSC, focusedSlot);
}

} // namespace UI::Gen2MovesPanel
