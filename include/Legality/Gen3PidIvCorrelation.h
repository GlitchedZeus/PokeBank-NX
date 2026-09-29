#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3PidIv {

enum class Method : uint8_t {
    None,
    Method1,
    Method2,
    Method3,
    Method4,
    Method1Unown,
    Method2Unown,
    Method3Unown,
    Method4Unown,
    Method1Roamer,
};

struct Result {
    Method method = Method::None;
    uint32_t originSeed = 0;

    constexpr bool matched() const noexcept { return method != Method::None; }
};

constexpr const char* methodName(Method method) noexcept {
    switch (method) {
        case Method::Method1: return "Method 1";
        case Method::Method2: return "Method 2";
        case Method::Method3: return "Method 3";
        case Method::Method4: return "Method 4";
        case Method::Method1Unown: return "Method 1 (Unown)";
        case Method::Method2Unown: return "Method 2 (Unown)";
        case Method::Method3Unown: return "Method 3 (Unown)";
        case Method::Method4Unown: return "Method 4 (Unown)";
        case Method::Method1Roamer: return "Method 1 (Roamer truncated IVs)";
        case Method::None: break;
    }
    return "No handheld LCRNG match";
}

namespace Detail {

inline constexpr uint32_t Mult = 0x41C64E6Du;
inline constexpr uint32_t Add = 0x00006073u;
inline constexpr uint32_t ReverseMult = 0xEEB9EB65u;
inline constexpr uint32_t ReverseAdd = 0x0A3561A1u;

constexpr uint32_t next(uint32_t seed) noexcept {
    return seed * Mult + Add;
}
constexpr uint32_t prev(uint32_t seed) noexcept {
    return seed * ReverseMult + ReverseAdd;
}
constexpr uint32_t next2(uint32_t seed) noexcept {
    return next(next(seed));
}
constexpr uint32_t next3(uint32_t seed) noexcept {
    return next(next2(seed));
}
constexpr uint32_t next15(uint32_t& seed) noexcept {
    seed = next(seed);
    return (seed >> 16) & 0x7FFFu;
}

struct Seeds {
    std::array<uint32_t, 6> values{};
    std::size_t count = 0;
};

constexpr Seeds reverseAdjacent(uint32_t first, uint32_t second) noexcept {
    constexpr uint32_t Mod = 0x67D3u;
    constexpr uint32_t Pattern = 0x0D3Eu;
    constexpr uint32_t Increment = 0x4034u;

    const uint32_t diff = (second - (first * Mult)) >> 16;
    uint32_t low = (((((diff * Mod) + Increment) >> 16) * Pattern) % Mod);

    Seeds out{};
    do {
        const uint32_t seed = first | low;
        if ((next(seed) & 0xFFFF0000u) == second)
            out.values[out.count++] = prev(seed);
        low += Mod;
    } while (low < 0x10000u);
    return out;
}

constexpr Seeds reverseSkipOne(uint32_t first, uint32_t third) noexcept {
    constexpr uint32_t Mod = 0x3A89u;
    constexpr uint32_t Pattern = 0x2E4Cu;
    constexpr uint32_t Increment = 0x5831u;

    const uint32_t diff = (third - next2(first)) >> 16;
    uint32_t low = (((((diff * Mod) + Increment) >> 16) * Pattern) % Mod);

    Seeds out{};
    do {
        const uint32_t seed = first | low;
        if ((next2(seed) & 0xFFFF0000u) == third)
            out.values[out.count++] = prev(seed);
        low += Mod;
    } while (low < 0x10000u);
    return out;
}

constexpr Result scanMethods124(uint32_t first, uint32_t second,
                                uint32_t iv1, uint32_t iv2,
                                bool unown) noexcept {
    const Seeds seeds = reverseAdjacent(first, second);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t seed = seeds.values[i];
        uint32_t state = next2(seed);

        if (iv1 == next15(state)) {
            if (iv2 == next15(state))
                return {unown ? Method::Method1Unown : Method::Method1, seed};
            if (iv2 == next15(state))
                return {unown ? Method::Method4Unown : Method::Method4, seed};
        } else {
            if (iv1 != next15(state))
                continue;
            if (iv2 == next15(state))
                return {unown ? Method::Method2Unown : Method::Method2, seed};
        }
    }
    return {};
}

constexpr Result scanMethod3(uint32_t first, uint32_t third,
                             uint32_t iv1, uint32_t iv2,
                             bool unown) noexcept {
    const Seeds seeds = reverseSkipOne(first, third);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t seed = seeds.values[i];
        uint32_t state = next3(seed);
        if (iv1 != next15(state))
            continue;
        if (iv2 != next15(state))
            continue;
        return {unown ? Method::Method3Unown : Method::Method3, seed};
    }
    return {};
}

} // namespace Detail

// IV order is the canonical PK3 order: HP, Attack, Defense, Speed, Sp. Atk, Sp. Def.
//
// This intentionally recognizes only the normal GBA handheld LCRNG Method 1/2/3/4 family.
// Channel, Colosseum/XD, roamers, BACD/event distributions and other special classes are
// separate legality layers; Method::None therefore means "unresolved", not "illegal".
constexpr Result analyzeRoamer(uint32_t pid,
                               const std::array<uint8_t, 6>& ivs) noexcept {
    // Ruby/Sapphire and FR/LG roaming encounters have the classic Gen III save bug:
    // only the low 8 bits of the generated IV32 survive. Emerald roamers use normal
    // Method 1 and should be checked with analyze() instead.
    const uint32_t iv1 = static_cast<uint32_t>(ivs[0])
                       | (static_cast<uint32_t>(ivs[1]) << 5)
                       | (static_cast<uint32_t>(ivs[2]) << 10);
    const uint32_t iv2 = static_cast<uint32_t>(ivs[3])
                       | (static_cast<uint32_t>(ivs[4]) << 5)
                       | (static_cast<uint32_t>(ivs[5]) << 10);
    const uint32_t iv32 = iv1 | (iv2 << 15);
    if (iv32 > 0xFFu)
        return {};

    const uint32_t top = pid & 0xFFFF0000u;
    const uint32_t bottom = pid << 16;
    const Detail::Seeds seeds = Detail::reverseAdjacent(bottom, top);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t seed = seeds.values[i];
        const uint32_t ivByte =
            (Detail::next3(seed) >> 16) & 0xFFu;
        if (ivByte == iv32)
            return {Method::Method1Roamer, seed};
    }
    return {};
}

constexpr bool isRoamerSpecies(uint16_t species) noexcept {
    return species == 243 || species == 244 || species == 245 ||
           species == 380 || species == 381;
}

constexpr Result analyze(uint32_t pid, const std::array<uint8_t, 6>& ivs,
                         bool unown = false) noexcept {
    const uint32_t iv1 = static_cast<uint32_t>(ivs[0])
                       | (static_cast<uint32_t>(ivs[1]) << 5)
                       | (static_cast<uint32_t>(ivs[2]) << 10);
    const uint32_t iv2 = static_cast<uint32_t>(ivs[3])
                       | (static_cast<uint32_t>(ivs[4]) << 5)
                       | (static_cast<uint32_t>(ivs[5]) << 10);

    const uint32_t top = pid & 0xFFFF0000u;
    const uint32_t bottom = pid << 16;

    const uint32_t first = unown ? top : bottom;
    const uint32_t second = unown ? bottom : top;

    if (const Result regular = Detail::scanMethods124(first, second, iv1, iv2, unown);
        regular.matched())
        return regular;
    return Detail::scanMethod3(first, second, iv1, iv2, unown);
}

} // namespace Legality::Gen3PidIv
