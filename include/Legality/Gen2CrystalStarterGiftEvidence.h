#pragma once
#include "Legality/Gen2StaticEncounter.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen2CrystalStarterGift {
// Positive-only evolved Johto starter ancestry, pinned to Crystal's
// Chikorita/Cyndaquil/Totodile gifts (location 1, level 5).
// No inference about the trainer's selection, trade path, or unique origin.
enum class Ancestor : uint8_t { None, Chikorita, Cyndaquil, Totodile };

constexpr uint16_t originalSpecies(uint16_t current) noexcept {
    switch (current) {
        case 153: case 154: return 152; // Bayleef / Meganium
        case 156: case 157: return 155; // Quilava / Typhlosion
        case 159: case 160: return 158; // Croconaw / Feraligatr
        default: return 0;
    }
}
constexpr uint8_t minimumLevel(uint16_t current) noexcept {
    switch (current) {
        case 153: return 16;
        case 154: return 32;
        case 156: return 14;
        case 157: return 36;
        case 159: return 18;
        case 160: return 30;
        default: return 0;
    }
}
inline Ancestor analyze(std::string_view exactContainerGame,
                        uint16_t currentSpecies, uint8_t currentLevel,
                        uint16_t caughtData, bool isEgg, bool isShiny) noexcept {
    const uint16_t base = originalSpecies(currentSpecies);
    if (exactContainerGame != "crystal_gbc" || !base || isEgg ||
        caughtData == 0 || currentLevel < minimumLevel(currentSpecies))
        return Ancestor::None;

    // Each level-up evolution must occur after receiving the level-5 gift.
    const bool finalStage = currentSpecies == 154 || currentSpecies == 157 ||
                            currentSpecies == 160;
    if (currentLevel < static_cast<unsigned>(5 + (finalStage ? 2 : 1)))
        return Ancestor::None;
    if (!Gen2Static::matches(exactContainerGame, base, 5,
                             caughtData, false, isShiny))
        return Ancestor::None;
    switch (base) {
        case 152: return Ancestor::Chikorita;
        case 155: return Ancestor::Cyndaquil;
        case 158: return Ancestor::Totodile;
        default: return Ancestor::None;
    }
}
} // namespace Legality::Gen2CrystalStarterGift
