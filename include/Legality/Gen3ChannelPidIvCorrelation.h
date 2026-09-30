#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3ChannelPidIv {

struct Result {
    bool matched = false;
    uint32_t originSeed = 0;
};

namespace Detail {
inline constexpr uint32_t Mult = 0x000343FDu;
inline constexpr uint32_t Add = 0x00269EC3u;
inline constexpr uint32_t ReverseMult = 0xB9B33155u;
inline constexpr uint32_t ReverseAdd = 0xA170F641u;
inline constexpr uint32_t Sub = Add - 0xFFFFu;
inline constexpr uint64_t Base = (static_cast<uint64_t>(Mult) + 1u) * 0xFFFFu;

constexpr uint32_t next(uint32_t seed) noexcept { return seed * Mult + Add; }
constexpr uint32_t next3(uint32_t seed) noexcept {
    return next(next(next(seed)));
}
constexpr uint32_t prev(uint32_t seed) noexcept {
    return seed * ReverseMult + ReverseAdd;
}

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

constexpr uint32_t sequentialIv32(uint32_t seed) noexcept {
    uint32_t out = 0;
    for (int i = 0; i < 6; ++i) {
        seed = next(seed);
        out |= ((seed >> 27) & 31u) << (i * 5);
    }
    return out;
}
} // namespace Detail

// Positive correlation for Pokémon Channel Jirachi's XDRNG pattern.
// originGame uses the PK3 stored values: 1=Sapphire, 2=Ruby.
constexpr Result analyze(uint32_t pid, const std::array<uint8_t, 6>& ivs,
                         uint16_t sid16, uint8_t originGame,
                         uint8_t otGender) noexcept {
    if (originGame != 1 && originGame != 2)
        return {};

    uint32_t top = pid & 0xFFFF0000u;
    const uint32_t bottom = pid << 16;
    const uint32_t undo = (top >> 16) ^ 0x8000u;
    const uint32_t expected = (bottom >> 16) ^ sid16 ^ 40122u;
    if ((undo > 7u ? 0u : 1u) != expected)
        top = undo << 16;

    const uint32_t packedIvs =
        static_cast<uint32_t>(ivs[0]) |
        (static_cast<uint32_t>(ivs[1]) << 5) |
        (static_cast<uint32_t>(ivs[2]) << 10) |
        (static_cast<uint32_t>(ivs[3]) << 15) |
        (static_cast<uint32_t>(ivs[4]) << 20) |
        (static_cast<uint32_t>(ivs[5]) << 25);

    const auto seeds = Detail::reversePid(top, bottom);
    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t seed = seeds.values[i];
        const uint32_t c = Detail::next3(seed); // held item
        const uint32_t d = Detail::next(c);     // Ruby/Sapphire bit
        if (((d >> 31) + 1u) != originGame)
            continue;
        const uint32_t e = Detail::next(d);     // OT gender bit
        if ((e >> 31) != otGender)
            continue;
        if (Detail::sequentialIv32(e) != packedIvs)
            continue;
        if ((seed >> 16) != sid16)
            continue;
        return {true, Detail::prev(seed)};
    }
    return {};
}

} // namespace Legality::Gen3ChannelPidIv
