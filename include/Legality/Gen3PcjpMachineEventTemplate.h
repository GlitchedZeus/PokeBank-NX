#pragma once

#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3PcjpMachineEvent {

enum class Distribution : uint8_t {
    First,
    Second,
    Third,
    Fourth,
    Fifth,
    Sixth,
    None = 0xFF,
};

inline constexpr std::array<uint16_t, 3> kFirst{{252, 255, 258}};
inline constexpr std::array<uint16_t, 3> kSecond{{152, 155, 158}};
inline constexpr std::array<uint16_t, 19> kThird{{
    23, 25, 52, 58, 69, 79, 90, 113, 115, 123,
    125, 126, 128, 198, 200, 211, 215, 225, 226,
}};
inline constexpr std::array<uint16_t, 3> kFourth{{1, 4, 7}};
inline constexpr std::array<uint16_t, 16> kFifth{{
    25, 270, 273, 283, 300, 302, 303, 307,
    311, 312, 315, 335, 336, 337, 338, 358,
}};
inline constexpr std::array<uint16_t, 14> kSixth{{
    25, 163, 179, 190, 191, 202, 204,
    207, 209, 213, 216, 228, 234, 235,
}};

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
    bool shiny = false;
    std::u16string_view otName{};
};

constexpr Distribution distributionForTid(uint16_t tid) noexcept {
    switch (tid) {
        case 51126: return Distribution::First;
        case 51224: return Distribution::Second;
        case 60114: return Distribution::Third;
        case 60227: return Distribution::Fourth;
        case 60321: return Distribution::Fifth;
        case 60505: return Distribution::Sixth;
        default: return Distribution::None;
    }
}

template <std::size_t N>
constexpr bool contains(const std::array<uint16_t, N>& values,
                        uint16_t species) noexcept {
    for (const uint16_t value : values)
        if (value == species)
            return true;
    return false;
}

constexpr bool speciesAllowed(Distribution dist, uint16_t species) noexcept {
    switch (dist) {
        case Distribution::First:  return contains(kFirst, species);
        case Distribution::Second: return contains(kSecond, species);
        case Distribution::Third:  return contains(kThird, species);
        case Distribution::Fourth: return contains(kFourth, species);
        case Distribution::Fifth:  return contains(kFifth, species);
        case Distribution::Sixth:  return contains(kSixth, species);
        case Distribution::None: break;
    }
    return false;
}

constexpr bool validOtName(Distribution dist,
                           std::u16string_view name) noexcept {
    const bool common =
        name == u"トウキョー" ||
        name == u"ヨコハマ" ||
        name == u"ナゴヤ" ||
        name == u"オーサカ" ||
        name == u"フクオカ";
    if (common)
        return true;
    return dist != Distribution::Sixth && name == u"サッポロ";
}

constexpr uint8_t expectedOtGender(uint32_t originSeed) noexcept {
    uint32_t seed = originSeed;
    for (int i = 0; i < 5; ++i)
        seed = Gen3BacdPidIv::Detail::next(seed);
    const uint16_t rand16 = static_cast<uint16_t>(seed >> 16);
    return static_cast<uint8_t>(((rand16 >> 7) & 1u) ^ 1u);
}

constexpr bool matches(const Candidate& c,
                       const Gen3BacdPidIv::Result& rng) noexcept {
    // Pinned PKHeX EncounterGift3JPN machine distributions:
    // Ruby origin, Japanese, level 10, event location 255, Poke Ball,
    // SID 0, one fixed TID per distribution, non-egg/non-shiny, and a
    // distribution-specific city OT. OT gender is bit7-derived five calls
    // after the recovered BA-CD origin. Table selection itself uses a
    // separate RNG and therefore is not inferred from the Pokemon PID.
    if (rng.variant != Gen3BacdPidIv::Variant::Regular ||
        c.sid != 0 ||
        c.originGame != 2 ||
        c.language != 1 ||
        c.metLevel != 10 ||
        c.metLocation != 255 ||
        c.ball != 4 ||
        c.isEgg ||
        c.shiny)
        return false;

    const Distribution dist = distributionForTid(c.tid);
    return dist != Distribution::None &&
           speciesAllowed(dist, c.species) &&
           validOtName(dist, c.otName) &&
           c.otGender == expectedOtGender(rng.originSeed);
}

inline constexpr std::size_t kEventCount =
    kFirst.size() + kSecond.size() + kThird.size() +
    kFourth.size() + kFifth.size() + kSixth.size();

} // namespace Legality::Gen3PcjpMachineEvent
