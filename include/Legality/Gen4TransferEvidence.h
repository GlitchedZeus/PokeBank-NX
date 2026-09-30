#pragma once

#include <cstdint>

namespace Legality::Gen4Transfer {

inline constexpr uint16_t kPalParkMetLocation = 0x37;

enum class Evidence : uint8_t {
    NotApplicable,
    PalParkMarker,
    PalParkDiamondPearlFields,
    PalParkPtHgssFields,
    InvalidEggTransfer,
    InvalidMissingPalParkMarker,
    InvalidSplitLocationFields,
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

constexpr Evidence classifyStoredFields(uint8_t originGeneration,
                                        uint16_t metLocationDP,
                                        uint16_t metLocationExtended,
                                        bool isEgg) noexcept {
    if (originGeneration != 3)
        return Evidence::NotApplicable;
    if (isEgg)
        return Evidence::InvalidEggTransfer;

    // PKHeX MiscVerifierG4 / TransferVerifier:
    // - D/P Pal Park: DP field = 0x37, extended field remains zero.
    // - Pt/HGSS Pal Park: DP + extended fields both carry the Pal Park value.
    if (metLocationExtended == 0)
        return metLocationDP == kPalParkMetLocation
            ? Evidence::PalParkDiamondPearlFields
            : Evidence::InvalidMissingPalParkMarker;

    if (metLocationDP == kPalParkMetLocation &&
        metLocationExtended == kPalParkMetLocation)
        return Evidence::PalParkPtHgssFields;

    return Evidence::InvalidSplitLocationFields;
}

constexpr bool invalid(Evidence evidence) noexcept {
    return evidence == Evidence::InvalidEggTransfer ||
           evidence == Evidence::InvalidMissingPalParkMarker ||
           evidence == Evidence::InvalidSplitLocationFields;
}

constexpr const char* evidenceName(Evidence evidence) noexcept {
    switch (evidence) {
        case Evidence::NotApplicable: return "not applicable";
        case Evidence::PalParkMarker: return "Gen III -> IV Pal Park transfer marker";
        case Evidence::PalParkDiamondPearlFields: return "Gen III -> IV Pal Park D/P split-field pattern";
        case Evidence::PalParkPtHgssFields: return "Gen III -> IV Pal Park Pt/HGSS split-field pattern";
        case Evidence::InvalidEggTransfer: return "Gen III-origin egg cannot be transferred through Pal Park";
        case Evidence::InvalidMissingPalParkMarker: return "Gen III-origin PK4 is missing the Pal Park transfer met location";
        case Evidence::InvalidSplitLocationFields: return "Gen III-origin PK4 has inconsistent D/P and Pt/HGSS Pal Park location fields";
    }
    return "unknown";
}

} // namespace Legality::Gen4Transfer
