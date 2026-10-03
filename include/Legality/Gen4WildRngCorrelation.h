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
    MethodJFishingNoLead,
    MethodKFishingNoLead,
    MethodKFishingSuctionCups,
    MethodKHeadbuttNoLead,
    MethodJHoneyTreeNoLead,
    MethodKRockSmashNoLead,
    MethodKRockSmashIlluminate,
    MethodKBugContestNoLead,
    MethodKSafariNoLead,
    MethodKSafariFishingNoLead,
    MethodKSafariFishingSuctionCups,
    MethodJSynchronize,
    MethodKSynchronize,
};

struct Result {
    Method method = Method::None;
    uint32_t encounterSeed = 0;
    uint8_t slot = 0;

    constexpr bool matched() const noexcept { return method != Method::None; }
};

enum class Activation : uint8_t {
    None,
    Normal,
    SuctionCups,
    Illuminate,
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

constexpr uint8_t superRodSlotJ(uint32_t roll) noexcept {
    return roll < 40 ? 0 :
           roll < 80 ? 1 :
           roll < 95 ? 2 :
           roll < 99 ? 3 :
           roll == 99 ? 4 : 0xFF;
}

constexpr uint8_t superRodSlotK(uint32_t roll) noexcept {
    return roll < 40 ? 0 :
           roll < 70 ? 1 :
           roll < 85 ? 2 :
           roll < 95 ? 3 :
           roll < 100 ? 4 : 0xFF;
}

constexpr bool isSafari(uint8_t type) noexcept {
    return type >= 10 && type <= 14;
}

constexpr bool isSafariFishing(uint8_t type) noexcept {
    return type >= 12 && type <= 14;
}

constexpr bool isFishing(uint8_t type) noexcept {
    return (type >= 2 && type <= 4) || isSafariFishing(type);
}

constexpr bool isHeadbutt(uint8_t type) noexcept {
    return type == 6 || type == 7;
}

constexpr bool isHoneyTree(uint8_t type) noexcept {
    return type == 9;
}

constexpr uint8_t honeyTreeLevel(uint16_t rand16) noexcept {
    // PKHeX MethodJ.GetHoneyTreeLevel: 5 + rand / 0x1745.
    return static_cast<uint8_t>(5u + (rand16 / 0x1745u));
}

constexpr uint8_t rockSmashSlot(uint16_t rand16) noexcept {
    return (rand16 % 100u) < 80u ? 0 : 1;
}

constexpr bool feebasTileReplacement(uint16_t rand16) noexcept {
    // PKHeX MethodJ.IsFeebasChance: upper bit set means the Coronet tile
    // replacement branch can produce Feebas when the player is on a valid tile.
    return (rand16 >> 15) == 1u;
}

constexpr Activation rockSmashActivationKind(uint8_t areaRate,
                                                uint16_t rand16) noexcept {
    if (areaRate == 0) return Activation::None;
    const uint32_t roll = rand16 % 100u;
    if (roll < areaRate)
        return Activation::Normal;
    if (roll < static_cast<uint32_t>(areaRate) * 2u)
        return Activation::Illuminate;
    return Activation::None;
}

constexpr bool rockSmashActivation(uint8_t areaRate, uint16_t rand16) noexcept {
    return rockSmashActivationKind(areaRate, rand16) != Activation::None;
}

constexpr uint8_t headbuttSlot(uint16_t rand16) noexcept {
    const uint32_t roll = rand16 % 100u;
    return roll < 50 ? 0 :
           roll < 65 ? 1 :
           roll < 80 ? 2 :
           roll < 90 ? 3 :
           roll < 95 ? 4 :
           roll < 100 ? 5 : 0xFF;
}

constexpr uint8_t bugContestSlot(uint16_t rand16) noexcept {
    const uint32_t roll = rand16 % 100u;
    return roll < 5 ? 9 :
           roll < 10 ? 8 :
           roll < 15 ? 7 :
           roll < 20 ? 6 :
           roll < 30 ? 5 :
           roll < 40 ? 4 :
           roll < 50 ? 3 :
           roll < 60 ? 2 :
           roll < 80 ? 1 :
           roll < 100 ? 0 : 0xFF;
}

constexpr uint8_t safariSlot(uint16_t rand16) noexcept {
    return static_cast<uint8_t>(rand16 % 10u);
}

constexpr bool hasAny31IvWord(uint16_t word) noexcept {
    return (word & 0x1Fu) == 31u ||
           ((word >> 5) & 0x1Fu) == 31u ||
           ((word >> 10) & 0x1Fu) == 31u;
}

constexpr bool directMinimum31Satisfied(uint32_t prePidSeed) noexcept {
    // HG/SS Bug Contest and Safari encounters reroll the entire candidate up to
    // four times when none of the six IVs is 31. A current candidate with any
    // 31 IV is therefore directly acceptable. A no-31 candidate can still be
    // legal only as the exhausted fourth attempt; that historical chain remains
    // unresolved rather than being treated as invalid.
    uint32_t seed = Gen3PidIv::Detail::next(
        Gen3PidIv::Detail::next(prePidSeed));
    seed = Gen3PidIv::Detail::next(seed);
    const uint16_t iv1 = static_cast<uint16_t>((seed >> 16) & 0x7FFFu);
    seed = Gen3PidIv::Detail::next(seed);
    const uint16_t iv2 = static_cast<uint16_t>((seed >> 16) & 0x7FFFu);
    return hasAny31IvWord(iv1) || hasAny31IvWord(iv2);
}

constexpr uint8_t fishingSlot(bool hgss, uint8_t type, uint16_t rand16) noexcept {
    if (!isFishing(type)) return 0xFF;
    if (isSafari(type))
        return hgss ? safariSlot(rand16) : 0xFF;
    if (hgss)
        return superRodSlotK(rand16 % 100u);

    const uint32_t roll = rand16 / 656u;
    return type == 2 ? surfSlot(roll) : superRodSlotJ(roll);
}

constexpr Activation fishingActivationKind(bool hgss, uint8_t type,
                                           uint16_t rand16) noexcept {
    if (!isFishing(type)) return Activation::None;
    const bool oldRod = type == 2 || type == 12;
    const bool goodRod = type == 3 || type == 13;
    uint32_t rate = oldRod ? 25u : goodRod ? 50u : 75u;
    if (hgss)
        rate += 50u; // Following Pokemon friendship bonus; PKHeX assumes best case.
    const uint32_t roll = hgss ? (rand16 % 100u) : (rand16 / 656u);
    if (roll < rate)
        return Activation::Normal;

    // HG/SS can compound Suction Cups / Sticky Hold after the following-Pokemon
    // bonus. In practice only Old Rod can reach this branch because Good/Super
    // Rod are already >=100 after the +50 bonus.
    if (hgss && roll < rate * 2u)
        return Activation::SuctionCups;
    return Activation::None;
}

constexpr bool fishingActivation(bool hgss, uint8_t type,
                                 uint16_t rand16) noexcept {
    return fishingActivationKind(hgss, type, rand16) != Activation::None;
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

// Conservative positive matcher for audited no-lead / Sweet-Scent-compatible paths.
// Supported now:
//   D/P/Pt Method J: Grass, Surf, fishing, Honey Tree
//   HG/SS  Method K: Grass, Surf, fishing, Headbutt, Rock Smash,
//                     Bug Contest, Safari Grass/Surf/fishing
// Bug Contest/Safari direct proof requires the current Method-1 candidate to have
// at least one 31 IV. Exhausted fourth-attempt no-31 reroll chains and special-lead
// branches remain unresolved rather than pretending to be invalid.
constexpr Result matchNoLeadRow(bool hgss, uint64_t row,
                                uint32_t prePidSeed, uint32_t pid,
                                uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (type > 14)
        return {};
    if ((isHeadbutt(type) || type == 8 || isSafari(type)) && !hgss)
        return {};
    if (isHoneyTree(type) && hgss)
        return {};

    const bool requiresMinimum31 = type == 8 || isSafari(type);
    const bool directMinimum31 =
        !requiresMinimum31 || directMinimum31Satisfied(prePidSeed);

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

            uint8_t rolledSlot = 0xFF;
            if (type == 0) {
                rolledSlot = hgss ? methodKSlot(type, prev1) : methodJSlot(type, prev1);
            } else if (type == 1) {
                rolledSlot = hgss ? methodKSlot(type, prev2) : methodJSlot(type, prev2);
            } else if (isSafari(type)) {
                // HG/SS Safari uses rand % 10 and has no random level frame.
                rolledSlot = safariSlot(prev1);
            } else if (isFishing(type)) {
                rolledSlot = fishingSlot(hgss, type, prev2);
            } else if (type == 5) {
                rolledSlot = rockSmashSlot(prev2);
            } else if (isHeadbutt(type)) {
                rolledSlot = headbuttSlot(prev2);
            } else if (type == 8) {
                rolledSlot = bugContestSlot(prev2);
            } else if (isHoneyTree(type)) {
                // Honey Tree species/slot is chosen before the normal Method J slot routine.
                // PKHeX treats the ESV check as pre-determined.
                rolledSlot = Gen4Wild::slot(row);
            }

            if (rolledSlot == Gen4Wild::slot(row)) {
                if (type == 0) {
                    if (Gen4Wild::levelMatches(row, metLevel))
                        return {hgss ? Method::MethodKNoLead : Method::MethodJNoLead,
                                candidate, rolledSlot};
                } else if (isSafari(type)) {
                    if (!Gen4Wild::levelMatches(row, metLevel) || !directMinimum31) {
                        candidate = Gen3PidIv::Detail::prev(
                            Gen3PidIv::Detail::prev(candidate));
                        continue;
                    }

                    if (isSafariFishing(type)) {
                        // No random level call: activation is immediately before ESV.
                        const auto activation =
                            fishingActivationKind(hgss, type, prev2);
                        if (activation == Activation::None) {
                            candidate = Gen3PidIv::Detail::prev(
                                Gen3PidIv::Detail::prev(candidate));
                            continue;
                        }
                        return {
                            activation == Activation::SuctionCups
                                ? Method::MethodKSafariFishingSuctionCups
                                : Method::MethodKSafariFishingNoLead,
                            candidate, rolledSlot
                        };
                    }
                    return {Method::MethodKSafariNoLead, candidate, rolledSlot};
                } else {
                    const uint8_t level = isHoneyTree(type)
                        ? honeyTreeLevel(prev1)
                        : randomLevel(Gen4Wild::minLevel(row), Gen4Wild::maxLevel(row), prev1);
                    if (level != metLevel) {
                        candidate = Gen3PidIv::Detail::prev(
                            Gen3PidIv::Detail::prev(candidate));
                        continue;
                    }

                    if (type == 1)
                        return {hgss ? Method::MethodKNoLead : Method::MethodJNoLead,
                                candidate, rolledSlot};

                    if (type == 8) {
                        // LeadRequired.None can enter Bug Contest encounters via
                        // Sweet Scent. Non-Sweet-Scent deadlock histories remain
                        // a separate special-lead branch.
                        if (directMinimum31)
                            return {Method::MethodKBugContestNoLead,
                                    candidate, rolledSlot};
                        continue;
                    }

                    if (type == 5) {
                        const uint32_t prev3Seed =
                            Gen3PidIv::Detail::prev(Gen3PidIv::Detail::prev(
                                Gen3PidIv::Detail::prev(candidate)));
                        const auto activation =
                            rockSmashActivationKind(
                                Gen4Wild::rate(row),
                                static_cast<uint16_t>(prev3Seed >> 16));
                        if (activation != Activation::None) {
                            return {
                                activation == Activation::Illuminate
                                    ? Method::MethodKRockSmashIlluminate
                                    : Method::MethodKRockSmashNoLead,
                                candidate, rolledSlot
                            };
                        }
                        continue;
                    }

                    if (isHeadbutt(type))
                        return {Method::MethodKHeadbuttNoLead, candidate, rolledSlot};

                    if (isHoneyTree(type))
                        return {Method::MethodJHoneyTreeNoLead, candidate, rolledSlot};

                    uint32_t activationSeed =
                        Gen3PidIv::Detail::prev(Gen3PidIv::Detail::prev(
                            Gen3PidIv::Detail::prev(candidate)));

                    if (!hgss && Gen4Wild::rate(row) == 0xFFu) {
                        // Mt. Coronet B1F always consumes a tile-replacement RNG call
                        // between the rod activation and encounter-slot rolls. Regular
                        // species can be obtained from a non-Feebas tile; Feebas itself
                        // additionally requires the 50% replacement roll to pass.
                        const uint16_t tileRand =
                            static_cast<uint16_t>(activationSeed >> 16);
                        if (Gen4Wild::species(row) == 349 &&
                            !feebasTileReplacement(tileRand)) {
                            continue;
                        }
                        activationSeed =
                            Gen3PidIv::Detail::prev(activationSeed);
                    }

                    const auto activation =
                        fishingActivationKind(
                            hgss, type,
                            static_cast<uint16_t>(activationSeed >> 16));
                    if (activation != Activation::None) {
                        const Method method = !hgss
                            ? Method::MethodJFishingNoLead
                            : activation == Activation::SuctionCups
                                ? Method::MethodKFishingSuctionCups
                                : Method::MethodKFishingNoLead;
                        return {method, candidate, rolledSlot};
                    }
                }
            }
        }

        candidate = Gen3PidIv::Detail::prev(
            Gen3PidIv::Detail::prev(candidate));
    }
    return {};
}

