#pragma once
// Gen IV Ability picker rules (owner Oct 28 PID-route review). The Ability row shows the ability the
// record STORES; the picker used to open on the PID's slot instead, so pressing A on the preselected
// row of a record whose stored ability disagrees with its PID rewrote the stored ability. The picker
// now opens on the slot holding the stored ability, and choosing it changes nothing.
// Works on any record type with abilitySlot(), ability() and abilityForSlot(slot) (Pokemon4Mutable).

#include <cstdint>

namespace PokeVault::Integration::Gen4 {

template <class Record>
[[nodiscard]] bool abilityChoiceKeepsStored(const Record& r, uint8_t slot) noexcept {
    return slot <= 1 && r.abilityForSlot(slot) != 0 && r.abilityForSlot(slot) == r.ability();
}

template <class Record>
[[nodiscard]] uint8_t abilityPickerRow(const Record& r) noexcept {
    const uint8_t pidSlot = r.abilitySlot();
    if (pidSlot <= 1 && abilityChoiceKeepsStored(r, pidSlot)) return pidSlot;
    for (uint8_t slot = 0; slot <= 1; ++slot)
        if (abilityChoiceKeepsStored(r, slot)) return slot;
    return pidSlot <= 1 ? pidSlot : 0;
}

}  // namespace PokeVault::Integration::Gen4
