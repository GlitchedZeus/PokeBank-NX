#pragma once

#include "Legality/Gen4WildEncounter.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen4FixedBall {

inline constexpr uint8_t kPokeBall = 4;
inline constexpr uint8_t kSafariBall = 5;
inline constexpr uint8_t kSportBall = 24;

struct Evidence {
    bool matched = false;
    uint8_t requiredBall = 0;
};

// PKHeX SlotType4 at pinned reference
// 6501f0ab46e8f8ca048539dbaf8cae8cb104e722:
//   BugContest = 8
//   Safari_Grass..Safari_Super_Rod = 10..14
// BallVerifier requires Sport Ball for a direct Bug Contest encounter and Safari
// Ball for direct Safari encounters. This helper deliberately proves only direct
// wild-slot provenance. Evolved/reconstructed histories (notably Shedinja) are
// separate and must remain unresolved until their history is reconstructed.
constexpr uint8_t requiredBallForMethod(uint8_t method) noexcept {
    if (method == 8)
        return kSportBall;
    if (method >= 10 && method <= 14)
        return kSafariBall;
    return 0;
}

inline Evidence directWildEvidence(std::string_view exactGameId,
                                   uint16_t speciesId,
                                   uint16_t metLocation,
                                   uint8_t metLevel,
                                   uint8_t pokemonForm,
                                   uint32_t id32) noexcept {
    const Gen4Wild::Game wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || speciesId == 0 ||
        metLocation > 0xFF || metLevel == 0)
        return {};

    uint8_t required = 0;
    bool matchedAny = false;
    for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
        if (Gen4Wild::game(row) != wanted || Gen4Wild::species(row) != speciesId ||
            Gen4Wild::location(row) != metLocation ||
            !Gen4Wild::levelMatches(row, metLevel) ||
            !Gen4Wild::formMatches(Gen4Wild::form(row), pokemonForm))
            continue;

        // Keep exact parity with the accepted trainer-aware wild matcher.
        if (speciesId == 446 && Gen4Wild::method(row) == 9 &&
            !Gen4Wild::isMunchlaxTreeLocation(id32, metLocation))
            continue;

        matchedAny = true;
        const uint8_t rowRequired = requiredBallForMethod(Gen4Wild::method(row));

        // A matching ordinary-wild row means the persistent fields do not prove
        // that this was a fixed-ball encounter. Ambiguity is unresolved, never
        // invalid by assumption.
        if (rowRequired == 0)
            return {};
        if (required == 0)
            required = rowRequired;
        else if (required != rowRequired)
            return {};
    }

    if (!matchedAny || required == 0)
        return {};
    return {true, required};
}

} // namespace Legality::Gen4FixedBall
