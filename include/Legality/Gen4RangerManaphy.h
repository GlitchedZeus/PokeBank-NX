#pragma once

#include <cstdint>

namespace Legality::Gen4RangerManaphy {

inline constexpr uint16_t SpeciesManaphy = 490;
inline constexpr uint16_t LocationLinkTrade4 = 2002;
inline constexpr uint16_t LocationRanger4 = 3001;
inline constexpr uint8_t BallPoke = 4;

struct Candidate {
    uint16_t species = 0;
    uint8_t language = 0;
    uint8_t gender = 0;
    bool isEgg = false;
    uint16_t eggLocation = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool fateful = false;
};

constexpr bool validRangerLanguage(uint8_t language) noexcept {
    // Ranger Manaphy creation disallows unused 0/6 and Korean/future language values.
    return language >= 1 && language <= 7 && language != 6;
}

constexpr bool matches(const Candidate& c) noexcept {
    if (c.species != SpeciesManaphy || !validRangerLanguage(c.language))
        return false;
    if (c.gender != 2 || c.ball != BallPoke || !c.fateful)
        return false;

    if (c.isEgg)
        return c.eggLocation == LocationRanger4 &&
               (c.metLocation == 0 || c.metLocation == LocationLinkTrade4);

    // A hatched Ranger Manaphy either retains Ranger provenance directly or
    // records that it crossed a link trade while still an egg.
    return c.eggLocation == LocationRanger4 ||
           c.eggLocation == LocationLinkTrade4;
}

} // namespace Legality::Gen4RangerManaphy
