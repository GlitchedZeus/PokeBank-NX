#pragma once

#include "Legality/Gen34EggMoveEvidence.h"
#include "Legality/Gen4SpecialBallEvidence.h"

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace Legality::Gen4EvolvedSpecialBall {

// Positive-only wild capture origin *before evolution*; PK4 retains
// the capture's met level, original retail game and ball. The current
// species does not need to have occupied that exact wild encounter slot.
// This does not prove the evolution itself, a transfer chronology,
// unique ancestry, or exclusion of egg/event/trade histories.
// Pin: PKHeX @ 6501f0ab46e8f8ca048539dbaf8cae8cb104e722.
// Missing provenance stays Incomplete/Unresolved, never hard Invalid.
struct Evidence {
    Gen4SpecialBallEvidence::Affinity affinity =
        Gen4SpecialBallEvidence::Affinity::None;
    uint16_t sourceSpecies = 0; // zero when several ancestor species match
    std::size_t matchingAncestors = 0;
    constexpr bool matched() const noexcept {
        return affinity != Gen4SpecialBallEvidence::Affinity::None;
    }
};

inline Evidence analyzeSupported(std::string_view exactStoredOrigin,
                                 uint16_t currentSpecies,
                                 uint16_t metLocation,
                                 uint8_t metLevel,
                                 uint8_t currentForm,
                                 uint8_t ball) noexcept {
    if (currentSpecies == 0 || currentSpecies > 493 ||
        currentForm != 0 || metLevel == 0 ||
        // Shedinja has special per-origin ball semantics verified elsewhere.
        currentSpecies == 292 ||
        Gen34EggMove::groupForId(exactStoredOrigin) ==
            Gen34EggMove::Group::None)
        return {};

    Evidence out{};
    uint16_t ancestor =
        Gen34EggMove::preEvolution(exactStoredOrigin, currentSpecies);
    for (int depth = 0; ancestor != 0 && depth < 8; ++depth) {
        const auto affinity=Gen4SpecialBallEvidence::analyzeSupported(
            exactStoredOrigin, ancestor, metLocation, metLevel, 0, ball);
        if (affinity != Gen4SpecialBallEvidence::Affinity::None) {
            if (out.matched() && affinity != out.affinity)
                return {}; // contradictory types: no specific claim
            out.affinity = affinity;
            ++out.matchingAncestors;
            out.sourceSpecies = out.matchingAncestors == 1 ?
                ancestor : 0; // do not claim a unique source
        }
        const uint16_t next =
            Gen34EggMove::preEvolution(exactStoredOrigin, ancestor);
        if (next == ancestor)
            break;
        ancestor = next;
    }
    return out;
}
} // namespace Legality::Gen4EvolvedSpecialBall
