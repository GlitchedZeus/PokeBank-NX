#ifndef NAMES_GEN4_HELD_ITEM_CATALOG_H
#define NAMES_GEN4_HELD_ITEM_CATALOG_H

#include "Enums/GameVersion.h"

#include <cstdint>

namespace Names {

// Exact Generation IV holdable-item structural catalog, pinned to PKHeX
// ItemStorage4DP/GetAllHeld and ItemStorage4Pt/GetAllHeld.
// HGSS uses the same held-item set as Platinum.
//
// DP: 1-4, 6-111, 135-419
// Pt/HGSS: 1-4, 6-112, 135-419
//
// 0 is the explicit "no held item" value and is handled by callers.
constexpr bool isGen4HeldItemPresent(uint16_t itemId,
                                     Enums::GameVersion group) noexcept {
    if (itemId == 0) return false;

    const bool common =
        (itemId >= 1 && itemId <= 4) ||
        (itemId >= 6 && itemId <= 111) ||
        (itemId >= 135 && itemId <= 419);

    switch (group) {
        case Enums::GameVersion::DP:
            return common;
        case Enums::GameVersion::PT:
        case Enums::GameVersion::HGSS:
            return common || itemId == 112; // Griseous Orb
        default:
            return false;
    }
}

constexpr uint16_t gen4MaximumHeldItemId(Enums::GameVersion group) noexcept {
    switch (group) {
        case Enums::GameVersion::DP:
        case Enums::GameVersion::PT:
        case Enums::GameVersion::HGSS:
            return 419;
        default:
            return 0;
    }
}

} // namespace Names

#endif
