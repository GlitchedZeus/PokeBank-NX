#pragma once

// Gen IV box touch entry must intercept only a tap on the already-focused slot
// or the summary panel. A tap on another cell belongs to the base box cursor,
// while arrows/header must retain their existing navigation/rename behavior.
namespace PokeBank::UIModel::Gen4BoxTouchActions {

inline constexpr int SummaryPanelTouchId = 2000;

constexpr bool opensSelectedSlotActions(
    int touchedId, int selectedSlot, int nativeSlotsPerBox) noexcept {
    return nativeSlotsPerBox > 0 &&
           selectedSlot >= 0 && selectedSlot < nativeSlotsPerBox &&
           (touchedId == selectedSlot || touchedId == SummaryPanelTouchId);
}

} // namespace PokeBank::UIModel::Gen4BoxTouchActions
