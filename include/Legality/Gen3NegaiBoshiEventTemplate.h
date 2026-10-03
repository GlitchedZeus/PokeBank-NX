#pragma once

#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen3NegaiBoshiEvent {

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

constexpr bool commonFields(const Candidate& c) noexcept {
    return c.species == 385 &&
           c.tid == 30719 && c.sid == 0 &&
           c.language == 1 &&
           c.metLevel == 5 &&
           c.metLocation == 255 &&
           c.ball == 4 &&
           !c.isEgg &&
           !c.fateful &&
           !c.shiny &&
           c.otName == u"ネガイボシ";
}

constexpr bool matchesForceAntiShiny(
    const Candidate& c, const Gen3BacdPidIv::Result& rng) noexcept {
    // Pinned PKHeX EncountersWC3 Japanese Negai Boshi Jirachi
    // BACD_U_AX row:
    // species 385, level 5, Ruby/Sapphire origin, Japanese language,
    // TID 30719 / SID 0, OT "ネガイボシ", recipient OT gender,
    // Poke Ball, event location 255, non-egg, non-fateful, non-shiny.
    // The recipient OT-gender rule permits either stored OT gender; the
    // event's RNG method is the unrestricted forced anti-shiny BA-CD class.
    if (rng.variant != Gen3BacdPidIv::Variant::ForceAntiShiny)
        return false;

    return commonFields(c) &&
           (c.originGame == 1 || c.originGame == 2) &&
           c.otGender <= 1;
}

constexpr uint32_t table2OriginSeed(
    const Gen3BacdPidIv::Result& rng) noexcept {
    if (rng.variant != Gen3BacdPidIv::Variant::Regular &&
        rng.variant != Gen3BacdPidIv::Variant::RegularAntiShiny)
        return 0x10000u;

    return Gen3BacdPidIv::Detail::prev(
        Gen3BacdPidIv::Detail::prev(rng.originSeed));
}

constexpr bool matchesRestrictedTable2(
    const Candidate& c, const Gen3BacdPidIv::Result& rng) noexcept {
    // Pinned PKHeX BACD_TA Negai Boshi Jirachi row:
    // Ruby-only, Japanese, male OT, same fixed trainer/event fields.
    // For Jirachi the table-selection result is identical, so the only RNG
    // restriction is that the BA-CD origin is two calls after a 16-bit seed.
    return commonFields(c) &&
           c.originGame == 2 &&
           c.otGender == 0 &&
           table2OriginSeed(rng) <= 0xFFFFu;
}

constexpr bool matches(
    const Candidate& c, const Gen3BacdPidIv::Result& rng) noexcept {
    return matchesForceAntiShiny(c, rng) ||
           matchesRestrictedTable2(c, rng);
}

} // namespace Legality::Gen3NegaiBoshiEvent
