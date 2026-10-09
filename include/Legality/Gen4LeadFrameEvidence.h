#pragma once

#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen4WildEncounter.h"

#include <cstdint>

namespace Legality::Gen4LeadFrame {

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
constexpr bool isBugContest(uint8_t type) noexcept {
    return type == 8;
}
constexpr bool isHoneyTree(uint8_t type) noexcept {
    return type == 9;
}

constexpr uint8_t honeyTreeLevel(uint16_t rand16) noexcept {
    return static_cast<uint8_t>(5u + (rand16 / 0x1745u));
}
constexpr uint8_t rockSmashSlot(uint16_t rand16) noexcept {
    return (rand16 % 100u) < 80u ? 0 : 1;
}
constexpr bool feebasTileReplacement(uint16_t rand16) noexcept {
    return (rand16 >> 15) == 1u;
}

constexpr Activation rockSmashActivationKind(uint8_t areaRate,
                                              uint16_t rand16) noexcept {
    if (areaRate == 0)
        return Activation::None;
    const uint32_t roll = rand16 % 100u;
    if (roll < areaRate)
        return Activation::Normal;
    if (roll < static_cast<uint32_t>(areaRate) * 2u)
        return Activation::Illuminate;
    return Activation::None;
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

constexpr bool bugContestActivationAllows(uint8_t areaRate,
                                           uint32_t activationSeed,
                                           bool canSweetScent) noexcept {
    if (canSweetScent)
        return true;
    if (areaRate == 0)
        return false;
    return ((activationSeed >> 16) % 100u) < areaRate;
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
    uint32_t seed = Gen3PidIv::Detail::next(
        Gen3PidIv::Detail::next(prePidSeed));
    seed = Gen3PidIv::Detail::next(seed);
    const uint16_t iv1 = static_cast<uint16_t>((seed >> 16) & 0x7FFFu);
    seed = Gen3PidIv::Detail::next(seed);
    const uint16_t iv2 = static_cast<uint16_t>((seed >> 16) & 0x7FFFu);
    return hasAny31IvWord(iv1) || hasAny31IvWord(iv2);
}

// HG/SS Bug Contest and Safari generate PID+IV attempts back-to-back when the
// minimum-31 rule rejects an attempt. Entering the next attempt's nature/sync
// frame, the immediately preceding RNG calls are IV2, IV1, PID-high, PID-low,
// then the previous attempt's nature/sync frame. This mirrors pinned PKHeX
// MethodK::RecurseReject without claiming that the earlier lead path is proven.
constexpr uint16_t previousRerollIv2Word(uint32_t nextNatureSeed) noexcept {
    const uint32_t seed = Gen3PidIv::Detail::prev(nextNatureSeed);
    return static_cast<uint16_t>((seed >> 16) & 0x7FFFu);
}

constexpr uint16_t previousRerollIv1Word(uint32_t nextNatureSeed) noexcept {
    uint32_t seed = Gen3PidIv::Detail::prev(nextNatureSeed);
    seed = Gen3PidIv::Detail::prev(seed);
    return static_cast<uint16_t>((seed >> 16) & 0x7FFFu);
}

constexpr bool previousRerollAttemptRejected(
        uint32_t nextNatureSeed) noexcept {
    return !hasAny31IvWord(previousRerollIv1Word(nextNatureSeed)) &&
           !hasAny31IvWord(previousRerollIv2Word(nextNatureSeed));
}

constexpr uint32_t previousRerollNatureSeed(
        uint32_t nextNatureSeed) noexcept {
    uint32_t seed = nextNatureSeed;
    for (int i = 0; i < 5; ++i)
        seed = Gen3PidIv::Detail::prev(seed);
    return seed;
}

// IV-gate-only reconstruction for one of the four HG/SS minimum-31 attempts.
// depth=0 means the first attempt, depth=3 the fourth/final attempt. Earlier
// attempts must all have lacked a 31 IV. A final attempt that also lacks a 31
// is legal only at depth 3 after all three prior attempts were rejected.
// This helper deliberately does not prove nature/lead/slot activation history;
// callers must reconstruct those separately before returning positive evidence.
constexpr bool minimum31IvChainAllows(uint32_t finalNatureSeed,
                                      uint8_t depth) noexcept {
    if (depth > 3)
        return false;

    uint32_t seed = finalNatureSeed;
    for (uint8_t i = 0; i < depth; ++i) {
        if (!previousRerollAttemptRejected(seed))
            return false;
        seed = previousRerollNatureSeed(seed);
    }

    return directMinimum31Satisfied(finalNatureSeed) || depth == 3;
}

constexpr uint8_t fishingSlot(bool hgss, uint8_t type,
                              uint16_t rand16) noexcept {
    if (!isFishing(type))
        return 0xFF;
    if (isSafari(type))
        return hgss ? safariSlot(rand16) : 0xFF;
    if (hgss)
        return superRodSlotK(rand16 % 100u);
    const uint32_t roll = rand16 / 656u;
    return type == 2 ? surfSlot(roll) : superRodSlotJ(roll);
}

constexpr Activation fishingActivationKind(bool hgss, uint8_t type,
                                            uint16_t rand16) noexcept {
    if (!isFishing(type))
        return Activation::None;
    const bool oldRod = type == 2 || type == 12;
    const bool goodRod = type == 3 || type == 13;
    uint32_t rate = oldRod ? 25u : goodRod ? 50u : 75u;
    if (hgss)
        rate += 50u;
    const uint32_t roll = hgss ? (rand16 % 100u) : (rand16 / 656u);
    if (roll < rate)
        return Activation::Normal;
    if (hgss && roll < rate * 2u)
        return Activation::SuctionCups;
    return Activation::None;
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

constexpr uint8_t methodJSlot(uint8_t encounterType,
                              uint16_t rand16) noexcept {
    const uint32_t roll = rand16 / 656u;
    if (encounterType == 0)
        return regularSlot(roll);
    if (encounterType == 1)
        return surfSlot(roll);
    return 0xFF;
}

constexpr uint8_t methodKSlot(uint8_t encounterType,
                              uint16_t rand16) noexcept {
    const uint32_t roll = rand16 % 100u;
    if (encounterType == 0)
        return regularSlot(roll);
    if (encounterType == 1)
        return surfSlot(roll);
    return 0xFF;
}

constexpr uint8_t randomLevel(uint8_t minimum, uint8_t maximum,
                              uint16_t rand16) noexcept {
    const uint32_t width = 1u + static_cast<uint32_t>(maximum) - minimum;
    return static_cast<uint8_t>((rand16 % width) + minimum);
}

} // namespace Legality::Gen4LeadFrame
