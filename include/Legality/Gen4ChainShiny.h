#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4ChainShiny {

namespace Detail {
inline constexpr uint32_t Mult = 0x41C64E6Du;
inline constexpr uint32_t Add = 0x00006073u;
inline constexpr uint32_t ReverseMult = 0xEEB9EB65u;
inline constexpr uint32_t ReverseAdd = 0x0A3561A1u;
inline constexpr uint32_t Mod = 0x67D3u;
inline constexpr uint32_t Pattern = 0x0D3Eu;
inline constexpr uint32_t Increment = 0x4034u;

constexpr uint32_t next(uint32_t seed) noexcept {
    return seed * Mult + Add;
}
constexpr uint32_t prev(uint32_t seed) noexcept {
    return seed * ReverseMult + ReverseAdd;
}
constexpr uint32_t prev2(uint32_t seed) noexcept {
    return prev(prev(seed));
}

struct Seeds {
    std::array<uint32_t, 6> values{};
    std::size_t count = 0;
};

constexpr void addIvSeeds(Seeds& out, uint32_t low,
                          uint32_t first, uint32_t second) noexcept {
    do {
        const uint32_t test = first | low;
        if ((next(test) & 0x7FFF0000u) == second) {
            const uint32_t seed = prev(test);
            if (out.count + 1 < out.values.size()) {
                out.values[out.count++] = seed;
                out.values[out.count++] = seed ^ 0x80000000u;
            }
        }
        low += Mod;
    } while (low < 0x10000u);
}

constexpr Seeds reverseIvSeeds(const std::array<uint8_t, 6>& ivs) noexcept {
    const uint32_t first =
        (static_cast<uint32_t>(ivs[0]) |
         (static_cast<uint32_t>(ivs[1]) << 5) |
         (static_cast<uint32_t>(ivs[2]) << 10)) << 16;
    const uint32_t second =
        (static_cast<uint32_t>(ivs[3]) |
         (static_cast<uint32_t>(ivs[4]) << 5) |
         (static_cast<uint32_t>(ivs[5]) << 10)) << 16;

    const uint32_t diff = (second - (first * Mult)) >> 16;
    const uint32_t start1 =
        (((((diff * Mod) + Increment) >> 16) * Pattern) % Mod);
    const uint32_t start2 =
        ((((((diff ^ 0x8000u) * Mod) + Increment) >> 16) * Pattern) % Mod);

    Seeds out{};
    addIvSeeds(out, start1, first, second);
    addIvSeeds(out, start2, first, second);
    return out;
}

constexpr bool isShiny(uint32_t pid, uint32_t id32) noexcept {
    const uint16_t tid = static_cast<uint16_t>(id32);
    const uint16_t sid = static_cast<uint16_t>(id32 >> 16);
    const uint16_t psv = static_cast<uint16_t>((pid >> 16) ^ (pid & 0xFFFFu));
    return static_cast<uint16_t>(psv ^ tid ^ sid) < 8;
}

constexpr bool validateSeed(uint32_t pid, uint32_t id32,
                            uint32_t seed, uint32_t& originSeed) noexcept {
    const uint16_t tid = static_cast<uint16_t>(id32);
    const uint16_t sid = static_cast<uint16_t>(id32 >> 16);

    uint32_t s = seed;
    int bitIndex = 15;
    do {
        const uint32_t bit = (s >> 16) & 1u;
        if (bit != ((pid >> bitIndex) & 1u))
            break;
        s = prev(s);
    } while (--bitIndex != 2);

    if (bitIndex != 2)
        return false;

    const uint32_t upper = s;
    if (((upper >> 16) & 7u) != ((pid >> 16) & 7u))
        return false;

    const uint32_t lower = prev(upper);
    if (((lower >> 16) & 7u) != (pid & 7u))
        return false;

    const uint32_t upperPid =
        (((pid & 0xFFFFu) ^ tid ^ sid) & 0xFFF8u) |
        ((upper >> 16) & 7u);
    if (upperPid != (pid >> 16))
        return false;

    originSeed = prev2(lower);
    return true;
}
} // namespace Detail

struct Result {
    bool matched = false;
    uint32_t originSeed = 0;
    uint32_t ivSeed = 0;
};

// Proves the Gen IV Poké Radar Chain Shiny PID/IV/trainer-ID RNG relationship.
// It does not prove that the encounter slot itself was Poké Radar capable; that
// remains a separate encounter-provenance layer until the generated wild table
// preserves the relevant slot/radar metadata.
constexpr Result analyze(uint32_t pid, uint32_t id32,
                         const std::array<uint8_t, 6>& ivs) noexcept {
    if (!Detail::isShiny(pid, id32))
        return {};

    const Detail::Seeds seeds = Detail::reverseIvSeeds(ivs);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        uint32_t origin = 0;
        if (Detail::validateSeed(pid, id32, seeds.values[i], origin))
            return {true, origin, seeds.values[i]};
    }
    return {};
}

} // namespace Legality::Gen4ChainShiny
