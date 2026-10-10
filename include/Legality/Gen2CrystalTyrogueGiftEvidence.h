#pragma once

#include "Legality/Gen2StaticEncounter.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen2CrystalTyrogueGift {

// Crystal's pinned level-10 Tyrogue static/gift at location 35.
// An evolved Hitmon* can retain the original caught-data record.
// The evolution-time Attack/Defense relationship is not retained
// by the final PK2 record, so this is positive compatibility only.
enum class Evolution : uint8_t {
    Unresolved,
    Hitmonlee,
    Hitmonchan,
    Hitmontop,
};

constexpr Evolution evolution(uint16_t species) noexcept {
    switch (species) {
        case 106: return Evolution::Hitmonlee;
        case 107: return Evolution::Hitmonchan;
        case 237: return Evolution::Hitmontop;
        default: return Evolution::Unresolved;
    }
}

inline Evolution analyze(std::string_view exactSourceGame,
                         uint16_t currentSpecies, uint8_t currentLevel,
                         uint16_t caughtData, bool isEgg, bool isShiny) noexcept {
    const Evolution result = evolution(currentSpecies);
    if (result == Evolution::Unresolved || exactSourceGame != "crystal_gbc" ||
        isEgg || caughtData == 0 || currentLevel < 20)
        return Evolution::Unresolved;

    // A level-10 gift can evolve on a subsequent level-up at 20, but
    // current stats/DVs do not prove the evolution-time stat branch.
    if (!Gen2Static::matches(exactSourceGame, 236, 10,
                             caughtData, false, isShiny))
        return Evolution::Unresolved;
    return result;
}

} // namespace Legality::Gen2CrystalTyrogueGift
