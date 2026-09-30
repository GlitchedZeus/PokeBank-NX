#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3BacdPidIv {

struct Result {
    bool matched = false;
    uint32_t originSeed = 0;
};

namespace Detail {
inline constexpr uint32_t Mult = 0x41C64E6Du;
inline constexpr uint32_t Add = 0x00006073u;
inline constexpr uint32_t ReverseMult = 0xEEB9EB65u;
inline constexpr uint32_t ReverseAdd = 0x0A3561A1u;
inline constexpr uint32_t Mod = 0x67D3u;
inline constexpr uint32_t Pattern = 0x0D3Eu;
inline constexpr uint32_t Increment = 0x4034u;

constexpr uint32_t next(uint32_t seed) noexcept { return seed * Mult + Add; }
constexpr uint32_t prev(uint32_t seed) noexcept { return seed * ReverseMult + ReverseAdd; }

struct Seeds {
    std::array<uint32_t, 12> values{};
    std::size_t count = 0;
};

constexpr void addSeeds(Seeds& out, uint32_t low,
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

constexpr Seeds reverseIvs(uint32_t first, uint32_t second) noexcept {
    const uint32_t diff = (second - (first * Mult)) >> 16;
    const uint32_t start1 =
        (((((diff * Mod) + Increment) >> 16) * Pattern) % Mod);
    const uint32_t start2 =
        ((((((diff ^ 0x8000u) * Mod) + Increment) >> 16) * Pattern) % Mod);

    Seeds out{};
    addSeeds(out, start1, first, second);
    addSeeds(out, start2, first, second);
    return out;
}

} // namespace Detail

// Positive recognition for the regular BA-CD Gen III event RNG class.
// Anti-shiny, forced-shiny, restricted-seed and other event variants are separate.
constexpr Result analyze(uint32_t pid, const std::array<uint8_t, 6>& ivs) noexcept {
    const uint32_t iv1 =
        static_cast<uint32_t>(ivs[0]) |
        (static_cast<uint32_t>(ivs[1]) << 5) |
        (static_cast<uint32_t>(ivs[2]) << 10);
    const uint32_t iv2 =
        static_cast<uint32_t>(ivs[3]) |
        (static_cast<uint32_t>(ivs[4]) << 5) |
        (static_cast<uint32_t>(ivs[5]) << 10);

    const auto seeds = Detail::reverseIvs(iv1 << 16, iv2 << 16);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        uint32_t seed = seeds.values[i];
        const uint32_t b16 = seed >> 16;
        seed = Detail::prev(seed);
        const uint32_t a16 = seed >> 16;
        const uint32_t expectedPid = (a16 << 16) | b16;
        if (expectedPid == pid)
            return {true, Detail::prev(seed)};
    }
    return {};
}

} // namespace Legality::Gen3BacdPidIv
