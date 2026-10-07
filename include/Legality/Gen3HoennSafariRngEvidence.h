#pragma once

#include <cstdint>
#include <string_view>

namespace Legality::Gen3HoennSafariRng {

// Pinned PKHeX MethodH.cs (reference 6501f0ab...):
// Hoenn Safari encounters have an extra nature-preference-table history.
// PKHeX models the possible block path by jumping 300 LCRNG calls backward
// from the nature/PID reversal seed with this precomputed affine transform.
inline constexpr uint32_t kPrev300Mult = 0xC048C851u;
inline constexpr uint32_t kPrev300Add  = 0x196302B4u;

inline constexpr uint32_t kLcrngPrevMult = 0xEEB9EB65u;
inline constexpr uint32_t kLcrngPrevAdd  = 0x0A3561A1u;

constexpr uint32_t prev(uint32_t seed) noexcept {
    return seed * kLcrngPrevMult + kLcrngPrevAdd;
}

constexpr uint32_t rewindNaturePreferenceBlock(uint32_t seed) noexcept {
    return seed * kPrev300Mult + kPrev300Add;
}

// Pinned PKHeX IsSafariBlockProc:
//   (seed >> 16) % 100 < 80
constexpr bool safariBlockProc(uint32_t seed) noexcept {
    return ((seed >> 16) % 100u) < 80u;
}

// PKHeX captures the original upper-half nature roll before applying the
// no-block adjustment. If a Hoenn Safari block was not used, MethodH consumes
// one additional dummy call before evaluating the ordinary slot/level frames.
constexpr uint16_t natureRoll(uint32_t seed) noexcept {
    return static_cast<uint16_t>(seed >> 16);
}

constexpr uint8_t nature(uint32_t seed) noexcept {
    return static_cast<uint8_t>(natureRoll(seed) % 25u);
}

constexpr uint32_t noBlockFrameSeed(uint32_t seed) noexcept {
    return prev(seed);
}

constexpr bool isHoennSafariGame(std::string_view exactGameId) noexcept {
    return exactGameId == "ruby_gba" ||
           exactGameId == "sapphire_gba" ||
           exactGameId == "emerald_gba";
}

constexpr bool isHoennSafariLocation(std::string_view exactGameId,
                                     uint16_t metLocation) noexcept {
    return isHoennSafariGame(exactGameId) && metLocation == 57;
}

struct FrameEvidence {
    bool applicable = false;

    // Seed entering pinned PKHeX's Hoenn-Safari branch.
    uint32_t natureSeed = 0;

    // Candidate history where the Safari nature-preference block proc succeeds.
    uint32_t blockSeed = 0;
    bool blockProc = false;

    // Candidate history where the block is not used and the dummy call is
    // rewound before normal Synchronize/nature/slot evaluation.
    uint32_t noBlockSeed = 0;

    uint16_t originalNatureRoll = 0;
    uint8_t originalNature = 0;
};

// This is frame-transformation evidence only. It does not decide a Method-H
// encounter match and does not evaluate slot, level, lead ability, activation,
// PID nature-rejection windows, or Emerald Cute Charm history.
constexpr FrameEvidence evidence(std::string_view exactGameId,
                                 uint16_t metLocation,
                                 uint32_t natureSeed) noexcept {
    if (!isHoennSafariLocation(exactGameId, metLocation))
        return {};

    const uint32_t blockSeed = rewindNaturePreferenceBlock(natureSeed);
    return {
        true,
        natureSeed,
        blockSeed,
        safariBlockProc(blockSeed),
        noBlockFrameSeed(natureSeed),
        natureRoll(natureSeed),
        nature(natureSeed),
    };
}

} // namespace Legality::Gen3HoennSafariRng
