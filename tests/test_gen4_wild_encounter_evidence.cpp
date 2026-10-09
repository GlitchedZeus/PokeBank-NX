#include "Legality/Gen4WildEncounter.h"

#include <cassert>
#include <cstddef>
#include <iostream>

int main() {
    using namespace Legality::Gen4Wild;

    // The generator preserves every distinct persisted-identity/lead-history
    // tuple. Source aliases may therefore legitimately change the row count as
    // more Static/Magnet Pull provenance is retained. Do not freeze old counts;
    // prove that every generated row belongs to exactly one supported game and
    // that every game's source table contributed evidence.
    const auto diamond = countForGame("diamond_nds");
    const auto pearl = countForGame("pearl_nds");
    const auto platinum = countForGame("platinum_nds");
    const auto heartgold = countForGame("heartgold_nds");
    const auto soulsilver = countForGame("soulsilver_nds");
    assert(diamond > 0);
    assert(pearl > 0);
    assert(platinum > 0);
    assert(heartgold > 0);
    assert(soulsilver > 0);
    assert(diamond + pearl + platinum + heartgold + soulsilver ==
           std::size(kPackedGen4WildEncounters));
    assert(std::size(kPackedGen4WildEncounters) ==
           std::size(kPackedGen4WildLeadMeta));

    // Platinum Route 201 Starly is a grass-ground encounter and therefore
    // positive Poké Radar evidence. Great Marsh and HG/SS never are.
    assert(matches("platinum_nds", 396, 16, 2, 0));
    assert(!matches("platinum_nds", 396, 16, 50, 0));
    assert(hasRadarEligibleMatch("platinum_nds", 396, 16, 2, 0));
    const auto evolvedRadar =
        radarEvidence("platinum_nds", 397, 16, 2, 0);
    assert(evolvedRadar.matched);
    assert(evolvedRadar.evolved);
    assert(evolvedRadar.sourceSpecies == 396);
    assert(radarEvidence("platinum_nds", 398, 16, 2, 0).sourceSpecies == 396);
    assert(!radarEvidence("platinum_nds", 398, 16, 50, 0).matched);
    assert(!hasRadarEligibleMatch("diamond_nds", 24, 52, 20, 0));
    assert(!hasRadarEligibleMatch("heartgold_nds", 16, 149, 2, 0));

    // PKHeX HoneyTreeUtil trainer-ID restriction for Munchlax group-C trees.
    constexpr auto zeroTrees = munchlaxTreeIndices(0);
    static_assert(zeroTrees[0] == 0 && zeroTrees[1] == 1 &&
                  zeroTrees[2] == 2 && zeroTrees[3] == 3);
    constexpr auto documentedTrees = munchlaxTreeIndices(1935328924u);
    static_assert(documentedTrees[0] == 10 && documentedTrees[1] == 6 &&
                  documentedTrees[2] == 9 && documentedTrees[3] == 10);
    static_assert(isMunchlaxTreeLocation(0, 20));
    static_assert(isMunchlaxTreeLocation(0, 21));
    static_assert(isMunchlaxTreeLocation(0, 22));
    static_assert(!isMunchlaxTreeLocation(0, 23));

    // Raw table presence is broad, but positive provenance is trainer-ID-specific.
    assert(matches("diamond_nds", 446, 23, 5, 0));
    assert(matchesWithTrainerId("diamond_nds", 446, 20, 5, 0, 0));
    assert(!matchesWithTrainerId("diamond_nds", 446, 23, 5, 0, 0));

    // HeartGold Route 29 Pidgey.
    assert(matches("heartgold_nds", 16, 149, 2, 0));

    // Unown is random-form in the HGSS encounter resource (FormRandom=31).
    assert(matches("heartgold_nds", 201, 209, 5, 0));
    assert(matches("heartgold_nds", 201, 209, 5, 25));

    assert(hasSpecies("platinum_nds", 396));
    assert(!hasSpecies("unknown", 396));
    assert(countForGame("unknown") == 0);

    // The generator retains EncounterArea4.Rate so rate-dependent Method K
    // activation checks (Rock Smash / Bug Contest) never rely on guessed constants.
    std::size_t rockSmashRows = 0;
    for (const uint64_t row : kPackedGen4WildEncounters) {
        if (method(row) != 5) continue;
        ++rockSmashRows;
        assert(rate(row) != 0);
    }
    assert(rockSmashRows > 0);

    // Radar capability is generated from the pinned area ground-tile flags.
    // Source aliases with distinct lead-history tables can legitimately duplicate
    // a persisted row, so verify semantic invariants instead of stale row totals.
    std::size_t radarDiamond = 0, radarPearl = 0, radarPlatinum = 0;
    for (const uint64_t row : kPackedGen4WildEncounters) {
        if (!radarCapable(row)) continue;
        assert(game(row) != Game::HeartGold && game(row) != Game::SoulSilver);
        assert(location(row) != 52); // Great Marsh
        if (game(row) == Game::Diamond) ++radarDiamond;
        if (game(row) == Game::Pearl) ++radarPearl;
        if (game(row) == Game::Platinum) ++radarPlatinum;
    }
    assert(radarDiamond > 0);
    assert(radarPearl > 0);
    assert(radarPlatinum > 0);

    std::cout << "Gen IV wild encounter evidence: PASS\n";
}
