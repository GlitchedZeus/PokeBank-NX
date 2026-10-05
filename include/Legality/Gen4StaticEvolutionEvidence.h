#pragma once

#include "Legality/Gen4StaticEncounter.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen4StaticEvolution {

// Compatibility surface for the dedicated evolution-provenance evidence API.
// The implementation now lives with Gen4Static so the central legality report's
// existing Gen4Static::findMatch consumer receives the same source-aware result.
using MatchResult = Gen4Static::EvolutionMatchResult;

inline const uint64_t* findNonEggBaseFormSource(
        std::string_view exactGameId,
        uint16_t sourceSpecies,
        uint16_t metLocation,
        uint8_t metLevel,
        uint8_t pokemonBall,
        uint8_t pokemonGender,
        uint8_t pokemonNature,
        bool pokemonShiny,
        bool pokemonFateful) noexcept {
    return Gen4Static::findNonEggBaseFormSource(
        exactGameId, sourceSpecies, metLocation, metLevel, pokemonBall,
        pokemonGender, pokemonNature, pokemonShiny, pokemonFateful);
}

inline const uint64_t* findHatchedGiftEggBaseFormSource(
        std::string_view exactGameId,
        uint16_t sourceSpecies,
        uint8_t metLevel,
        uint16_t pokemonEggLocation,
        uint8_t pokemonBall,
        uint8_t pokemonGender,
        uint8_t pokemonNature,
        bool pokemonShiny,
        bool pokemonFateful) noexcept {
    return Gen4Static::findHatchedGiftEggBaseFormSource(
        exactGameId, sourceSpecies, metLevel, pokemonEggLocation, pokemonBall,
        pokemonGender, pokemonNature, pokemonShiny, pokemonFateful);
}

inline MatchResult matchEvolutionLine(
        std::string_view exactGameId,
        uint16_t currentSpecies,
        uint16_t metLocation,
        uint8_t metLevel,
        uint8_t pokemonForm,
        uint16_t pokemonEggLocation,
        uint8_t pokemonBall,
        uint8_t pokemonGender,
        uint8_t pokemonNature,
        bool pokemonShiny,
        bool pokemonFateful) noexcept {
    return Gen4Static::matchEvolutionLine(
        exactGameId, currentSpecies, metLocation, metLevel, pokemonForm,
        pokemonEggLocation, pokemonBall, pokemonGender, pokemonNature,
        pokemonShiny, pokemonFateful);
}

inline MatchResult matchEvolutionLineWithEggState(
        std::string_view exactGameId,
        uint16_t currentSpecies,
        uint16_t metLocation,
        uint8_t metLevel,
        uint8_t pokemonForm,
        uint16_t pokemonEggLocation,
        uint8_t pokemonBall,
        uint8_t pokemonGender,
        uint8_t pokemonNature,
        bool pokemonShiny,
        bool pokemonFateful,
        bool pokemonIsEgg) noexcept {
    return Gen4Static::matchEvolutionLineWithEggState(
        exactGameId, currentSpecies, metLocation, metLevel, pokemonForm,
        pokemonEggLocation, pokemonBall, pokemonGender, pokemonNature,
        pokemonShiny, pokemonFateful, pokemonIsEgg);
}

} // namespace Legality::Gen4StaticEvolution
