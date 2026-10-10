#pragma once
#include "Enums/GameVersion.h"
#include <cstdint>

namespace Legality::Gen34FormatDomain {

// A PKM's immutable *storage format* can be known even if its containing
// save and acquisition game are not. FRLG is the shared PK3 format group;
// DP/PT/HGSS are stored encrypted PK4. These generation-wide domain
// limits are independent of wild/gift/trade/event provenance.
constexpr uint8_t generationFromGroup(Enums::GameVersion group) noexcept {
    switch (group) {
        case Enums::GameVersion::FRLG:
        case Enums::GameVersion::FR:
        case Enums::GameVersion::LG:
            return 3;
        case Enums::GameVersion::DP:
        case Enums::GameVersion::PT:
        case Enums::GameVersion::HGSS:
        case Enums::GameVersion::D:
        case Enums::GameVersion::P:
        case Enums::GameVersion::Pt:
        case Enums::GameVersion::HG:
        case Enums::GameVersion::SS:
            return 4;
        default:
            return 0;
    }
}
// Pinned PKHeX and project SourceGameProfile generation ceilings.
constexpr uint16_t maxSpecies(uint8_t gen) noexcept {
    return gen==3 ? 386 : gen==4 ? 493 : 0;
}
constexpr uint16_t maxMove(uint8_t gen) noexcept {
    return gen==3 ? 354 : gen==4 ? 467 : 0;
}
constexpr bool impossibleSpecies(uint8_t gen,uint16_t species) noexcept {
    return maxSpecies(gen)!=0 && species>maxSpecies(gen);
}
constexpr bool impossibleMove(uint8_t gen,uint16_t move) noexcept {
    return maxMove(gen)!=0 && move>maxMove(gen);
}
} // namespace Legality::Gen34FormatDomain
