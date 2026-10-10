#pragma once
#include "Legality/Gen2StaticEncounter.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen2CrystalDratiniGift {

// Crystal's pinned Dragon's Den Dratini gift: species 147,
// received at level 15, caught-data location 42. Informational
// compatibility only; preserved PK2 data cannot prove receipt,
// trainer ownership, or trade/evolution chronology.
enum class Evidence : uint8_t { Unresolved, Dragonair, Dragonite };

constexpr Evidence stage(uint16_t current) noexcept {
    switch (current) {
        case 148: return Evidence::Dragonair;
        case 149: return Evidence::Dragonite;
        default: return Evidence::Unresolved;
    }
}

inline Evidence analyze(std::string_view exactSourceGame,
                        uint16_t currentSpecies, uint8_t currentLevel,
                        uint16_t caughtData, bool isEgg, bool isShiny) noexcept {
    const auto match = stage(currentSpecies);
    if (match == Evidence::Unresolved || exactSourceGame != "crystal_gbc" ||
        isEgg || caughtData == 0)
        return Evidence::Unresolved;
    // Dratini evolves by level-up at 30, then Dragonair at 55.
    // Both must happen after receiving the level-15 gift.
    const uint8_t evolutionLevel =
        match == Evidence::Dragonair ? 30 : 55;
    if (currentLevel < evolutionLevel) return Evidence::Unresolved;
    if (!Gen2Static::matches(exactSourceGame, 147, 15,
                             caughtData, false, isShiny))
        return Evidence::Unresolved;
    return match;
}

} // namespace Legality::Gen2CrystalDratiniGift
