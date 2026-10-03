#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3BacdPidIv {

enum class Variant : uint8_t {
    None,
    Regular,
    RegularAntiShiny,
    ForceAntiShiny,
    ForceShiny,
};

struct Result {
    Variant variant = Variant::None;
    uint32_t originSeed = 0;
    bool restrictedSeed = false;

    constexpr bool matched() const noexcept { return variant != Variant::None; }
};

constexpr const char* variantName(Variant variant) noexcept {
    switch (variant) {
        case Variant::Regular:          return "regular BA-CD";
        case Variant::RegularAntiShiny: return "regular anti-shiny BA-CD_A";
        case Variant::ForceAntiShiny:   return "forced anti-shiny BA-CD_AX";
        case Variant::ForceShiny:       return "forced-shiny BA-CD_S";
        case Variant::None:             break;
    }
    return "unresolved BA-CD";
}

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

constexpr bool isShiny(uint32_t pid, uint32_t idXor) noexcept {
    return ((pid >> 16) ^ (pid & 0xFFFFu) ^ idXor) < 8u;
}

constexpr uint32_t regularAntiShiny(uint32_t expectedPid, uint32_t idXor) noexcept {
    if (!isShiny(expectedPid, idXor))
        return expectedPid;
    return (expectedPid + 8u) & 0xFFFFFFF8u;
}

constexpr uint32_t forceAntiShiny(uint32_t a16, uint32_t b16, uint32_t idXor) noexcept {
    if ((a16 & ~0x7u) == 0)
        return 0xFFFFFFFFu;
    return ((a16 ^ (idXor ^ b16)) << 16) | b16;
}

constexpr uint32_t forceShiny(uint32_t x16, uint32_t b16, uint32_t idXor) noexcept {
    return (x16 << 16) | (((idXor ^ x16) & 0xFFF8u) | (b16 & 0x7u));
}

} // namespace Detail

// Positive recognition for regular and anti-shiny BA-CD Gen III event RNG classes.
// Forced-shiny, restricted-seed and exact event-template checks remain separate.
constexpr Result analyzeWithTrainer(uint32_t pid, const std::array<uint8_t, 6>& ivs,
                                    uint16_t tid16, uint16_t sid16) noexcept {
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
        const uint32_t origin = Detail::prev(seed);
        const bool restricted = origin <= 0xFFFFu;
        if (expectedPid == pid)
            return {Variant::Regular, origin, restricted};

        const uint32_t idXor = static_cast<uint32_t>(tid16 ^ sid16);
        if (Detail::regularAntiShiny(expectedPid, idXor) == pid &&
            expectedPid != pid)
            return {Variant::RegularAntiShiny, origin, restricted};

        if (Detail::forceAntiShiny(a16, b16, idXor) == pid)
            return {Variant::ForceAntiShiny, origin, restricted};

        const uint32_t xState = Detail::prev(seed);
        const uint32_t x16 = xState >> 16;
        if (Detail::forceShiny(x16, b16, idXor) == pid)
            return {Variant::ForceShiny, Detail::prev(xState),
                    Detail::prev(xState) <= 0xFFFFu};
    }
    return {};
}

constexpr Result analyze(uint32_t pid, const std::array<uint8_t, 6>& ivs) noexcept {
    // Backward-compatible base matcher: impossible IDs disable anti-shiny variants.
    const auto result = analyzeWithTrainer(pid, ivs, 0xFFFFu, 0xFFFFu);
    return result.variant == Variant::Regular ? result : Result{};
}

} // namespace Legality::Gen3BacdPidIv
