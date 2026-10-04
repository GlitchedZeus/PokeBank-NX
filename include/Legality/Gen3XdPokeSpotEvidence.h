#pragma once

#include "Legality/Gen3CxdPidIvCorrelation.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3XdPokeSpotEvidence {

enum class Setup : uint8_t {
    Invalid,
    Neither,
    Munchlax,
};

struct Slot {
    uint16_t species;
    uint8_t location;
    uint8_t levelMin;
    uint8_t levelMax;
    uint8_t slot;
};

inline constexpr std::array<Slot, 9> kSlots{{
    {27,  90, 10, 23, 0}, // Rock: Sandshrew
    {207, 90, 10, 20, 1}, // Rock: Gligar
    {328, 90, 10, 20, 2}, // Rock: Trapinch
    {187, 91, 10, 20, 0}, // Oasis: Hoppip
    {231, 91, 10, 20, 1}, // Oasis: Phanpy
    {283, 91, 10, 20, 2}, // Oasis: Surskit
    {41,  92, 10, 21, 0}, // Cave: Zubat
    {304, 92, 10, 21, 1}, // Cave: Aron
    {194, 92, 10, 21, 2}, // Cave: Wooper
}};

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t language = 0;
    uint8_t otGender = 0;
    uint8_t metLevel = 0;
    uint8_t metLocation = 0;
    bool egg = false;
    bool fateful = false;
    bool hasOriginalMetLevel = true;
    uint32_t pid = 0;
    std::array<uint8_t, 6> ivs{};
};

struct Evidence {
    bool identityMatched = false;
    bool pidMatched = false;
    bool ivMatched = false;
    uint8_t slot = 0xFF;
    uint8_t generatedLevel = 0;
    Setup setup = Setup::Invalid;
    uint32_t pidOriginSeed = 0;
    uint32_t ivOriginSeed = 0;

    constexpr bool matched() const noexcept {
        return identityMatched && pidMatched && ivMatched;
    }
};

namespace Detail {

constexpr bool isGen3Language(uint8_t language) noexcept {
    return language >= 1 && language <= 7 && language != 6;
}

constexpr uint32_t prevN(uint32_t seed, unsigned count) noexcept {
    while (count-- != 0)
        seed = Gen3CxdPidIv::Detail::prev(seed);
    return seed;
}

constexpr uint8_t getSlot(uint32_t esv) noexcept {
    return esv < 50 ? 0 : esv < 85 ? 1 : 2;
}

constexpr Setup validActivation(uint8_t slot, uint32_t seed,
                                uint32_t& origin) noexcept {
    origin = 0;
    const uint32_t esv = (seed >> 16) % 100u;
    if (getSlot(esv) != slot)
        return Setup::Invalid;

    seed = Gen3CxdPidIv::Detail::prev(seed);
    uint32_t preSlot = seed >> 16;
    if ((preSlot % 3u) == 0) {
        origin = Gen3CxdPidIv::Detail::prev(seed);
        return Setup::Neither;
    }

    // Source-supported permissive alternate setup: only Munchlax is available
    // to steal Poké Snacks. If its 10% check succeeds, the wild slot cannot spawn.
    if ((preSlot % 100u) < 10u)
        return Setup::Invalid;

    seed = Gen3CxdPidIv::Detail::prev(seed);
    preSlot = seed >> 16;
    if ((preSlot % 3u) == 0) {
        origin = Gen3CxdPidIv::Detail::prev(seed);
        return Setup::Munchlax;
    }

    return Setup::Invalid;
}

struct IvSeeds {
    std::array<uint32_t, 6> values{};
    std::size_t count = 0;
};

// Mirrors XDRNG.GetSeedsIVs: IV calls expose only 15 RNG bits, so each recovered
// lower-half candidate also has a valid top-bit-flipped seed. Pinned XDRNG caps
// this inverse at six candidates.
constexpr IvSeeds reverseIvs(uint32_t first, uint32_t second) noexcept {
    const uint32_t wrapped =
        (second - (first * Gen3CxdPidIv::Detail::Mult) -
         Gen3CxdPidIv::Detail::Sub) & 0x7FFFFFFFu;
    uint64_t t = wrapped;
    const uint64_t kmax = (Gen3CxdPidIv::Detail::Base - t) >> 31;

    IvSeeds out{};
    for (uint64_t k = 0; k <= kmax; ++k, t += 0x80000000ULL) {
        if ((t % Gen3CxdPidIv::Detail::Mult) >= 0x10000ULL)
            continue;
        const uint32_t seed = Gen3CxdPidIv::Detail::prev(
            first | static_cast<uint16_t>(t / Gen3CxdPidIv::Detail::Mult));
        if (out.count + 1 >= out.values.size())
            break;
        out.values[out.count++] = seed;
        out.values[out.count++] = seed ^ 0x80000000u;
    }
    return out;
}

constexpr bool validAnimation(uint32_t seed, uint32_t& origin,
                              uint32_t& animation) noexcept {
    seed = Gen3CxdPidIv::Detail::prev(seed);
    uint32_t prev16 = seed >> 16;
    animation = prev16 % 10u;
    if (animation < 5u && animation != 3u) {
        origin = Gen3CxdPidIv::Detail::prev(seed);
        return true;
    }

    seed = Gen3CxdPidIv::Detail::prev(seed);
    prev16 = seed >> 16;
    animation = prev16 % 10u;
    if (animation >= 5u && animation != 8u) {
        origin = Gen3CxdPidIv::Detail::prev(seed);
        return true;
    }

    seed = prevN(seed, 3);
    animation = (seed >> 16) % 10u;
    if (animation == 8u) {
        origin = Gen3CxdPidIv::Detail::prev(seed);
        return true;
    }

    origin = 0;
    return false;
}

struct PidEvidence {
    bool matched = false;
    Setup setup = Setup::Invalid;
    uint32_t origin = 0;
};

constexpr PidEvidence analyzePid(uint32_t pid, uint8_t slot) noexcept {
    const auto seeds = Gen3CxdPidIv::Detail::reversePid(
        pid & 0xFFFF0000u, pid << 16);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        uint32_t origin = 0;
        const Setup setup = validActivation(slot, seeds.values[i], origin);
        if (setup != Setup::Invalid)
            return {true, setup, origin};
    }
    return {};
}

