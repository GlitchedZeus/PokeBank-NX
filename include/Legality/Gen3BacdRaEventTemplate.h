#pragma once

#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3BacdRaEvent {

enum class OtGenderRule : uint8_t {
    Only0 = 0,
    Only1 = 1,
    RandD3 = 2,
    RandS3 = 3,
    RandS7 = 4,
    RandSG15 = 5,
};

struct Entry {
    uint16_t species;
    uint16_t tid;
    uint16_t sid;
    uint8_t level;
    uint8_t language; // 0 = unrestricted recipient language
    uint8_t otGenderRule;
    bool fateful;
    std::u16string_view otName;
    // Distribution-time moves are source metadata. Non-egg event gifts may
    // legally replace current moves after receipt, matching PKHeX IsMatchExact.
    std::array<uint16_t, 4> moves{};
};

#include "Legality/Gen3BacdRaEventTemplateData.inc"

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

struct MatchResult {
    bool matched = false;
    uint16_t species = 0;
    uint16_t tid = 0;
    std::array<uint16_t, 4> initialMoves{};
};

constexpr uint32_t postIvSeed(uint32_t originSeed) noexcept {
    uint32_t seed = originSeed;
    for (int i = 0; i < 4; ++i)
        seed = Gen3BacdPidIv::Detail::next(seed); // A, B, C, D
    return seed;
}

constexpr uint8_t expectedOtGender(uint8_t ruleValue,
                                   uint32_t originSeed) noexcept {
    const auto rule = static_cast<OtGenderRule>(ruleValue);
    if (rule == OtGenderRule::Only0)
        return 0;
    if (rule == OtGenderRule::Only1)
        return 1;

    uint32_t seed = postIvSeed(originSeed);
    if (rule == OtGenderRule::RandSG15)
        seed = Gen3BacdPidIv::Detail::next(
            Gen3BacdPidIv::Detail::next(seed));
    else
        seed = Gen3BacdPidIv::Detail::next(seed);

    const uint16_t rand16 = static_cast<uint16_t>(seed >> 16);
    switch (rule) {
        case OtGenderRule::RandD3:
            return static_cast<uint8_t>((rand16 / 3u) & 1u);
        case OtGenderRule::RandS3:
            return static_cast<uint8_t>((rand16 >> 3) & 1u);
        case OtGenderRule::RandS7:
            return static_cast<uint8_t>(((rand16 >> 7) & 1u) ^ 1u);
        case OtGenderRule::RandSG15:
            return static_cast<uint8_t>((rand16 >> 15) & 1u);
        case OtGenderRule::Only0:
        case OtGenderRule::Only1:
            break;
    }
    return 0xFF;
}

constexpr bool rngCompatible(const Gen3BacdPidIv::Result& rng) noexcept {
    return rng.restrictedSeed &&
           (rng.variant == Gen3BacdPidIv::Variant::Regular ||
            rng.variant == Gen3BacdPidIv::Variant::RegularAntiShiny);
}

constexpr bool persistentFieldsMatch(const Entry& row, const Candidate& c,
                                     uint32_t originSeed) noexcept {
    // Every pinned BACD_R_A row is a Ruby-origin, non-egg event gift.
    if (c.originGame != 2 || c.isEgg || c.shiny || c.metLocation != 255 ||
        c.ball != 4)
        return false;
    if (row.species != c.species || row.tid != c.tid || row.sid != c.sid ||
        row.level != c.metLevel || row.fateful != c.fateful ||
        row.otName != c.otName)
        return false;
    if (row.language != 0 && row.language != c.language)
        return false;
    return expectedOtGender(row.otGenderRule, originSeed) == c.otGender;
}

constexpr MatchResult match(const Candidate& c,
                            const Gen3BacdPidIv::Result& rng) noexcept {
    if (!rngCompatible(rng))
        return {};
    for (const auto& row : kEntries) {
        if (persistentFieldsMatch(row, c, rng.originSeed))
            return {true, row.species, row.tid, row.moves};
    }
    return {};
}

inline constexpr std::size_t kEventCount =
    sizeof(kEntries) / sizeof(kEntries[0]);

} // namespace Legality::Gen3BacdRaEvent
