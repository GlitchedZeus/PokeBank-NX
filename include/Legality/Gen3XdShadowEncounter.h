#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3XdShadowEncounter {

struct Entry {
    uint8_t index;
    uint16_t species;
    uint8_t level;
    uint16_t location;
    uint8_t fixedBall;
};

#include "Legality/Gen3XdShadowEncounterData.inc"

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool isEgg = false;
    bool fateful = false;
    bool shiny = false;
};

struct MatchResult {
    bool matched = false;
    uint8_t index = 0;
    bool rebattleLocation = false;
    bool fixedBall = false;
};

constexpr bool isRebattleLocation(uint16_t location) noexcept {
    // Pinned EncounterShadow3XD::IsMatchLocation permits these Miror B.
    // recapture/rebattle locations in addition to each encounter's own location.
    return location == 59 || location == 90 || location == 91 ||
           location == 92 || location == 113;
}

constexpr MatchResult match(const Candidate& c) noexcept {
    // PKHeX's Gen III GameVersion.CXD stored version value is 15.
    // This is positive XD shadow-template evidence only. A non-match is not proof
    // of illegality because recursive team-lock and shiny-skip history is separate.
    if (c.originGame != 15 || c.isEgg || !c.fateful || c.shiny)
        return {};

    for (const auto& row : kEntries) {
        if (row.species != c.species || row.level != c.metLevel)
            continue;

        const bool rebattle =
            row.location != c.metLocation && isRebattleLocation(c.metLocation);
        if (row.location != c.metLocation && !rebattle)
            continue;

        if (row.fixedBall != 0 && c.ball != row.fixedBall)
            continue;

        return {true, row.index, rebattle, row.fixedBall != 0};
    }
    return {};
}

inline constexpr std::size_t kEncounterCount = kEntries.size();

} // namespace Legality::Gen3XdShadowEncounter
