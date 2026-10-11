#pragma once
// Gen III HM identities used for source-backed Pal Park transfer-edge
// witnesses. An old HM cannot be carried unchanged across this boundary.
// These IDs alone NEVER justify a global legality rejection.
#include <cstdint>
namespace Legality::Gen34PalParkHMIds {
constexpr bool isGen3HM(uint16_t move) noexcept {
    switch(move) {
        case 15:  // Cut
        case 19:  // Fly
        case 57:  // Surf
        case 70:  // Strength
        case 148: // Flash
        case 249: // Rock Smash
        case 127: // Waterfall
        case 291: // Dive
            return true;
        default: return false;
    }
}
}
