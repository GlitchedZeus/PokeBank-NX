#pragma once

#include "Legality/Gen3PidIvCorrelation.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4MysteryGiftPid {

namespace Detail {
inline constexpr uint32_t ArngReverseMult = 0x9638806Du;
inline constexpr uint32_t ArngReverseAdd = 0x69C77F93u;

constexpr uint32_t arngPrev(uint32_t pid) noexcept {
    return pid * ArngReverseMult + ArngReverseAdd;
}

constexpr uint32_t psv(uint32_t pid) noexcept {
    return ((pid >> 16) ^ (pid & 0xFFFFu)) >> 3;
}

constexpr uint32_t packedIv1(const std::array<uint8_t, 6>& ivs) noexcept {
    return static_cast<uint32_t>(ivs[0])
         | (static_cast<uint32_t>(ivs[1]) << 5)
         | (static_cast<uint32_t>(ivs[2]) << 10);
}

constexpr uint32_t packedIv2(const std::array<uint8_t, 6>& ivs) noexcept {
    return static_cast<uint32_t>(ivs[3])
         | (static_cast<uint32_t>(ivs[4]) << 5)
         | (static_cast<uint32_t>(ivs[5]) << 10);
}

constexpr bool method1IvsMatch(uint32_t candidatePid,
                               const std::array<uint8_t, 6>& ivs,
                               uint32_t& originSeed) noexcept {
    const auto seeds = Gen3PidIv::Detail::reverseAdjacent(
        candidatePid << 16, candidatePid & 0xFFFF0000u);
    const uint32_t iv1 = packedIv1(ivs);
    const uint32_t iv2 = packedIv2(ivs);

    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t seed = seeds.values[i];
        uint32_t state = Gen3PidIv::Detail::next2(seed);
        if (iv1 != Gen3PidIv::Detail::next15(state))
            continue;
        if (iv2 != Gen3PidIv::Detail::next15(state))
            continue;
        originSeed = seed;
        return true;
    }
    return false;
}
} // namespace Detail

struct Result {
    bool matched = false;
    uint8_t rerolls = 0;
    uint32_t originalPid = 0;
    uint32_t originSeed = 0;
};

// Gen IV Mystery Gift anti-shiny PID rerolls are performed with ARNG.
// PKHeX checks at most three previous ARNG states. This proves the RNG class
// only; a matching distribution/event template is a separate legality layer.
constexpr Result analyze(uint32_t finalPid,
                         const std::array<uint8_t, 6>& ivs) noexcept {
    const uint32_t finalPsv = Detail::psv(finalPid);
    uint32_t candidate = Detail::arngPrev(finalPid);
    const uint32_t originalPsv = Detail::psv(candidate);

    if (originalPsv == finalPsv)
        return {};

    for (uint8_t rerolls = 1; rerolls <= 3; ++rerolls) {
        uint32_t originSeed = 0;
        if (Detail::method1IvsMatch(candidate, ivs, originSeed))
            return {true, rerolls, candidate, originSeed};

        candidate = Detail::arngPrev(candidate);
        if (Detail::psv(candidate) != originalPsv)
            break;
    }
    return {};
}

} // namespace Legality::Gen4MysteryGiftPid