struct IvEvidence {
    bool matched = false;
    uint8_t level = 0;
    uint32_t origin = 0;
};

constexpr IvEvidence analyzeIvs(const std::array<uint8_t, 6>& ivs,
                                uint8_t levelMin, uint8_t levelMax,
                                uint8_t metLevel,
                                bool hasOriginalMetLevel) noexcept {
    const uint32_t iv1 = Gen3CxdPidIv::Detail::packIv1(ivs) << 16;
    const uint32_t iv2 = Gen3CxdPidIv::Detail::packIv2(ivs) << 16;
    const auto seeds = reverseIvs(iv1, iv2);
    const uint32_t levelDelta = 1u + levelMax - levelMin;

    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t preIv = seeds.values[i];
        uint32_t animationOrigin = 0;
        uint32_t animation = 0;
        const uint32_t animationSeed = prevN(preIv, 6);
        if (!validAnimation(animationSeed, animationOrigin, animation))
            continue;

        const uint32_t level16 = prevN(preIv, 2) >> 16;
        const uint8_t generatedLevel = static_cast<uint8_t>(
            levelMin + (level16 % levelDelta));
        if (hasOriginalMetLevel ? (metLevel != generatedLevel)
                                : (metLevel < generatedLevel))
            continue;

        // The post-IV ability bit is deliberately not constrained. PKHeX also
        // ignores it here because evolution can legitimately reset the ability.
        return {true, generatedLevel, animationOrigin};
    }
    return {};
}

} // namespace Detail

constexpr Evidence analyze(const Candidate& candidate) noexcept {
    if (candidate.originGame != 15 || !Detail::isGen3Language(candidate.language) ||
        candidate.otGender != 0 || candidate.egg || !candidate.fateful)
        return {};

    for (const auto& slot : kSlots) {
        if (candidate.species != slot.species ||
            candidate.metLocation != slot.location)
            continue;

        Evidence evidence{};
        evidence.identityMatched = true;
        evidence.slot = slot.slot;

        const auto pid = Detail::analyzePid(candidate.pid, slot.slot);
        evidence.pidMatched = pid.matched;
        evidence.setup = pid.setup;
        evidence.pidOriginSeed = pid.origin;

        const auto iv = Detail::analyzeIvs(
            candidate.ivs, slot.levelMin, slot.levelMax,
            candidate.metLevel, candidate.hasOriginalMetLevel);
        evidence.ivMatched = iv.matched;
        evidence.generatedLevel = iv.level;
        evidence.ivOriginSeed = iv.origin;
        return evidence;
    }
    return {};
}

inline constexpr std::size_t kSlotCount = kSlots.size();

} // namespace Legality::Gen3XdPokeSpotEvidence
