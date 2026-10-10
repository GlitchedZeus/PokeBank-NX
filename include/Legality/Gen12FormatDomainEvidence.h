#pragma once
#include "Enums/GameVersion.h"
#include <cstdint>

namespace Legality::Gen12FormatDomain {

// Only the native, immutable read-only PK1/PK2 wrappers prove these
// format generations when the physical save/source game is missing.
// No historical R/B/Y versus Gold/Silver/Crystal acquisition is inferred.
constexpr uint8_t generationFromGroup(Enums::GameVersion group) noexcept {
    switch(group){
        case Enums::GameVersion::RBY: return 1;
        case Enums::GameVersion::GSC: return 2;
        default: return 0;
    }
}
constexpr uint16_t maxSpecies(uint8_t generation) noexcept {
    return generation==1 ? 151 : generation==2 ? 251 : 0;
}
constexpr uint16_t maxMove(uint8_t generation) noexcept {
    return generation==1 ? 165 : generation==2 ? 251 : 0;
}
} // namespace Legality::Gen12FormatDomain
