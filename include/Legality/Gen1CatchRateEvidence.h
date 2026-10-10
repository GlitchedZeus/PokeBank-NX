#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen1CatchRate {

enum class Evidence : uint8_t {
    Unknown,
    NativeSpeciesRate,
    Gen1SpeciesOrPreEvolutionRate,
    AmbiguousGen1OrTimeCapsuleHeldItem,
    PossibleTimeCapsuleHeldItem,
};

#include "Legality/Gen1CatchRateData.inc"

constexpr bool isGen1Game(std::string_view exactGameId) noexcept {
    return exactGameId == "red_gb" || exactGameId == "blue_gb" ||
           exactGameId == "yellow_gb";
}

constexpr uint8_t expectedRate(std::string_view exactGameId, uint16_t species) noexcept {
    if (!isGen1Game(exactGameId) || species == 0 || species > 151)
        return 0;
    return exactGameId == "yellow_gb"
        ? kCatchRateY[species]
        : kCatchRateRB[species];
}

// PKHeX ItemConverter.IsCatchRateHeldItem @ the pinned reference.
// A PK1 catch-rate byte can become a Gen II held-item byte after Time Capsule tradeback.
inline constexpr std::array<uint8_t, 32> kCatchRateIsHeldItem{{
    0x3F, 0xFF, 0xFF, 0xFD, 0xFF, 0xDF, 0x3B, 0xD2,
    0x03, 0xFF, 0xFF, 0xFB, 0xEF, 0xFF, 0xE7, 0x7E,
    0x18, 0x9C, 0xC5, 0xF1, 0xFB, 0x77, 0xF0, 0xBF,
    0xF7, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0x07, 0x00,
}};

constexpr bool isPossibleTimeCapsuleHeldItem(uint8_t rate) noexcept {
    return (kCatchRateIsHeldItem[rate >> 3] &
            static_cast<uint8_t>(1u << (rate & 7u))) != 0;
}

constexpr uint8_t evolutionStage(uint16_t species) noexcept {
    return species < kEvolutionStage.size() ? kEvolutionStage[species] : 0;
}

// Graph-aware original ancestor. The pinned stage offset table is a
// linear-chain aid, NOT a complete evolution graph: all three Gen I
// Eevee branches descend directly from #133, not from one another.
constexpr uint16_t originalGen1Ancestor(uint16_t species) noexcept {
    if (species == 0 || species > 151) return 0;
    if (species >= 134 && species <= 136) return 133;
    return static_cast<uint16_t>(species - evolutionStage(species));
}

constexpr bool matchesGen1SpeciesOrPreEvolutionRate(
    uint16_t species, uint8_t catchRate) noexcept {
    if (species == 0 || species > 151)
        return false;
    // Eevee is a BRANCHED evolution family. The PKHeX-derived stage table
    // alone has numeric neighbors, not graph edges: subtracting stage(135)
    // accidentally selected Vaporeon as Jolteon's parent, and stage(136)
    // selected Jolteon as Flareon's parent. Neither can evolve into the
    // other; only Eevee and the current branch are valid ancestors.
    if (species >= 134 && species <= 136)
        return catchRate == kCatchRateRB[133] ||
               catchRate == kCatchRateY[133] ||
               catchRate == kCatchRateRB[species] ||
               catchRate == kCatchRateY[species];

    // The remaining native Gen I evolutions are sequential, linear chains;
    // use the PKHeX-pinned stage table for their actual pre-evolutions.
    const uint16_t base = originalGen1Ancestor(species);
    for (uint16_t s = base; s <= species; ++s) {
        if (catchRate == kCatchRateRB[s] || catchRate == kCatchRateY[s])
            return true;
    }
    return false;
}

constexpr Evidence classify(std::string_view exactGameId, uint16_t species,
                            uint8_t catchRate) noexcept {
    if (!isGen1Game(exactGameId) || species == 0 || species > 151)
        return Evidence::Unknown;

    const bool native = catchRate == expectedRate(exactGameId, species);
    const bool gen1Rate = matchesGen1SpeciesOrPreEvolutionRate(species, catchRate);
    const bool item = isPossibleTimeCapsuleHeldItem(catchRate);

    // The catch-rate byte alone cannot distinguish these cases. Mirror that ambiguity
    // instead of claiming the Pokemon definitely was or was not traded through Gen II.
    if (gen1Rate && item)
        return Evidence::AmbiguousGen1OrTimeCapsuleHeldItem;
    if (native)
        return Evidence::NativeSpeciesRate;
    if (gen1Rate)
        return Evidence::Gen1SpeciesOrPreEvolutionRate;
    if (item)
        return Evidence::PossibleTimeCapsuleHeldItem;
    return Evidence::Unknown;
}

} // namespace Legality::Gen1CatchRate
