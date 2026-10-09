#pragma once

#include "Legality/Gen4FixedBallEvidence.h"
#include "Legality/Gen34EggMoveEvidence.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen4FixedBallOrigin {

enum class Resolution : uint8_t {
    Unresolved,
    RequiredBallKnown,
    Conflicting,
};

struct RequirementAccumulator {
    bool sawAny = false;
    bool sawOrdinary = false;
    bool conflicting = false;
    uint8_t requiredBall = 0;

    constexpr void observeMethod(uint8_t method) noexcept {
        sawAny = true;
        const uint8_t required =
            Gen4FixedBall::requiredBallForMethod(method);
        if (required == 0) {
            sawOrdinary = true;
            return;
        }
        if (requiredBall == 0)
            requiredBall = required;
        else if (requiredBall != required)
            conflicting = true;
    }

    constexpr Resolution resolution() const noexcept {
        if (!sawAny || sawOrdinary || requiredBall == 0)
            return Resolution::Unresolved;
        if (conflicting)
            return Resolution::Conflicting;
        return Resolution::RequiredBallKnown;
    }
};

struct WildEvidence {
    Resolution resolution = Resolution::Unresolved;
    uint8_t requiredBall = 0;
    uint16_t sourceSpecies = 0;
    bool evolved = false;
    bool sawOrdinaryAlternative = false;
};

struct CompetingHistory {
    // RequiredBallKnown is only globally meaningful after every encounter family
    // supported by the caller has been checked. Until then, stay unresolved.
    bool supportComplete = false;
    bool staticOrGift = false;
    bool event = false;
    bool trade = false;
    bool pokewalker = false;
    bool other = false;

    constexpr bool anyCompatible() const noexcept {
        return staticOrGift || event || trade || pokewalker || other;
    }
};

struct Evidence {
    Resolution resolution = Resolution::Unresolved;
    uint8_t requiredBall = 0;
    WildEvidence wild{};
};

inline WildEvidence wildHistoryEvidence(std::string_view exactGameId,
                                        uint16_t currentSpecies,
                                        uint16_t metLocation,
                                        uint8_t metLevel,
                                        uint8_t pokemonForm,
                                        uint32_t id32) noexcept {
    const Gen4Wild::Game wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || currentSpecies == 0 ||
        metLocation > 0xFF || metLevel == 0)
        return {};

    // PKHeX has generation-specific Shedinja ball handling. The accepted
    // pre-evolution table cannot reconstruct the Nincada -> Shedinja creation
    // history by itself, so never resolve Shedinja here.
    if (currentSpecies == 292)
        return {};

    RequirementAccumulator accumulator{};
    uint16_t onlySourceSpecies = 0;
    bool multipleSourceSpecies = false;
    bool sawAncestor = false;

    const auto observeSpecies =
        [&](uint16_t sourceSpecies, bool ancestor) noexcept {
            for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
                if (Gen4Wild::game(row) != wanted ||
                    Gen4Wild::species(row) != sourceSpecies ||
                    Gen4Wild::location(row) != metLocation ||
                    !Gen4Wild::levelMatches(row, metLevel))
                    continue;

                if (ancestor) {
                    // Form-specific ancestry is not reconstructed yet. Mirror the
                    // existing conservative evolution helpers and only promote an
                    // ordinary base-form source through evolution.
                    if (Gen4Wild::form(row) != 0)
                        continue;
                } else if (!Gen4Wild::formMatches(
                               Gen4Wild::form(row), pokemonForm)) {
                    continue;
                }

                if (sourceSpecies == 446 && Gen4Wild::method(row) == 9 &&
                    !Gen4Wild::isMunchlaxTreeLocation(id32, metLocation))
                    continue;

                accumulator.observeMethod(Gen4Wild::method(row));
                if (ancestor)
                    sawAncestor = true;

                if (onlySourceSpecies == 0)
                    onlySourceSpecies = sourceSpecies;
                else if (onlySourceSpecies != sourceSpecies)
                    multipleSourceSpecies = true;
            }
        };

    observeSpecies(currentSpecies, false);

    uint16_t ancestor =
        Gen34EggMove::preEvolution(exactGameId, currentSpecies);
    for (int depth = 0; ancestor != 0 && depth < 8; ++depth) {
        observeSpecies(ancestor, true);
        const uint16_t next =
            Gen34EggMove::preEvolution(exactGameId, ancestor);
        if (next == ancestor)
            break;
        ancestor = next;
    }

    WildEvidence out{};
    out.resolution = accumulator.resolution();
    out.requiredBall =
        out.resolution == Resolution::RequiredBallKnown
            ? accumulator.requiredBall
            : 0;
    out.sourceSpecies = multipleSourceSpecies ? 0 : onlySourceSpecies;
    out.evolved = sawAncestor;
    out.sawOrdinaryAlternative = accumulator.sawOrdinary;
    return out;
}

constexpr Evidence applyCompetingHistory(
        const WildEvidence& wild,
        const CompetingHistory& competing) noexcept {
    if (wild.resolution == Resolution::Conflicting)
        return {Resolution::Conflicting, 0, wild};

    if (wild.resolution != Resolution::RequiredBallKnown)
        return {Resolution::Unresolved, 0, wild};

    if (!competing.supportComplete || competing.anyCompatible())
        return {Resolution::Unresolved, 0, wild};

    return {Resolution::RequiredBallKnown, wild.requiredBall, wild};
}

inline Evidence resolve(std::string_view exactGameId,
                        uint16_t currentSpecies,
                        uint16_t metLocation,
                        uint8_t metLevel,
                        uint8_t pokemonForm,
                        uint32_t id32,
                        const CompetingHistory& competing) noexcept {
    return applyCompetingHistory(
        wildHistoryEvidence(
            exactGameId, currentSpecies, metLocation, metLevel,
            pokemonForm, id32),
        competing);
}

} // namespace Legality::Gen4FixedBallOrigin
