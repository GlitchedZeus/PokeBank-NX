#include "Legality/Gen4FixedBallEvidence.h"

#include <cassert>
#include <string_view>

using namespace Legality;

static_assert(Gen4FixedBall::requiredBallForMethod(0) == 0);
static_assert(Gen4FixedBall::requiredBallForMethod(7) == 0);
static_assert(Gen4FixedBall::requiredBallForMethod(8) == Gen4FixedBall::kSportBall);
static_assert(Gen4FixedBall::requiredBallForMethod(9) == 0);
static_assert(Gen4FixedBall::requiredBallForMethod(10) == Gen4FixedBall::kSafariBall);
static_assert(Gen4FixedBall::requiredBallForMethod(11) == Gen4FixedBall::kSafariBall);
static_assert(Gen4FixedBall::requiredBallForMethod(12) == Gen4FixedBall::kSafariBall);
static_assert(Gen4FixedBall::requiredBallForMethod(13) == Gen4FixedBall::kSafariBall);
static_assert(Gen4FixedBall::requiredBallForMethod(14) == Gen4FixedBall::kSafariBall);
static_assert(Gen4FixedBall::requiredBallForMethod(15) == 0);

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
    assert(!Gen4FixedBall::directWildEvidence(
        "unknown", 1, 1, 1, 0, 0).matched);
    assert(!Gen4FixedBall::directWildEvidence(
        "platinum_nds", 0, 1, 1, 0, 0).matched);
    assert(!Gen4FixedBall::directWildEvidence(
        "platinum_nds", 1, 1, 0, 0, 0).matched);

    bool sawBugContestSource = false;
    bool sawSafariSource = false;
    bool sawSourceProvenFixedBall = false;

    // Exercise the accepted generated encounter table rather than a synthetic
    // copy. A fixed-ball result is permitted only if every persistent-field
    // compatible row requires the same ball; mixed ordinary/fixed histories
    // intentionally remain unresolved.
    for (const uint64_t row : Gen4Wild::kPackedGen4WildEncounters) {
        const uint8_t method = Gen4Wild::method(row);
        if (method == 8)
            sawBugContestSource = true;
        if (method >= 10 && method <= 14)
            sawSafariSource = true;

        const uint8_t expected = Gen4FixedBall::requiredBallForMethod(method);
        if (expected == 0)
            continue;

        const auto evidence = Gen4FixedBall::directWildEvidence(
            gameId(Gen4Wild::game(row)),
            Gen4Wild::species(row),
            Gen4Wild::location(row),
            Gen4Wild::minLevel(row),
            Gen4Wild::form(row) >= 30 ? 0 : Gen4Wild::form(row),
            0);
        if (!evidence.matched)
            continue;

        assert(evidence.requiredBall == Gen4FixedBall::kSafariBall ||
               evidence.requiredBall == Gen4FixedBall::kSportBall);
        sawSourceProvenFixedBall = true;
    }

    assert(sawBugContestSource);
    assert(sawSafariSource);
    assert(sawSourceProvenFixedBall);
    return 0;
}
