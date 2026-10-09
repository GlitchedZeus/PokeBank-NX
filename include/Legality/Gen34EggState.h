#pragma once

#include <cstdint>

namespace Legality::Gen34EggState {

enum class Evidence : uint8_t {
    NotApplicable,
    Consistent,
    InvalidMetLevel,
    MissingEggLocation,
};

struct Result {
    Evidence evidence = Evidence::NotApplicable;
    bool eggOrigin = false;

    constexpr bool applies() const noexcept {
        return evidence != Evidence::NotApplicable;
    }
    constexpr bool invalid() const noexcept {
        return evidence == Evidence::InvalidMetLevel ||
               evidence == Evidence::MissingEggLocation;
    }
};

// Native PK3/PK4 egg-origin records use met level 0.
// PK3 does not expose a persistent egg-location field after hatching, so only
// unhatched egg state can be positively identified here.
// PK4 keeps EggLocation after hatching, so either IsEgg or EggLocation!=0 is
// positive egg-origin evidence.
constexpr Result analyze(uint8_t generation, bool isEgg,
                         uint16_t eggLocation, uint8_t metLevel) noexcept {
    if (generation == 3) {
        if (!isEgg)
            return {};
        if (metLevel != 0)
            return {Evidence::InvalidMetLevel, true};
        return {Evidence::Consistent, true};
    }

    if (generation == 4) {
        const bool eggOrigin = isEgg || eggLocation != 0;
        if (!eggOrigin)
            return {};
        if (metLevel != 0)
            return {Evidence::InvalidMetLevel, true};
        if (isEgg && eggLocation == 0)
            return {Evidence::MissingEggLocation, true};
        return {Evidence::Consistent, true};
    }

    return {};
}

constexpr const char* evidenceName(Evidence evidence) noexcept {
    switch (evidence) {
        case Evidence::NotApplicable:      return "Not applicable";
        case Evidence::Consistent:         return "Egg state structurally consistent";
        case Evidence::InvalidMetLevel:    return "Egg-origin record has invalid met level";
        case Evidence::MissingEggLocation: return "Unhatched PK4 egg is missing egg location";
    }
    return "Unknown";
}

} // namespace Legality::Gen34EggState
