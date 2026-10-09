#pragma once

#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3PcjpFifthEggEvent {

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t language = 0;
    uint8_t otGender = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool isEgg = false;
    bool fateful = false;
    bool shiny = false;
    std::u16string_view otName{};
    std::array<uint16_t, 4> moves{};
};

struct TableResult {
    uint8_t index = 0xFF;
    bool wish = false;
    bool shiny = false;
};

constexpr uint32_t periodicWeight(uint32_t rand, uint32_t max) noexcept {
    const uint32_t high = rand >> 16;
    const uint32_t first = ((high << 2) & 0xFFFFu) + high;
    uint32_t second = ((rand & 0xFFFFu) << 1) + (first >> 16);
    second += high + (second >> 16);
    return (max * (second & 0xFFFFu)) >> 16;
}

constexpr TableResult tableResult(uint16_t seed16) noexcept {
    uint32_t seed = seed16;
    seed = Gen3BacdPidIv::Detail::next(seed);
    const uint32_t hi = seed >> 16;
    seed = Gen3BacdPidIv::Detail::next(seed);
    const uint32_t lo = seed >> 16;
    const uint32_t result = periodicWeight((hi << 16) | lo, 1000u);
    const uint32_t eighth = result / 125u;
    return {
        static_cast<uint8_t>(eighth >> 1),
        (eighth & 1u) != 0,
        (eighth >> 1) == 0 && (result % 125u) >= 100u,
    };
}

constexpr uint8_t speciesIndex(uint16_t species) noexcept {
    switch (species) {
        case 172: return 0; // Pichu
        case 371: return 1; // Bagon
        case 359: return 2; // Absol
        case 280: return 3; // Ralts
        default: return 0xFF;
    }
}

constexpr bool expectedMoves(uint16_t species, bool wish,
                             const std::array<uint16_t, 4>& moves) noexcept {
    switch (species) {
        case 172:
            return moves == (wish
                ? std::array<uint16_t, 4>{84, 204, 273, 0}
                : std::array<uint16_t, 4>{84, 204, 298, 0});
        case 371:
            return moves == (wish
                ? std::array<uint16_t, 4>{99, 44, 273, 0}
                : std::array<uint16_t, 4>{99, 44, 334, 0});
        case 359:
            return moves == (wish
                ? std::array<uint16_t, 4>{10, 43, 273, 0}
                : std::array<uint16_t, 4>{10, 43, 180, 0});
        case 280:
            return moves == (wish
                ? std::array<uint16_t, 4>{45, 273, 0, 0}
                : std::array<uint16_t, 4>{45, 204, 0, 0});
        default:
            return false;
    }
}

constexpr uint32_t tableSeed(const Gen3BacdPidIv::Result& rng) noexcept {
    if (rng.variant != Gen3BacdPidIv::Variant::Regular &&
        rng.variant != Gen3BacdPidIv::Variant::RegularAntiShiny &&
        rng.variant != Gen3BacdPidIv::Variant::ForceShiny)
        return 0x10000u;
    return Gen3BacdPidIv::Detail::prev(
        Gen3BacdPidIv::Detail::prev(rng.originSeed));
}

constexpr bool commonFields(const Candidate& c) noexcept {
    // Pinned PKHeX PCJP Fifth Anniversary eggs are Ruby-origin, Japanese-language
    // unhatched event eggs with male OT "オヤＮＡＭＥ", event location 255 and Poke Ball.
    // TID/SID are unspecified by the source template and must not be constrained.
    return speciesIndex(c.species) != 0xFF &&
           c.originGame == 2 &&
           c.language == 1 &&
           c.otGender == 0 &&
           c.metLevel == 0 &&
           c.metLocation == 255 &&
           c.ball == 4 &&
           c.isEgg &&
           !c.fateful &&
           c.otName == u"オヤＮＡＭＥ";
}

constexpr bool matches(const Candidate& c,
                       const Gen3BacdPidIv::Result& rng) noexcept {
    if (!commonFields(c))
        return false;

    const uint32_t seed = tableSeed(rng);
    if (seed > 0xFFFFu)
        return false;

    const auto result = tableResult(static_cast<uint16_t>(seed));
    if (result.index != speciesIndex(c.species) ||
        result.shiny != c.shiny ||
        !expectedMoves(c.species, result.wish, c.moves))
        return false;

    if (c.shiny)
        return rng.variant == Gen3BacdPidIv::Variant::ForceShiny;

    return rng.variant == Gen3BacdPidIv::Variant::Regular ||
           rng.variant == Gen3BacdPidIv::Variant::RegularAntiShiny;
}

} // namespace Legality::Gen3PcjpFifthEggEvent
