#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3CxdPidIv {

enum class Variant : uint8_t {
    None,
    Standard,
    AntiShiny,
};

struct Result {
    bool matched = false;
    Variant variant = Variant::None;
    bool searchLimited = false;
    uint32_t originSeed = 0;

    constexpr bool antiShiny() const noexcept {
        return variant == Variant::AntiShiny;
    }
};

namespace Detail {
inline constexpr uint32_t Mult = 0x000343FDu;
inline constexpr uint32_t Add = 0x00269EC3u;
inline constexpr uint32_t ReverseMult = 0xB9B33155u;
inline constexpr uint32_t ReverseAdd = 0xA170F641u;
inline constexpr uint32_t Sub = Add - 0xFFFFu;
inline constexpr uint64_t Base = (static_cast<uint64_t>(Mult) + 1u) * 0xFFFFu;
inline constexpr uint32_t MaxAntiShinyRerolls = 64;

constexpr uint32_t next(uint32_t seed) noexcept { return seed * Mult + Add; }
constexpr uint32_t prev(uint32_t seed) noexcept { return seed * ReverseMult + ReverseAdd; }

struct Seeds {
    std::array<uint32_t, 4> values{};
    std::size_t count = 0;
};

constexpr Seeds reversePid(uint32_t first, uint32_t second) noexcept {
    uint32_t wrapped = second - (first * Mult) - Sub;
    uint64_t t = wrapped;
    const uint64_t kmax = (Base - t) >> 32;
    Seeds out{};
    for (uint64_t k = 0; k <= kmax; ++k, t += 0x1'0000'0000ULL) {
        if ((t % Mult) >= 0x1'0000ULL)
            continue;
        const uint32_t seed = first | static_cast<uint16_t>(t / Mult);
        if (out.count < out.values.size())
            out.values[out.count++] = prev(seed);
    }
    return out;
}

constexpr uint32_t packIv1(const std::array<uint8_t, 6>& ivs) noexcept {
    return static_cast<uint32_t>(ivs[0])
         | (static_cast<uint32_t>(ivs[1]) << 5)
         | (static_cast<uint32_t>(ivs[2]) << 10);
}

constexpr uint32_t packIv2(const std::array<uint8_t, 6>& ivs) noexcept {
    return static_cast<uint32_t>(ivs[3])
         | (static_cast<uint32_t>(ivs[4]) << 5)
         | (static_cast<uint32_t>(ivs[5]) << 10);
}

constexpr bool ivsMatch(uint32_t a, uint32_t b,
                        uint32_t iv1, uint32_t iv2) noexcept {
    return ((a >> 16) & 0x7FFFu) == iv1 &&
           ((b >> 16) & 0x7FFFu) == iv2;
}
} // namespace Detail

constexpr Result analyze(uint32_t pid, const std::array<uint8_t, 6>& ivs) noexcept {
    const uint32_t top = pid & 0xFFFF0000u;
    const uint32_t bottom = pid << 16;
    const uint32_t iv1 = Detail::packIv1(ivs);
    const uint32_t iv2 = Detail::packIv2(ivs);

    const auto seeds = Detail::reversePid(top, bottom);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t seed = seeds.values[i];
        const uint32_t b = Detail::prev(seed);
        const uint32_t a = Detail::prev(b);
        if (Detail::ivsMatch(a, b, iv1, iv2))
            return {true, Variant::Standard, false, Detail::prev(a)};
    }
    return {};
}

// Mirrors PKHeX MethodFinder.GetXDRNGMatch's CXDAnti branch. XD can reroll
// the target PID when it would be shiny to the player while retaining the
// earlier IV-generation origin. The current recorded PID must itself be
// non-shiny, and every skipped target PID must match the player's TSV.
constexpr Result analyzeWithTrainer(uint32_t pid,
                                    const std::array<uint8_t, 6>& ivs,
                                    uint16_t tid, uint16_t sid) noexcept {
    const auto regular = analyze(pid, ivs);
    if (regular.matched)
        return regular;

    const uint32_t top = pid & 0xFFFF0000u;
    const uint32_t bottom = pid << 16;
    const uint32_t iv1 = Detail::packIv1(ivs);
    const uint32_t iv2 = Detail::packIv2(ivs);
    const uint32_t trainerShinyValue =
        static_cast<uint32_t>(tid ^ sid) >> 3;
    const uint32_t recordedShinyValue = (top ^ bottom) >> 19;

    // A shiny recorded PID would have been rerolled again, so it cannot be
    // the terminal result of the XD anti-shiny path.
    if (recordedShinyValue == trainerShinyValue)
        return {};

    bool limited = false;
    const auto seeds = Detail::reversePid(top, bottom);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t seed = seeds.values[i];
        uint32_t b = Detail::prev(seed);
        uint32_t a = Detail::prev(b);

        // The immediately preceding PID must have been shiny to the player.
        uint32_t p2 = seed;
        uint32_t p1 = b;
        if (((p2 ^ p1) >> 19) != trainerShinyValue)
            continue;

        for (uint32_t reroll = 0; reroll < Detail::MaxAntiShinyRerolls; ++reroll) {
            b = Detail::prev(a);
            a = Detail::prev(b);
            if (Detail::ivsMatch(a, b, iv1, iv2))
                return {true, Variant::AntiShiny, false, Detail::prev(a)};

            p2 = Detail::prev(p1);
            p1 = Detail::prev(p2);
            if (((p2 ^ p1) >> 19) != trainerShinyValue)
                break;
            if (reroll + 1 == Detail::MaxAntiShinyRerolls)
                limited = true;
        }
    }

    return {false, Variant::None, limited, 0};
}

} // namespace Legality::Gen3CxdPidIv
