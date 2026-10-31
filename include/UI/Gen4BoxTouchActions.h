#pragma once

// Gen IV box touch now selects any real native cell on its first tap.
// Whether that slot opens native Actions is decided by the staged occupancy
// check, not by a second tap or by the inherited generic read-only sheet.
// Non-cell header arrows, name pill and summary remain separate touch targets.
namespace PokeBank::UIModel::Gen4BoxTouchActions {

inline constexpr int SummaryPanelTouchId = 2000;

constexpr bool selectsTappedBoxSlot(int touchedId, int nativeSlotsPerBox) noexcept {
    return nativeSlotsPerBox > 0 &&
           touchedId >= 0 && touchedId < nativeSlotsPerBox;
}

constexpr bool opensSelectedSlotActions(
    int touchedId, int selectedSlot, int nativeSlotsPerBox) noexcept {
    return nativeSlotsPerBox > 0 &&
           selectedSlot >= 0 && selectedSlot < nativeSlotsPerBox &&
           (touchedId == selectedSlot || touchedId == SummaryPanelTouchId);
}

} // namespace PokeBank::UIModel::Gen4BoxTouchActions
