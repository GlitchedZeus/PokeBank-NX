#include "Legality/Gen4WildEncounter.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Wild;

    // Slot number is part of the evidence identity now. The pinned resource contains
    // 18,826 distinct Gen IV wild-slot rows across the five games.
    const auto diamond = countForGame("diamond_nds");
    const auto pearl = countForGame("pearl_nds");
    const auto platinum = countForGame("platinum_nds");
    const auto heartgold = countForGame("heartgold_nds");
    const auto soulsilver = countForGame("soulsilver_nds");
    assert(diamond > 1800);
    assert(pearl > 1800);
    assert(platinum > 1800);
    assert(heartgold > 2500);
    assert(soulsilver > 2500);
    assert(diamond + pearl + platinum + heartgold + soulsilver == 18826);

    // Platinum Route 201 Starly.
    assert(matches("platinum_nds", 396, 16, 2, 0));
    assert(!matches("platinum_nds", 396, 16, 50, 0));

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

    std::cout << "Gen IV wild encounter evidence: PASS\n";
}
