#pragma once
#include "Legality/Gen2WildEncounter.h"
#include <cstdint>

namespace Legality::Gen2CrystalEvolvedWild {

// Bounded positive-only evidence from actual pinned Crystal wild rows.
// Crystal preserves caught-data location, encounter level and time after
// evolution. No assumptions about eggs, events, trades or unavailable
// evolution families: unsupported provenance stays Incomplete/Unresolved.
enum class Ancestor : uint16_t {
    None=0, Caterpie=10, Weedle=13, Pidgey=16,
    Rattata=19, Spearow=21, Zubat=41, Tentacool=72, Sentret=161
};

constexpr uint16_t requiredBase(uint16_t current) noexcept {
    switch (current) {
        case 11: case 12: return 10;  // Metapod/Butterfree
        case 14: case 15: return 13;  // Kakuna/Beedrill
        case 17: case 18: return 16;  // Pidgeotto/Pidgeot
        case 20: return 19;           // Raticate
        case 22: return 21;           // Fearow
        case 42: return 41;           // Golbat
        case 162: return 161;         // Furret
        case 73: return 72;          // Tentacruel
        default: return 0;
    }
}
constexpr uint8_t minimumEvolvedLevel(uint16_t current) noexcept {
    switch (current) {
        case 11: case 14: return 7;  // first cocoon evolution
        case 12: case 15: return 10; // two-stage Butterfree/Beedrill
        case 17: return 18; // Pidgeotto
        case 20: case 22: return 20; // Raticate/Fearow
        case 42: return 22;          // Golbat
        case 162: return 15;         // Furret
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
    // Level-based evolutions require distinct level-up events *after*
    // capture. Capturing a base Pokémon at its evolution threshold does
    // not instantly evolve it, and reaching the second stage consumes a
    // separate level-up. This remains bounded positive evidence.
    const bool twoStage =
        current==12 || current==15 || current==18;
    const uint8_t steps=twoStage ? 2 : 1;
    if (static_cast<unsigned>(currentLevel) <
        static_cast<unsigned>(metLevel)+steps)
        return Ancestor::None;
    if (!Gen2Wild::matchesCrystalCaughtData(base,caughtData))
        return Ancestor::None;
    return static_cast<Ancestor>(base);
}
} // namespace Legality::Gen2CrystalEvolvedWild
