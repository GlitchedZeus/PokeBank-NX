#pragma once

#include "Legality/Gen3ColoShadowTeamLock.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3ColoShadowEncounter {

struct Entry {
    uint8_t shadowIndex;
    uint16_t species;
    uint8_t level;
    uint8_t location;
    uint8_t teamSet;
};

#include "Legality/Gen3ColoShadowEncounterData.inc"

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t metLevel = 0;
    uint8_t metLocation = 0;
    bool isEgg = false;
    bool fateful = false;
};

struct MatchList {
    std::array<const Entry*, 2> entries{};
    std::size_t count = 0;

    constexpr bool matched() const noexcept { return count != 0; }
};

constexpr MatchList match(const Candidate& candidate) noexcept {
    MatchList result{};

    // PK3 stores Colosseum and XD under the shared CXD version value 15.
    // Normal Colosseum shadows are not eggs and do not set fateful encounter.
    if (candidate.originGame != 15 || candidate.isEgg || candidate.fateful)
        return result;

    for (const auto& entry : kEntries) {
        if (entry.species != candidate.species ||
            entry.level != candidate.metLevel ||
            entry.location != candidate.metLocation)
            continue;

        // The pinned normal table has at most two persistent-field matches:
        // Murkrow Lv43 @ location 67 (original lock history + later rematch).
        // Preserve both instead of first-match selection.
        if (result.count < result.entries.size())
            result.entries[result.count++] = &entry;
    }
    return result;
}

constexpr Gen3ColoShadowTeamLock::TeamSet teamSet(const Entry& entry) noexcept {
    return static_cast<Gen3ColoShadowTeamLock::TeamSet>(entry.teamSet);
}

inline constexpr std::size_t kEncounterCount = kEntries.size();

} // namespace Legality::Gen3ColoShadowEncounter
