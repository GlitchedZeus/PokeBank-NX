#pragma once
#include "Enums/GameVersion.h"
#include "UI/SharedMovesPanel.h"

#include <array>
#include <cstdint>

// Gen I / Gen II adapter for the ONE shared MOVES panel (UI/SharedMovesPanel.h). The native
// editors supply only data: the four move ids, current PP, PP Ups, each slot's native maximum
// PP and its compatibility status. Layout, focus outline and columns are the shared panel's, so
// Gen I/II rows sit exactly where Gen III-V rows do, and touch uses the rows the panel outlines.
//
// Gen I/II max PP follows pret/pokered and pret/pokecrystal (each PP Up adds min(7, base/5)), not
// the Gen III+ base*1.6 rule, so callers always pass the native maximum.
namespace UI::ClassicMovesPanel {

enum class Status : uint8_t { Ok, Preserved, Invalid };

struct StatusView { const char* text; Color color; };

constexpr StatusView statusView(Status status) noexcept {
    switch (status) {
        case Status::Ok: return {"OK", Colors::Success};
        case Status::Preserved: return {"Preserved", Colors::Warning};
        default: return {"Invalid", Colors::Error};
    }
}

template <typename MoveId, typename PP, typename Ups, typename MaxPP>
std::array<SharedMovesPanel::Slot, 4> slots(const std::array<MoveId, 4>& moves,
                                            const std::array<PP, 4>& pp,
                                            const std::array<Ups, 4>& ppUps,
                                            const std::array<MaxPP, 4>& maxPP,
                                            const std::array<Status, 4>& status) {
    std::array<SharedMovesPanel::Slot, 4> out{};
    for (std::size_t i = 0; i < 4; ++i) {
        auto& slot = out[i];
        slot.move = static_cast<uint16_t>(moves[i]);
        slot.pp = static_cast<uint8_t>(pp[i]);
        slot.ppUps = static_cast<uint8_t>(ppUps[i]);
        if (slot.move == 0) continue;
        slot.maxPP = static_cast<int>(maxPP[i]);
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

// Touch rectangle of one slot (what callers publish as a touch button).
struct Rect { int x, y, w, h; };
inline Rect slotRect(int slot, int panelX, int panelY, int panelW) noexcept {
    const auto l = SharedMovesPanel::layout(panelX, panelY, panelW);
    return {l.focusX(), l.rowTop(slot), l.focusW(), SharedMovesPanel::Layout::kRowHeight};
}

inline void draw(PKSEFramebuffer& fb, int panelX, int panelY, int panelW,
                 const std::array<SharedMovesPanel::Slot, 4>& rows, Enums::GameVersion game,
                 int focusedSlot) {
    SharedMovesPanel::draw(fb, SharedMovesPanel::layout(panelX, panelY, panelW), rows, game, focusedSlot);
}

} // namespace UI::ClassicMovesPanel
