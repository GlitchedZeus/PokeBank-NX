#include "Legality/Gen3SafariEncounter.h"

#include <array>
#include <cassert>
#include <cstddef>

using namespace Legality;

namespace {

std::string_view gameId(Gen3Safari::Game game) {
    switch (game) {
        case Gen3Safari::Game::Ruby: return "ruby_gba";
        case Gen3Safari::Game::Sapphire: return "sapphire_gba";
        case Gen3Safari::Game::Emerald: return "emerald_gba";
        case Gen3Safari::Game::FireRed: return "firered_gba";
        case Gen3Safari::Game::LeafGreen: return "leafgreen_gba";
        case Gen3Safari::Game::Invalid: return {};
    }
    return {};
}

} // namespace

int main() {
    using namespace Gen3Safari;

    static_assert(kSafariBall == 5);
    static_assert(kSafariLocationRSE == 57);
    static_assert(kSafariLocationFRLG == 136);
    static_assert(safariLocation(Game::Ruby) == 57);
    static_assert(safariLocation(Game::Emerald) == 57);
    static_assert(safariLocation(Game::FireRed) == 136);
    static_assert(safariLocation(Game::LeafGreen) == 136);

    // Exact pinned resource-row counts are permanent source-regeneration guards.
    assert(countForGame("ruby_gba") == 61);
    assert(countForGame("sapphire_gba") == 73);
    assert(countForGame("emerald_gba") == 117);
    assert(countForGame("firered_gba") == 63);
    assert(countForGame("leafgreen_gba") == 63);

    std::array<bool, 6> sawRseMethod{};
    std::array<bool, 6> sawFrlgMethod{};

    for (const Entry& row : kGen3SafariEntries) {
        const auto game = static_cast<Game>(row.game);
        assert(isSafariLocation(game, row.location));
        assert(row.species != 0);
        assert(row.minLevel != 0);
        assert(row.maxLevel >= row.minLevel);
        assert(row.method <= 5);

        const auto evidence = directWildEvidence(
            gameId(game), row.species, row.location, row.minLevel, row.form);
        assert(evidence.matched);
        assert(evidence.requiredBall == kSafariBall);
        assert(evidence.candidateRows >= 1);

        if (game == Game::Ruby || game == Game::Sapphire ||
            game == Game::Emerald)
            sawRseMethod[row.method] = true;
        else
            sawFrlgMethod[row.method] = true;
    }

    // The real pinned Safari areas use every ordinary RSE wild method including
    // Rock Smash, while FR/LG Safari areas contain grass/surf/fishing only.
    for (bool saw : sawRseMethod)
        assert(saw);
    for (std::size_t i = 0; i < 5; ++i)
        assert(sawFrlgMethod[i]);
    assert(!sawFrlgMethod[5]);

    assert(!directWildEvidence("unknown", 1, 57, 5, 0).matched);
    assert(!directWildEvidence("ruby_gba", 0, 57, 5, 0).matched);
    assert(!directWildEvidence("ruby_gba", 1, 57, 0, 0).matched);
    assert(!directWildEvidence("ruby_gba", 1, 136, 5, 0).matched);
    assert(!directWildEvidence("firered_gba", 1, 57, 5, 0).matched);

    return 0;
}
