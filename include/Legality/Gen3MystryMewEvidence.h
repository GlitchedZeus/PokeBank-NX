#pragma once

#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3MystryMew {

inline constexpr std::array<uint16_t, 86> kBaseSeeds{{
    0x0652, 0x0932, 0x0C13, 0x0D43, 0x0EEE,
    0x1263, 0x13C9, 0x1614, 0x1C09, 0x1EA5,
    0x20BF, 0x2389, 0x2939, 0x302D, 0x306E,
    0x34F3, 0x45F3, 0x46CE, 0x4A0D, 0x4B63,
    0x4C79, 0x508E, 0x50AB, 0x5240, 0x5327,
    0x56BA, 0x56CC, 0x5841, 0x5A60, 0x5BC1,
    0x5E2B, 0x5EF3, 0x6065, 0x643F, 0x6457,
    0x67A3, 0x6944, 0x6E06, 0x6E62, 0x7667,
    0x77EF, 0x78D2, 0x8655, 0x8A92, 0x8B48,
    0x93D0, 0x941D, 0x95A0, 0x967D, 0x9690,
    0x9C37, 0x9C40, 0x9D9C, 0x9DE4, 0x9E86,
    0xA153, 0xA443, 0xA8AC, 0xAC08, 0xAFFB,
    0xB1F2, 0xB831, 0xBE96, 0xC2D4, 0xC385,
    0xC6CE, 0xC92C, 0xC953, 0xC962, 0xCC43,
    0xCD47, 0xCD96, 0xD1E4, 0xDFED, 0xE62C,
    0xE6CC, 0xE90A, 0xE95D, 0xE991, 0xEBB2,
    0xEE7F, 0xEE9F, 0xEFC8, 0xF0E4, 0xFE4E,
    0xFE9D,
}};

struct SeedInfo {
    int index = -1;
    uint8_t subIndex = 0;
    uint16_t baseSeed = 0;
};

constexpr int seedIndex(uint16_t seed) noexcept {
    for (std::size_t i = 0; i < kBaseSeeds.size(); ++i)
        if (kBaseSeeds[i] == seed)
            return static_cast<int>(i);
    return -1;
}

constexpr uint32_t prev5(uint32_t seed) noexcept {
    for (int i = 0; i < 5; ++i)
        seed = Gen3BacdPidIv::Detail::prev(seed);
    return seed;
}

constexpr SeedInfo seedInfo(uint32_t originSeed) noexcept {
    if (originSeed <= 0xFFFFu) {
        const auto base = static_cast<uint16_t>(originSeed);
        return {seedIndex(base), 0, base};
    }

    uint32_t seed = originSeed;
    for (uint8_t sub = 1; sub < 5; ++sub) {
        seed = prev5(seed);
        if (seed <= 0xFFFFu) {
            const auto base = static_cast<uint16_t>(seed);
            return {seedIndex(base), sub, base};
        }
    }
    return {};
}

constexpr bool validSeedProvenance(uint32_t originSeed) noexcept {
    const auto info = seedInfo(originSeed);
    if (info.index < 0)
        return false;

    // PKHeX MystryMew: seed 0x6065 had only sibling index 2 released.
    if (info.baseSeed == 0x6065u)
        return info.subIndex == 2;
    return true;
}

constexpr uint8_t expectedOtGender(uint32_t originSeed) noexcept {
    // EncounterGift3 RandD3 uses the RNG call immediately after A/B/C/D.
    uint32_t seed = originSeed;
    for (int i = 0; i < 5; ++i)
        seed = Gen3BacdPidIv::Detail::next(seed);
    const uint16_t rand16 = static_cast<uint16_t>(seed >> 16);
    return static_cast<uint8_t>((rand16 / 3u) & 1u);
}

struct Candidate {
    uint16_t species = 0;
    uint16_t tid = 0;
    uint16_t sid = 0;
    uint8_t originGame = 0;
    uint8_t language = 0;
    uint8_t otGender = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool isEgg = false;
    bool fateful = false;
    bool shiny = false;
    std::u16string_view otName{};
};

constexpr bool matches(const Candidate& c,
                       const Gen3BacdPidIv::Result& rng) noexcept {
    if (rng.variant != Gen3BacdPidIv::Variant::Regular ||
        !validSeedProvenance(rng.originSeed))
        return false;

    return c.species == 151 &&
           c.tid == 6930 && c.sid == 0 &&
           c.originGame == 2 &&
           c.language == 2 &&
           c.otGender == expectedOtGender(rng.originSeed) &&
           c.metLevel == 10 &&
           c.metLocation == 255 &&
           c.ball == 4 &&
           !c.isEgg &&
           c.fateful &&
           !c.shiny &&
           c.otName == u"MYSTRY";
}

} // namespace Legality::Gen3MystryMew
