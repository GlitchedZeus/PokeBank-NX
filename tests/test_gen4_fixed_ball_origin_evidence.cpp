#include "Legality/Gen4FixedBallOriginEvidence.h"

#include <cassert>
#include <string_view>

using namespace Legality;

namespace {

std::string_view gameId(Gen4Wild::Game game) {
    switch (game) {
        case Gen4Wild::Game::Diamond: return "diamond_nds";
        case Gen4Wild::Game::Pearl: return "pearl_nds";
        case Gen4Wild::Game::Platinum: return "platinum_nds";
        case Gen4Wild::Game::HeartGold: return "heartgold_nds";
        case Gen4Wild::Game::SoulSilver: return "soulsilver_nds";
        default: return {};
    }
}

} // namespace

int main() {
    using namespace Gen4FixedBallOrigin;

    // Synthetic accumulator contracts permanently pin ambiguity semantics.
    RequirementAccumulator sportOnly{};
    sportOnly.observeMethod(8);
    assert(sportOnly.resolution() == Resolution::RequiredBallKnown);
    assert(sportOnly.requiredBall == Gen4FixedBall::kSportBall);

    // Direct fixed-ball evidence plus an ordinary-wild ancestry/history
    // alternative must stay unresolved.
    RequirementAccumulator evolutionAmbiguity{};
    evolutionAmbiguity.observeMethod(8);
    evolutionAmbiguity.observeMethod(0);
    assert(evolutionAmbiguity.resolution() == Resolution::Unresolved);
    assert(evolutionAmbiguity.sawOrdinary);

    // Conflicting fixed-ball histories are ambiguity, never a hard verdict.
    RequirementAccumulator conflicting{};
    conflicting.observeMethod(8);
    conflicting.observeMethod(10);
    assert(conflicting.resolution() == Resolution::Conflicting);

    bool sawUniqueSportWildHistory = false;
    bool sawUniqueSafariWildHistory = false;

    // Exercise the accepted generated table and the evolution-aware resolver.
    // We only elevate to globally-known below when the caller explicitly marks
    // competing-family support complete and reports no compatible alternatives.
    for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
        const uint8_t expected =
            Gen4FixedBall::requiredBallForMethod(Gen4Wild::method(row));
        if (expected == 0)
            continue;

        const auto wild = wildHistoryEvidence(
            gameId(Gen4Wild::game(row)),
            Gen4Wild::species(row),
            Gen4Wild::location(row),
            Gen4Wild::minLevel(row),
            Gen4Wild::form(row) >= 30 ? 0 : Gen4Wild::form(row),
            0);

        if (wild.resolution != Resolution::RequiredBallKnown ||
            wild.requiredBall != expected)
            continue;

        CompetingHistory completeNoAlternative{};
        completeNoAlternative.supportComplete = true;
        const auto resolved =
            applyCompetingHistory(wild, completeNoAlternative);
        assert(resolved.resolution == Resolution::RequiredBallKnown);
        assert(resolved.requiredBall == expected);

        if (expected == Gen4FixedBall::kSportBall)
            sawUniqueSportWildHistory = true;
        if (expected == Gen4FixedBall::kSafariBall)
            sawUniqueSafariWildHistory = true;
    }

    assert(sawUniqueSportWildHistory);
    assert(sawUniqueSafariWildHistory);

    // Unknown/unreconstructed source-family support can never escalate.
    WildEvidence knownWild{
        Resolution::RequiredBallKnown,
        Gen4FixedBall::kSafariBall,
        1,
        false,
        false
    };
    assert(applyCompetingHistory(knownWild, {}).resolution ==
           Resolution::Unresolved);

    // A source-backed compatible static/gift, event, trade, Pokewalker, or
    // other history blocks exclusivity even when the wild family is unique.
    CompetingHistory staticOverlap{};
    staticOverlap.supportComplete = true;
    staticOverlap.staticOrGift = true;
    assert(applyCompetingHistory(knownWild, staticOverlap).resolution ==
           Resolution::Unresolved);

    CompetingHistory eventOverlap{};
    eventOverlap.supportComplete = true;
    eventOverlap.event = true;
    assert(applyCompetingHistory(knownWild, eventOverlap).resolution ==
           Resolution::Unresolved);

    CompetingHistory tradeOverlap{};
    tradeOverlap.supportComplete = true;
    tradeOverlap.trade = true;
    assert(applyCompetingHistory(knownWild, tradeOverlap).resolution ==
           Resolution::Unresolved);

    CompetingHistory pokewalkerOverlap{};
    pokewalkerOverlap.supportComplete = true;
    pokewalkerOverlap.pokewalker = true;
    assert(applyCompetingHistory(knownWild, pokewalkerOverlap).resolution ==
           Resolution::Unresolved);

    CompetingHistory otherOverlap{};
    otherOverlap.supportComplete = true;
    otherOverlap.other = true;
    assert(applyCompetingHistory(knownWild, otherOverlap).resolution ==
           Resolution::Unresolved);

    // Shedinja has special PKHeX ball history and must remain unresolved until
    // that creation history is reconstructed explicitly.
    const auto shedinja = wildHistoryEvidence(
        "heartgold_nds", 292, 1, 20, 0, 0);
    assert(shedinja.resolution == Resolution::Unresolved);

    return 0;
}
