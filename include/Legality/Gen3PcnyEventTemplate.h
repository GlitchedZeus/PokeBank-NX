#pragma once

#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3PcnyEvent {

enum class Distribution : uint8_t {
    Evolution = 0,
    Dragon = 1,
    Monster = 2,
    Halloween = 3,
    EXDragon = 4,
    UnknownSpring = 5,
    Colosseum = 6,
    Box = 7,
    BabyTrade = 8,
    SlitherSwim = 9,
    AncientAliens = 10,
    Sixth = 11,
};

struct Entry {
    uint16_t species;
    uint8_t level;
    uint8_t distribution;
    std::array<uint16_t, 4> moves;
};

#include "Legality/Gen3PcnyEventTemplateData.inc"

struct Candidate {
    uint16_t species = 0;
    uint16_t tid = 0;
    uint16_t sid = 0;
    uint8_t originGame = 0;
    uint8_t language = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool isEgg = false;
    bool shiny = false;
    std::u16string_view otName{};
};

constexpr bool validTrainerId(uint16_t tid) noexcept {
    return tid != 0 && tid < 3000;
}

constexpr bool validTrainerName(Distribution dist,
                                std::u16string_view name) noexcept {
    switch (dist) {
        case Distribution::Evolution:
        case Distribution::Monster:
        case Distribution::Halloween:
        case Distribution::EXDragon:
            return name == u"PCNYb" || name == u"PCNYc";
        case Distribution::Dragon:
            return name == u"PCNYb" || name == u"PCNYc" || name == u"PCNYd";
        case Distribution::UnknownSpring:
        case Distribution::Colosseum:
        case Distribution::Box:
            return name == u"PCNYc" || name == u"PCNYd";
        case Distribution::BabyTrade:
        case Distribution::SlitherSwim:
        case Distribution::AncientAliens:
        case Distribution::Sixth:
            return name == u"PCNYd";
    }
    return false;
}

constexpr bool speciesHistoryMatches(const Entry& row,
                                     uint16_t currentSpecies) noexcept {
    if (row.species == currentSpecies)
        return true;

    // Pinned PKHeX EncounterGift3NY matching is evaluated against an evolution
    // criterion rather than requiring the current species to remain identical
    // to the distributed species. Start with the three explicitly named
    // "Evolution" distribution rows from encounter_pcny.pkl. Keep the mapping
    // directional: a received Pokemon may evolve, but it cannot become one of
    // its own pre-evolutions after receipt.
    if (static_cast<Distribution>(row.distribution) != Distribution::Evolution)
        return false;

    switch (row.species) {
        case 25:  // Pikachu -> Raichu
            return currentSpecies == 26;
        case 44:  // Gloom -> Vileplume / Bellossom
            return currentSpecies == 45 || currentSpecies == 182;
        case 120: // Staryu -> Starmie
            return currentSpecies == 121;
        default:
            return false;
    }
}

constexpr bool persistentFieldsMatch(const Entry& row,
                                     const Candidate& c) noexcept {
    return speciesHistoryMatches(row, c.species) &&
           row.level == c.metLevel &&
           validTrainerId(c.tid) &&
           c.sid == 0 &&
           (c.originGame == 1 || c.originGame == 2) &&
           c.language != 1 &&
           c.metLocation == 255 &&
           c.ball == 4 &&
           !c.isEgg &&
           !c.shiny &&
           validTrainerName(
               static_cast<Distribution>(row.distribution), c.otName);
}

constexpr bool matches(const Candidate& c,
                       const Gen3BacdPidIv::Result& rng) noexcept {
    // PKHeX EncounterGift3NY uses BACD_U_AX / observed BACD_AX:
    // unrestricted forced anti-shiny BA-CD, with gift-table selection driven
    // by a separate RNG and therefore not recoverable from the Pokemon PID.
    if (rng.variant != Gen3BacdPidIv::Variant::ForceAntiShiny)
        return false;

    for (const auto& row : kEntries)
        if (persistentFieldsMatch(row, c))
            return true;
    return false;
}

inline constexpr std::size_t kEventCount = kEntries.size();

} // namespace Legality::Gen3PcnyEvent
