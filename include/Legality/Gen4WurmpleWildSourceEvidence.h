#pragma once

#include "Legality/Gen4WildEncounter.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen4WurmpleWildSource {

// This classifier is deliberately bounded to the pinned Gen IV wild-slot table.
// WurmpleOnlyWildSource does NOT mean that all encounter families have been
// excluded; static/gift/trade/event/egg histories are evaluated separately.
enum class Status : uint8_t {
    NotApplicable = 0,
    NoWildFamilySource,
    WurmpleOnlyWildSource,
    AlternateWildSource,
};

struct Result {
    Status status = Status::NotApplicable;
    bool wurmpleCandidate = false;
    uint16_t alternateSourceSpecies = 0;

    constexpr bool wurmpleOnlyWithinWildEvidence() const noexcept {
        return status == Status::WurmpleOnlyWildSource;
    }
};

constexpr bool isEvolvedWurmpleFamily(uint16_t species) noexcept {
    return species >= 266 && species <= 269;
}

constexpr uint16_t cocoonAncestor(uint16_t species) noexcept {
    switch (species) {
        case 267: return 266; // Beautifly <- Silcoon
        case 269: return 268; // Dustox <- Cascoon
        default:  return 0;
    }
}

inline bool wildMatch(std::string_view exactGameId,
                      uint16_t sourceSpecies,
                      uint16_t metLocation,
                      uint8_t metLevel,
                      uint32_t trainerId) noexcept {
    return Gen4Wild::matchesWithTrainerId(
        exactGameId, sourceSpecies, metLocation, metLevel, 0, trainerId);
}

inline Result classify(std::string_view exactGameId,
                       uint16_t currentSpecies,
                       uint16_t metLocation,
                       uint8_t metLevel,
                       uint32_t trainerId) noexcept {
    if (!isEvolvedWurmpleFamily(currentSpecies) ||
        Gen4Wild::gameForId(exactGameId) == Gen4Wild::Game::Invalid ||
        metLevel == 0)
        return {};

    const bool wurmple = wildMatch(
        exactGameId, 265, metLocation, metLevel, trainerId);

    // A direct encounter as the surviving species is always an alternate to a
    // Wurmple-origin history. For Beautifly/Dustox, a directly encountered
    // cocoon at the same persisted met location/level is another valid
    // non-Wurmple source that must block a Wurmple-only conclusion.
    uint16_t alternate = 0;
    if (wildMatch(exactGameId, currentSpecies, metLocation, metLevel, trainerId)) {
        alternate = currentSpecies;
    } else if (const uint16_t cocoon = cocoonAncestor(currentSpecies);
               cocoon != 0 &&
               wildMatch(exactGameId, cocoon, metLocation, metLevel, trainerId)) {
        alternate = cocoon;
    }

    if (alternate != 0)
        return {Status::AlternateWildSource, wurmple, alternate};
    if (wurmple)
        return {Status::WurmpleOnlyWildSource, true, 0};
    return {Status::NoWildFamilySource, false, 0};
}

} // namespace Legality::Gen4WurmpleWildSource
