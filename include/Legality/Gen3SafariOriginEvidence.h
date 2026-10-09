#pragma once

#include "Legality/Gen3SafariEncounter.h"
#include "Legality/Gen34EggMoveEvidence.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3SafariOrigin {

enum class Resolution : uint8_t {
    Unresolved,
    RequiredBallKnown,
};

struct WildEvidence {
    Resolution resolution = Resolution::Unresolved;
    uint8_t requiredBall = 0;
    uint16_t sourceSpecies = 0;
    bool evolved = false;
    bool multipleSourceSpecies = false;
    std::size_t candidateRows = 0;
};

// Resolve only the wild Safari history represented by the pinned Gen III
// encounter table. PK3 retains its original met location/level after evolution,
// so a current species can inherit Safari provenance from any accepted
// pre-evolution source species.
//
// This does not claim complete encounter-family provenance. It is deliberately
// separate from the global competing-history gate below.
inline WildEvidence wildHistoryEvidence(std::string_view exactGameId,
                                        uint16_t currentSpecies,
                                        uint16_t metLocation,
                                        uint8_t metLevel,
                                        uint8_t pokemonForm) noexcept {
    if (Gen3Safari::gameForId(exactGameId) == Gen3Safari::Game::Invalid ||
        currentSpecies == 0 || currentSpecies > 386 ||
        metLocation > 0xFF || metLevel == 0)
        return {};

    std::size_t candidates = 0;
    uint16_t onlySourceSpecies = 0;
    bool multipleSourceSpecies = false;
    bool sawAncestor = false;

    const auto observeSpecies =
        [&](uint16_t sourceSpecies, uint8_t form, bool ancestor) noexcept {
            const auto evidence = Gen3Safari::directWildEvidence(
                exactGameId, sourceSpecies, metLocation, metLevel, form);
            if (!evidence.matched)
                return;

            candidates += evidence.candidateRows;
            if (ancestor)
                sawAncestor = true;

            if (onlySourceSpecies == 0)
                onlySourceSpecies = sourceSpecies;
            else if (onlySourceSpecies != sourceSpecies)
                multipleSourceSpecies = true;
        };

    observeSpecies(currentSpecies, pokemonForm, false);

    uint16_t ancestor =
        Gen34EggMove::preEvolution(exactGameId, currentSpecies);
    for (int depth = 0; ancestor != 0 && depth < 8; ++depth) {
        // Gen III Safari rows in the pinned table are ordinary base-form source
        // rows. Do not project a current evolved form onto its ancestor.
        observeSpecies(ancestor, 0, true);

        const uint16_t next =
            Gen34EggMove::preEvolution(exactGameId, ancestor);
        if (next == ancestor)
            break;
        ancestor = next;
    }

    if (candidates == 0)
        return {};

    WildEvidence out{};
    out.resolution = Resolution::RequiredBallKnown;
    out.requiredBall = Gen3Safari::kSafariBall;
    out.sourceSpecies = multipleSourceSpecies ? 0 : onlySourceSpecies;
    out.evolved = sawAncestor;
    out.multipleSourceSpecies = multipleSourceSpecies;
    out.candidateRows = candidates;
    return out;
}

struct CompetingHistory {
    // A fixed-ball conclusion is globally meaningful only if the caller has
    // source-backed coverage for every encounter family it intends to support.
    // Until then, preserve Unknown/unsupported as Unresolved.
    bool supportComplete = false;
    bool staticOrGift = false;
    bool event = false;
    bool trade = false;
    bool egg = false;
    bool gameCube = false;
    bool other = false;

    constexpr bool anyCompatible() const noexcept {
        return staticOrGift || event || trade || egg || gameCube || other;
    }
};

struct Evidence {
    Resolution resolution = Resolution::Unresolved;
    uint8_t requiredBall = 0;
    WildEvidence wild{};
};

constexpr Evidence applyCompetingHistory(
        const WildEvidence& wild,
        const CompetingHistory& competing) noexcept {
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
                        const CompetingHistory& competing) noexcept {
    return applyCompetingHistory(
        wildHistoryEvidence(
            exactGameId, currentSpecies, metLocation, metLevel, pokemonForm),
        competing);
}

} // namespace Legality::Gen3SafariOrigin
