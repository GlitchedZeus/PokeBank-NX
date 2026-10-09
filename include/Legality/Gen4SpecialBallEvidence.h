#pragma once
#include "Legality/Gen4WildEncounter.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen4SpecialBallEvidence {

// Positive source-affinity only. Pinned PKHeX
// kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722
// EncounterSlot4.GetRequiredBallValue and BallVerifier.VerifyBall:
// Bug Catching Contest -> Sport Ball (24); HGSS Safari slots -> Safari
// Ball (5). A missing match is UNRESOLVED; it is never evidence that
// another acquisition / evolved / transferred history is impossible.
// D/P/Pt Great Marsh and Nincada -> Shedinja exceptions are out of scope.
enum class Affinity : uint8_t { None, Sport, Safari, GreatMarsh, ShedinjaBugContest, ApricornWild };

constexpr Affinity matchDirectRow(uint64_t row, uint8_t ball) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (type == 8 && ball == 24)
        return Affinity::Sport;
    // Pinned BallUseLegality.WildPokeBalls4_HGSS includes the seven Kurt
    // Apricorn Balls (17..23). These are NOT a D/P/Pt direct wild option;
    // HGSS BCC and Safari source types have their own fixed ball instead.
    if (ball >= 17 && ball <= 23 && type <= 7 &&
        (Gen4Wild::game(row) == Gen4Wild::Game::HeartGold ||
         Gen4Wild::game(row) == Gen4Wild::Game::SoulSilver))
        return Affinity::ApricornWild;
    if (type >= 10 && type <= 14 && ball == 5 &&
        (Gen4Wild::game(row) == Gen4Wild::Game::HeartGold ||
         Gen4Wild::game(row) == Gen4Wild::Game::SoulSilver))
        return Affinity::Safari;
    // Pinned Locations4.IsSafariBallRequired: D/P/Pt Great Marsh is
    // met location 52 (unlike HGSS Safari Zone 202). Other encounters
    // from the same location, including surfing, share the fixed ball.
    // This is only a POSITIVE direct-source affinity, not a proof that
    // a nonmatching ball record has impossible transfer/egg history.
    const auto game = Gen4Wild::game(row);
    if (ball == 5 && Gen4Wild::location(row) == 52 &&
        (game == Gen4Wild::Game::Diamond ||
         game == Gen4Wild::Game::Pearl ||
         game == Gen4Wild::Game::Platinum))
        return Affinity::GreatMarsh;
    return Affinity::None;
}

inline Affinity analyzeSupported(std::string_view exactGameId,
                                 uint16_t species, uint16_t metLocation,
                                 uint8_t metLevel, uint8_t form,
                                 uint8_t ball) noexcept {
    if (ball == 0 || metLevel == 0 || metLocation > 0xFF || species == 0)
        return Affinity::None;
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid)
        return Affinity::None;

    // Pinned PKHeX BallVerifier.VerifyEvolvedShedinja:
    // a Gen IV BCC Nincada (#290) -> Shedinja (#292) may retain a Sport
    // Ball or revert to Poké Ball. PK4 retains the capture's met data.
    // This is source-compatible history only, not a unique provenance
    // or evidence that unrelated origins are impossible.
    if (species == 292 && form == 0 && (ball == 24 || ball == 4) &&
        (wanted == Gen4Wild::Game::HeartGold ||
         wanted == Gen4Wild::Game::SoulSilver)) {
        for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
            if (Gen4Wild::game(row) == wanted &&
                Gen4Wild::species(row) == 290 &&
                Gen4Wild::method(row) == 8 &&
                Gen4Wild::location(row) == metLocation &&
                Gen4Wild::levelMatches(row, metLevel) &&
                Gen4Wild::formMatches(Gen4Wild::form(row), 0))
                return Affinity::ShedinjaBugContest;
        }
    }

    for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
        if (Gen4Wild::game(row) != wanted ||
            Gen4Wild::species(row) != species ||
            Gen4Wild::location(row) != metLocation ||
            !Gen4Wild::levelMatches(row, metLevel) ||
            !Gen4Wild::formMatches(Gen4Wild::form(row), form))
            continue;
        const Affinity proof = matchDirectRow(row, ball);
        if (proof != Affinity::None)
            return proof;
    }
    return Affinity::None;
}
} // namespace Legality::Gen4SpecialBallEvidence
