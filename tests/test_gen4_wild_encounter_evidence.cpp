#include "Legality/Gen4WildEncounter.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Wild;

    assert(countForGame("diamond_nds") == 1878);
    assert(countForGame("pearl_nds") == 1877);
    assert(countForGame("platinum_nds") == 1854);
    assert(countForGame("heartgold_nds") == 2616);
    assert(countForGame("soulsilver_nds") == 2622);

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

    std::cout << "Gen IV wild encounter evidence: PASS\n";
}
