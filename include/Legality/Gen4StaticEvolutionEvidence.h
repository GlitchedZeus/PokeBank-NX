#pragma once

#include "Legality/Gen4StaticEncounter.h"
#include "Legality/Gen34EggMoveEvidence.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen4StaticEvolution {

// Positive-only reconstruction for a surviving PK4 that evolved after a native
// Gen IV static/gift encounter. Pinned PKHeX EncounterStatic4 receives an
// EvoCriteria; the surviving species therefore does not have to equal the
// original static source species.
//
// Scope is intentionally conservative:
// - direct static matches retain the existing Gen4Static matcher semantics;
// - evolved-source reconstruction accepts non-egg base-form source rows;
// - hatched static-gift egg descendants are accepted only when the persisted
//   egg-location/met-level/fixed-ball constraints still prove the source row;
// - source-form history remains separate until its evolution semantics are
//   represented explicitly;
// - absence of a match is never an Invalid verdict.
struct MatchResult {
    const uint64_t* row = nullptr;
    uint16_t sourceSpecies = 0;
    bool evolved = false;
    bool hatchedGiftEgg = false;

    constexpr bool matched() const noexcept { return row != nullptr; }
};

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
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || sourceSpecies == 0)
        return nullptr;

    for (const uint64_t& row : Gen4Static::kPackedGen4StaticEncounters) {
        if (Gen4Static::game(row) != wanted ||
            Gen4Static::species(row) != sourceSpecies ||
            Gen4Static::eggLocation(row) != 0 ||
            Gen4Static::form(row) != 0)
            continue;

        const uint8_t requiredBall = Gen4Static::fixedBall(row);
        if (requiredBall != 0 && pokemonBall != 0xFF && pokemonBall != requiredBall)
            continue;
        if (metLevel != Gen4Static::level(row))
            continue;

        if (Gen4Static::roaming(row)) {
            if (!Gen4Static::roamerLocationAllowed(
                    Gen4Static::location(row), metLocation))
                continue;
        } else if (Gen4Static::location(row) != metLocation) {
            continue;
        }

        if (!Gen4Static::constraintsMatch(
                row, pokemonGender, pokemonNature,
                pokemonShiny, pokemonFateful))
            continue;
        return &row;
    }
    return nullptr;
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
    constexpr uint16_t kLinkTrade4 = 2002;

    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || sourceSpecies == 0 || metLevel != 0)
        return nullptr;

    for (const uint64_t& row : Gen4Static::kPackedGen4StaticEncounters) {
        if (Gen4Static::game(row) != wanted ||
            Gen4Static::species(row) != sourceSpecies ||
            Gen4Static::eggLocation(row) == 0 ||
            Gen4Static::form(row) != 0)
            continue;

        const uint16_t expectedEgg = Gen4Static::eggLocation(row);
        if (pokemonEggLocation != expectedEgg && pokemonEggLocation != kLinkTrade4)
            continue;

        const uint8_t requiredBall = Gen4Static::fixedBall(row);
        if (requiredBall != 0 && pokemonBall != 0xFF && pokemonBall != requiredBall)
            continue;

        if (!Gen4Static::constraintsMatch(
                row, pokemonGender, pokemonNature,
                pokemonShiny, pokemonFateful))
            continue;
        return &row;
    }
    return nullptr;
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
    if (const uint64_t* direct = Gen4Static::findMatch(
            exactGameId, currentSpecies, metLocation, metLevel,
            pokemonForm, pokemonEggLocation, pokemonBall,
            pokemonGender, pokemonNature, pokemonShiny, pokemonFateful))
        return {direct, currentSpecies, false, false};

    uint16_t ancestor = Gen34EggMove::preEvolution(exactGameId, currentSpecies);
    for (int depth = 0; ancestor != 0 && depth < 8; ++depth) {
        if (const uint64_t* source = findNonEggBaseFormSource(
                exactGameId, ancestor, metLocation, metLevel, pokemonBall,
                pokemonGender, pokemonNature, pokemonShiny, pokemonFateful))
            return {source, ancestor, true, false};

        // Pinned EncounterStatic4 treats a hatched PK4 gift egg differently from
        // an unhatched egg: met location is the hatch location and therefore is
        // not compared to the gift row, while met level remains 0 and the gift
        // egg location survives (or becomes Link Trade 2002 after an egg trade).
        // An evolved descendant necessarily implies the source egg hatched.
        if (const uint64_t* source = findHatchedGiftEggBaseFormSource(
                exactGameId, ancestor, metLevel, pokemonEggLocation, pokemonBall,
                pokemonGender, pokemonNature, pokemonShiny, pokemonFateful))
            return {source, ancestor, true, true};

        const uint16_t next = Gen34EggMove::preEvolution(exactGameId, ancestor);
        if (next == ancestor)
            break;
        ancestor = next;
    }
    return {};
}

} // namespace Legality::Gen4StaticEvolution
