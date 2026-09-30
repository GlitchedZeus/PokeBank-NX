#pragma once

#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen4WildEncounter.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen4WildRng {

enum class Method : uint8_t {
    None,
    MethodJNoLead,
    MethodKNoLead,
};

struct Result {
    Method method = Method::None;
    uint32_t encounterSeed = 0;
    uint8_t slot = 0;

    constexpr bool matched() const noexcept { return method != Method::None; }
};

constexpr uint8_t regularSlot(uint32_t roll) noexcept {
    return roll < 20 ? 0 :
           roll < 40 ? 1 :
           roll < 50 ? 2 :
           roll < 60 ? 3 :
           roll < 70 ? 4 :
           roll < 80 ? 5 :
           roll < 85 ? 6 :
           roll < 90 ? 7 :
           roll < 94 ? 8 :
           roll < 98 ? 9 :
           roll < 99 ? 10 :
           roll == 99 ? 11 : 0xFF;
}

constexpr uint8_t surfSlot(uint32_t roll) noexcept {
    return roll < 60 ? 0 :
           roll < 90 ? 1 :
           roll < 95 ? 2 :
           roll < 99 ? 3 :
           roll == 99 ? 4 : 0xFF;
}

constexpr uint32_t sequentialPid(uint32_t seed) noexcept {
    seed = Gen3PidIv::Detail::next(seed);
    const uint32_t low = seed >> 16;
    seed = Gen3PidIv::Detail::next(seed);
    const uint32_t high = seed >> 16;
    return (high << 16) | low;
}

constexpr int reversalWindow(uint32_t seed, uint8_t nature) noexcept {
    int count = 0;
    uint32_t upper = seed >> 16;
    // PKHeX MethodJ.GetReversalWindow: step backward over earlier PID attempts
    // until the prior sequential PID shares the final nature.
    for (; count < 128; ++count) {
        seed = Gen3PidIv::Detail::prev(seed);
        const uint32_t lower = seed >> 16;
        const uint32_t pid = (upper << 16) | lower;
        if ((pid % 25u) == nature)
            return count;
        seed = Gen3PidIv::Detail::prev(seed);
        upper = seed >> 16;
    }
    return -1;
}

constexpr uint8_t methodJSlot(uint8_t encounterType, uint16_t rand16) noexcept {
    const uint32_t roll = rand16 / 656u;
    if (encounterType == 0) return regularSlot(roll); // Grass
    if (encounterType == 1) return surfSlot(roll);    // Surf
    return 0xFF;
}

constexpr uint8_t methodKSlot(uint8_t encounterType, uint16_t rand16) noexcept {
    const uint32_t roll = rand16 % 100u;
    if (encounterType == 0) return regularSlot(roll); // Grass
    if (encounterType == 1) return surfSlot(roll);    // Surf
    return 0xFF;
}

constexpr uint8_t randomLevel(uint8_t minimum, uint8_t maximum,
                              uint16_t rand16) noexcept {
    const uint32_t width = 1u + static_cast<uint32_t>(maximum) - minimum;
    return static_cast<uint8_t>((rand16 % width) + minimum);
}

// Conservative positive matcher for the straightforward no-lead wild paths only.
// Supported now:
//   D/P/Pt Method J: Grass, Surf
//   HG/SS  Method K: Grass, Surf
// Fishing/Honey Tree/Rock Smash/Headbutt/BCC/Safari and all lead-ability branches
// deliberately return unresolved rather than pretending to be invalid.
constexpr Result matchNoLeadRow(bool hgss, uint64_t row,
                                uint32_t prePidSeed, uint32_t pid,
                                uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (type > 1)
        return {};

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    const int frames = reversalWindow(prePidSeed, nature);
    if (frames < 0)
        return {};

    uint32_t candidate = prePidSeed;
    for (int i = 0; i <= frames; ++i) {
        const uint16_t natureRand = static_cast<uint16_t>(candidate >> 16);
        const uint32_t rolledNature = hgss
            ? (natureRand % 25u)
            : (natureRand / 0x0A3Eu);

        if (rolledNature == nature) {
            const uint32_t seed1 = Gen3PidIv::Detail::prev(candidate);
            const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
            const uint16_t prev1 = static_cast<uint16_t>(seed1 >> 16);
            const uint16_t prev2 = static_cast<uint16_t>(seed2 >> 16);

            const uint8_t rolledSlot = hgss
                ? methodKSlot(type, type == 0 ? prev1 : prev2)
                : methodJSlot(type, type == 0 ? prev1 : prev2);
            if (rolledSlot == Gen4Wild::slot(row)) {
                if (type == 0) {
                    if (Gen4Wild::levelMatches(row, metLevel))
                        return {hgss ? Method::MethodKNoLead : Method::MethodJNoLead,
                                candidate, rolledSlot};
                } else {
                    const uint8_t level =
                        randomLevel(Gen4Wild::minLevel(row), Gen4Wild::maxLevel(row), prev1);
                    if (level == metLevel)
                        return {hgss ? Method::MethodKNoLead : Method::MethodJNoLead,
                                candidate, rolledSlot};
                }
            }
        }

        candidate = Gen3PidIv::Detail::prev(
            Gen3PidIv::Detail::prev(candidate));
    }
    return {};
}

inline Result analyzeNoLead(std::string_view exactGameId, uint16_t speciesId,
                            uint16_t metLocation, uint8_t metLevel,
                            uint8_t pokemonForm, uint32_t prePidSeed,
                            uint32_t pid) noexcept {
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || speciesId == 0 ||
        metLocation > 0xFF || metLevel == 0)
        return {};

    const bool hgss =
        wanted == Gen4Wild::Game::HeartGold ||
        wanted == Gen4Wild::Game::SoulSilver;

    for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
        if (Gen4Wild::game(row) != wanted ||
            Gen4Wild::species(row) != speciesId ||
            Gen4Wild::location(row) != metLocation ||
            !Gen4Wild::levelMatches(row, metLevel) ||
            !Gen4Wild::formMatches(Gen4Wild::form(row), pokemonForm))
            continue;

        const auto result = matchNoLeadRow(
            hgss, row, prePidSeed, pid, metLevel);
        if (result.matched())
            return result;
    }
    return {};
}

constexpr const char* methodName(Method method) noexcept {
    switch (method) {
        case Method::MethodJNoLead: return "Method J (no lead)";
        case Method::MethodKNoLead: return "Method K (no lead)";
        case Method::None: break;
    }
    return "No no-lead Method J/K match";
}

} // namespace Legality::Gen4WildRng
