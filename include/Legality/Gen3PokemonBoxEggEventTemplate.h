#pragma once

#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3PokemonBoxEggEvent {

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t otGender = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool isEgg = false;
    bool fateful = false;
    std::u16string_view otName{};
    std::array<uint16_t, 4> moves{};
};

constexpr bool isGen3Origin(uint8_t originGame) noexcept {
    return originGame >= 1 && originGame <= 5;
}

constexpr bool expectedMoves(uint16_t species,
                             const std::array<uint16_t, 4>& moves) noexcept {
    switch (species) {
        case 333: return moves == std::array<uint16_t, 4>{64, 45, 206, 0};
        case 263: return moves == std::array<uint16_t, 4>{33, 45, 39, 245};
        case 300: return moves == std::array<uint16_t, 4>{45, 33, 39, 6};
        case 172: return moves == std::array<uint16_t, 4>{84, 204, 57, 0};
        default: return false;
    }
}

constexpr bool matches(const Candidate& c,
                       const Gen3BacdPidIv::Result& rng) noexcept {
    // Pinned PKHeX Pokémon Box recipient eggs:
    // unrestricted regular BACD_U, any valid Gen III stored origin,
    // unhatched level/met-level 0 event egg, event location 255,
    // female OT "ＡＺＵＳＡ", Poke Ball, and one of four fixed movesets.
    // TID/SID and language are recipient-derived/unspecified and are not constrained.
    return rng.variant == Gen3BacdPidIv::Variant::Regular &&
           isGen3Origin(c.originGame) &&
           c.otGender == 1 &&
           c.metLevel == 0 &&
           c.metLocation == 255 &&
           c.ball == 4 &&
           c.isEgg &&
           !c.fateful &&
           c.otName == u"ＡＺＵＳＡ" &&
           expectedMoves(c.species, c.moves);
}

} // namespace Legality::Gen3PokemonBoxEggEvent
