#pragma once

#include <cstdint>

namespace Legality::Gen4Transfer {

inline constexpr uint16_t kPalParkMetLocation = 0x37;

enum class Evidence : uint8_t {
    NotApplicable,
    PalParkMarker,
    InvalidEggTransfer,
    InvalidMissingPalParkMarker,
};

constexpr Evidence classify(uint8_t originGeneration,
                            uint16_t metLocation,
                            bool isEgg) noexcept {
    if (originGeneration != 3)
        return Evidence::NotApplicable;
    if (isEgg)
        return Evidence::InvalidEggTransfer;
    if (metLocation != kPalParkMetLocation)
        return Evidence::InvalidMissingPalParkMarker;
    return Evidence::PalParkMarker;
}

constexpr bool invalid(Evidence evidence) noexcept {
    return evidence == Evidence::InvalidEggTransfer ||
           evidence == Evidence::InvalidMissingPalParkMarker;
}

constexpr const char* evidenceName(Evidence evidence) noexcept {
    switch (evidence) {
        case Evidence::NotApplicable: return "not applicable";
        case Evidence::PalParkMarker: return "Gen III -> IV Pal Park transfer marker";
        case Evidence::InvalidEggTransfer: return "Gen III-origin egg cannot be transferred through Pal Park";
        case Evidence::InvalidMissingPalParkMarker: return "Gen III-origin PK4 is missing the Pal Park transfer met location";
    }
    return "unknown";
}

} // namespace Legality::Gen4Transfer
