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

constexpr bool sourceSpeciesCompatible(uint16_t sourceSpecies,
                                       uint16_t currentSpecies) noexcept {
    if (sourceSpecies == currentSpecies)
        return true;

    // Pinned PKHeX EncounterGift3JPN matching receives EvoCriteria rather than
    // requiring the surviving PK3 to remain the distributed species. Keep the
    // reconstruction deliberately limited to descendants reachable in Gen III
    // from species actually present in the six PCJP machine-gift tables.
    // Direction is one-way: evolution after receipt is possible; devolution is not.
    switch (sourceSpecies) {
        // First: Hoenn starters.
        case 252: return currentSpecies == 253 || currentSpecies == 254;
        case 255: return currentSpecies == 256 || currentSpecies == 257;
        case 258: return currentSpecies == 259 || currentSpecies == 260;

        // Second: Johto starters.
        case 152: return currentSpecies == 153 || currentSpecies == 154;
        case 155: return currentSpecies == 156 || currentSpecies == 157;
        case 158: return currentSpecies == 159 || currentSpecies == 160;

        // Third distribution species with Gen III descendants.
        case 23:  return currentSpecies == 24;
        case 25:  return currentSpecies == 26;
        case 52:  return currentSpecies == 53;
        case 58:  return currentSpecies == 59;
        case 69:  return currentSpecies == 70 || currentSpecies == 71;
        case 79:  return currentSpecies == 80 || currentSpecies == 199;
        case 90:  return currentSpecies == 91;
        case 113: return currentSpecies == 242;
        case 123: return currentSpecies == 212;

        // Fourth: Kanto starters.
        case 1: return currentSpecies == 2 || currentSpecies == 3;
        case 4: return currentSpecies == 5 || currentSpecies == 6;
        case 7: return currentSpecies == 8 || currentSpecies == 9;

        // Fifth distribution species with Gen III descendants.
        case 270: return currentSpecies == 271 || currentSpecies == 272;
        case 273: return currentSpecies == 274 || currentSpecies == 275;
        case 283: return currentSpecies == 284;
        case 300: return currentSpecies == 301;
        case 307: return currentSpecies == 308;

        // Sixth distribution species with Gen III descendants.
        case 163: return currentSpecies == 164;
        case 179: return currentSpecies == 180 || currentSpecies == 181;
        case 191: return currentSpecies == 192;
        case 204: return currentSpecies == 205;
        case 209: return currentSpecies == 210;
        case 216: return currentSpecies == 217;
        case 228: return currentSpecies == 229;
        default: return false;
    }
}

template <std::size_t N>
constexpr bool containsSpeciesHistory(const std::array<uint16_t, N>& values,
                                      uint16_t currentSpecies) noexcept {
    for (const uint16_t sourceSpecies : values)
        if (sourceSpeciesCompatible(sourceSpecies, currentSpecies))
            return true;
    return false;
}

constexpr bool speciesAllowed(Distribution dist, uint16_t species) noexcept {
    switch (dist) {
        case Distribution::First:  return containsSpeciesHistory(kFirst, species);
        case Distribution::Second: return containsSpeciesHistory(kSecond, species);
        case Distribution::Third:  return containsSpeciesHistory(kThird, species);
        case Distribution::Fourth: return containsSpeciesHistory(kFourth, species);
        case Distribution::Fifth:  return containsSpeciesHistory(kFifth, species);
        case Distribution::Sixth:  return containsSpeciesHistory(kSixth, species);
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
