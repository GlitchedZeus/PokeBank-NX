#pragma once

#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3PokeParkEggEvent {

inline constexpr std::array<uint16_t, 15> kSpecies{{
    54, 172, 174, 222, 276,
    283, 293, 300, 311, 312,
    325, 327, 331, 341, 360,
}};

struct Candidate {
    uint16_t species = 0;
    uint16_t tid = 0;
    uint16_t sid = 0;
    uint8_t originGame = 0;
    uint8_t otGender = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool isEgg = false;
    bool fateful = false;
    std::u16string_view otName{};
};

constexpr bool isPinnedSpecies(uint16_t species) noexcept {
    for (const uint16_t value : kSpecies)
        if (value == species)
            return true;
    return false;
}

constexpr bool matches(const Candidate& c,
                       const Gen3BacdPidIv::Result& rng) noexcept {
    // Pinned PKHeX EncountersWC3 PokéPark DS Download eggs:
    // Ruby origin, unhatched level/met-level 5 event egg, event location 255,
    // TID 50318 / SID 0, OT "ポケパーク", male OT, Poke Ball.
    // They use restricted regular BACD_R. Once hatched, Gen III replaces the
    // trainer/met fields, so this matcher deliberately proves unhatched state only.
    return rng.variant == Gen3BacdPidIv::Variant::Regular &&
           rng.restrictedSeed &&
           isPinnedSpecies(c.species) &&
           c.tid == 50318 && c.sid == 0 &&
           c.originGame == 2 &&
           c.otGender == 0 &&
           c.metLevel == 5 &&
           c.metLocation == 255 &&
           c.ball == 4 &&
           c.isEgg &&
           !c.fateful &&
           c.otName == u"ポケパーク";
}

} // namespace Legality::Gen3PokeParkEggEvent
