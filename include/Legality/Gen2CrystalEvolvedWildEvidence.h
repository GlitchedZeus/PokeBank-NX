#pragma once
#include "Legality/Gen2WildEncounter.h"
#include <cstdint>

namespace Legality::Gen2CrystalEvolvedWild {

// Bounded positive-only evidence from actual pinned Crystal wild rows.
// Crystal preserves caught-data location, encounter level and time after
// evolution. No assumptions about eggs, events, trades or unavailable
// evolution families: unsupported provenance stays Incomplete/Unresolved.
enum class Ancestor : uint16_t { None=0, Pidgey=16, Tentacool=72 };

constexpr uint16_t requiredBase(uint16_t current) noexcept {
    switch (current) {
        case 17: case 18: return 16; // Pidgeotto/Pidgeot
        case 73: return 72;          // Tentacruel
        default: return 0;
    }
}
constexpr uint8_t minimumEvolvedLevel(uint16_t current) noexcept {
    switch (current) {
        case 17: return 18; // Pidgeotto
        case 18: return 36; // Pidgeot
        case 73: return 30; // Tentacruel
        default: return 0;
    }
}
inline Ancestor analyze(uint16_t current, uint8_t currentLevel,
                        uint16_t caughtData, bool isEgg) noexcept {
    const uint16_t base=requiredBase(current);
    const uint8_t evoLevel=minimumEvolvedLevel(current);
    if (isEgg || !base || currentLevel<evoLevel || caughtData==0)
        return Ancestor::None;
    const uint8_t metLevel=static_cast<uint8_t>((caughtData>>8)&0x3Fu);
    if (metLevel==0 || metLevel>currentLevel)
        return Ancestor::None;
    if (!Gen2Wild::matchesCrystalCaughtData(base,caughtData))
        return Ancestor::None;
    return base==16 ? Ancestor::Pidgey : Ancestor::Tentacool;
}
} // namespace Legality::Gen2CrystalEvolvedWild
