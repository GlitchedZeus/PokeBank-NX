#pragma once

#include <array>
#include <cstdint>

namespace Legality::Gen4Hatch {

inline constexpr uint8_t MaskDP = 1u << 0;
inline constexpr uint8_t MaskPt = 1u << 1;
inline constexpr uint8_t MaskHGSS = 1u << 2;
inline constexpr uint8_t MaskAll = MaskDP | MaskPt | MaskHGSS;
inline constexpr uint16_t LinkTrade4 = 2002;

// Pinned PKHeX EggHatchLocation4.LocationPermitted4 table.
// Each entry is a bitmask: DP=1, Pt=2, HGSS=4.
inline constexpr std::array<uint8_t, 235> LocationPermitted = {
    0, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 2, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 2,
    7, 3, 3, 3, 3, 2, 0, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 6, 6, 6, 6, 6, 2, 6, 2,
    2, 2, 2, 2, 2, 2, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 0, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 0, 4,
};

constexpr uint8_t maskForOriginVersion(uint8_t version) noexcept {
    switch (version) {
        case 10: // Diamond
        case 11: // Pearl
            return MaskDP;
        case 12: // Platinum
            return MaskPt;
        case 7:  // HeartGold
        case 8:  // SoulSilver
            return MaskHGSS;
        default:
            return 0;
    }
}

constexpr bool hasMask(uint16_t location, uint8_t mask) noexcept {
    return location < LocationPermitted.size() &&
           (LocationPermitted[location] & mask) != 0;
}

constexpr bool isValidForOrigin(uint16_t location,
                                uint8_t originVersion) noexcept {
    const uint8_t mask = maskForOriginVersion(originVersion);
    return mask != 0 && hasMask(location, mask);
}

constexpr bool isValidAny(uint16_t location) noexcept {
    return hasMask(location, MaskAll);
}

// PKHeX's Gen IV egg verifier checks the stored origin version for ordinary
// eggs. A Link Trade egg can hatch in another Gen IV game without changing its
// stored origin version, so that marker permits any Gen IV hatch location.
constexpr bool isValidHatchedEgg(uint8_t originVersion,
                                 uint16_t eggLocation,
                                 uint16_t metLocation) noexcept {
    if (eggLocation == LinkTrade4)
        return isValidAny(metLocation);
    return isValidForOrigin(metLocation, originVersion);
}

} // namespace Legality::Gen4Hatch