constexpr bool synchronizePass(bool hgss, uint16_t rand16) noexcept {
    return hgss ? ((rand16 & 1u) == 0u)
                : ((rand16 >> 15) == 0u);
}

constexpr Result matchSynchronizeRow(bool hgss, uint64_t row,
                                     uint32_t prePidSeed, uint32_t pid,
                                     uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (type > 9 || type == 8 || isSafari(type))
        return {};
    if ((type == 5 || isHeadbutt(type)) && !hgss)
        return {};
    if (isHoneyTree(type) && hgss)
        return {};

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    const int frames = reversalWindow(prePidSeed, nature);
    if (frames < 0)
        return {};

    const Method syncMethod =
        hgss ? Method::MethodKSynchronize : Method::MethodJSynchronize;

    uint32_t candidate = prePidSeed;
    for (int i = 0; i <= frames; ++i) {
        const uint16_t natureRand = static_cast<uint16_t>(candidate >> 16);
        const uint32_t rolledNature = hgss
            ? (natureRand % 25u)
            : (natureRand / 0x0A3Eu);

        // PKHeX tries the regular/no-lead path first when the nature roll already
        // equals the PID nature. Successful Synchronize evidence is the distinct
        // branch where that regular nature roll fails but the 50% sync check passes.
        if (rolledNature != nature && synchronizePass(hgss, natureRand)) {
            const uint32_t seed1 = Gen3PidIv::Detail::prev(candidate);
            const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
            const uint16_t prev1 = static_cast<uint16_t>(seed1 >> 16);
            const uint16_t prev2 = static_cast<uint16_t>(seed2 >> 16);

            uint8_t rolledSlot = 0xFF;
            if (type == 0) {
                rolledSlot = hgss ? methodKSlot(type, prev1)
                                  : methodJSlot(type, prev1);
            } else if (type == 1) {
                rolledSlot = hgss ? methodKSlot(type, prev2)
                                  : methodJSlot(type, prev2);
            } else if (isFishing(type)) {
                rolledSlot = fishingSlot(hgss, type, prev2);
            } else if (type == 5) {
                rolledSlot = rockSmashSlot(prev2);
            } else if (isHeadbutt(type)) {
                rolledSlot = headbuttSlot(prev2);
            } else if (isHoneyTree(type)) {
                // Honey Tree species/slot is selected before Method J's normal
                // slot routine, so ESV is already determined by encounter data.
                rolledSlot = Gen4Wild::slot(row);
            }

            if (rolledSlot == Gen4Wild::slot(row)) {
                if (type == 0) {
                    if (Gen4Wild::levelMatches(row, metLevel))
                        return {syncMethod, candidate, rolledSlot};
                } else {
                    const uint8_t level = isHoneyTree(type)
                        ? honeyTreeLevel(prev1)
                        : randomLevel(
                            Gen4Wild::minLevel(row), Gen4Wild::maxLevel(row), prev1);
                    if (level != metLevel) {
                        candidate = Gen3PidIv::Detail::prev(
                            Gen3PidIv::Detail::prev(candidate));
                        continue;
                    }

                    if (type == 1 || isHeadbutt(type) || isHoneyTree(type))
                        return {syncMethod, candidate, rolledSlot};

                    uint32_t activationSeed =
                        Gen3PidIv::Detail::prev(seed2);

                    if (type == 5) {
                        // Synchronize and Illuminate are mutually exclusive leads.
                        // Only the normal Rock Smash activation path can prove sync.
                        if (rockSmashActivationKind(
                                Gen4Wild::rate(row),
                                static_cast<uint16_t>(activationSeed >> 16)) ==
                            Activation::Normal) {
                            return {syncMethod, candidate, rolledSlot};
                        }
                        continue;
                    }

                    if (isFishing(type)) {
                        if (!hgss && Gen4Wild::rate(row) == 0xFFu) {
                            const uint16_t tileRand =
                                static_cast<uint16_t>(activationSeed >> 16);
                            if (Gen4Wild::species(row) == 349 &&
                                !feebasTileReplacement(tileRand)) {
                                continue;
                            }
                            activationSeed =
                                Gen3PidIv::Detail::prev(activationSeed);
                        }

                        // Synchronize cannot simultaneously be the Suction Cups /
                        // Sticky Hold lead, so only normal fishing activation proves it.
                        if (fishingActivationKind(
                                hgss, type,
                                static_cast<uint16_t>(activationSeed >> 16)) ==
                            Activation::Normal) {
                            return {syncMethod, candidate, rolledSlot};
                        }
                    }
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
                            uint8_t pokemonForm, uint32_t id32,
                            uint32_t prePidSeed, uint32_t pid) noexcept {
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
        if (speciesId == 446 && Gen4Wild::method(row) == 9 &&
            !Gen4Wild::isMunchlaxTreeLocation(id32, metLocation))
            continue;

        const auto result = matchNoLeadRow(
            hgss, row, prePidSeed, pid, metLevel);
        if (result.matched())
            return result;
    }
    return {};
}

inline Result analyzeSupported(std::string_view exactGameId,
                               uint16_t speciesId,
                               uint16_t metLocation,
                               uint8_t metLevel,
                               uint8_t pokemonForm,
                               uint32_t id32,
                               uint32_t prePidSeed,
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
        if (speciesId == 446 && Gen4Wild::method(row) == 9 &&
            !Gen4Wild::isMunchlaxTreeLocation(id32, metLocation))
            continue;

        if (const auto noLead =
                matchNoLeadRow(hgss, row, prePidSeed, pid, metLevel);
            noLead.matched())
            return noLead;

        if (const auto sync =
                matchSynchronizeRow(hgss, row, prePidSeed, pid, metLevel);
            sync.matched())
            return sync;
    }
    return {};
}

constexpr const char* methodName(Method method) noexcept {
    switch (method) {
        case Method::MethodJNoLead: return "Method J (no lead)";
        case Method::MethodKNoLead: return "Method K (no lead)";
        case Method::MethodJFishingNoLead: return "Method J fishing (no lead)";
        case Method::MethodKFishingNoLead: return "Method K fishing (no lead)";
        case Method::MethodKFishingSuctionCups: return "Method K fishing (Suction Cups / Sticky Hold)";
        case Method::MethodKHeadbuttNoLead: return "Method K Headbutt (no lead)";
        case Method::MethodJHoneyTreeNoLead: return "Method J Honey Tree (no lead)";
        case Method::MethodKRockSmashNoLead: return "Method K Rock Smash (no lead)";
        case Method::MethodKRockSmashIlluminate: return "Method K Rock Smash (Illuminate)";
        case Method::MethodKBugContestNoLead: return "Method K Bug Contest (no lead / Sweet Scent)";
        case Method::MethodKSafariNoLead: return "Method K Safari (no lead)";
        case Method::MethodKSafariFishingNoLead: return "Method K Safari fishing (no lead)";
        case Method::MethodKSafariFishingSuctionCups: return "Method K Safari fishing (Suction Cups / Sticky Hold)";
        case Method::MethodJSynchronize: return "Method J (Synchronize)";
        case Method::MethodKSynchronize: return "Method K (Synchronize)";
        case Method::None: break;
    }
    return "No supported Method J/K match";
}

} // namespace Legality::Gen4WildRng
