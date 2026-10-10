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
} // namespace Legality::Gen34FormatDomain
