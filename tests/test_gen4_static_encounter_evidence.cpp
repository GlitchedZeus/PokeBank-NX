#include "Legality/Gen4StaticEncounter.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Static;

    assert(countForGame("diamond_nds") == 24);
    assert(countForGame("pearl_nds") == 24);
    assert(countForGame("platinum_nds") == 35);
    assert(countForGame("heartgold_nds") == 57);
    assert(countForGame("soulsilver_nds") == 57);

    // Diamond Dialga / Pearl Palkia are version-specific static encounters.
    assert(matches("diamond_nds", 483, 51, 47, 0, 0));
    assert(!matches("pearl_nds", 483, 51, 47, 0, 0));
    assert(matches("pearl_nds", 484, 51, 47, 0, 0));

    // Platinum Distortion World Giratina is Origin Forme.
    assert(matches("platinum_nds", 487, 117, 47, 1, 0));
    assert(!matches("platinum_nds", 487, 117, 47, 0, 0));

    // D/P Riolu egg requires its exact egg-location evidence.
    assert(matches("diamond_nds", 447, 40, 0, 0, 2010));
    assert(!matches("diamond_nds", 447, 40, 0, 0, 0));
    assert(!matches("diamond_nds", 447, 40, 1, 0, 2010));

    // HG/SS Spiky-eared Pichu is form 1 at Ilex Forest.
    assert(matches("heartgold_nds", 172, 214, 30, 1, 0));
    assert(!matches("heartgold_nds", 172, 214, 30, 0, 0));

    // Roamers use a permitted route set rather than one fixed met location.
    assert(matches("platinum_nds", 481, 20, 50, 0, 0));
    assert(!matches("platinum_nds", 481, 100, 50, 0, 0));
    assert(matches("heartgold_nds", 243, 180, 40, 0, 0));

    // Version-exclusive cover legends.
    assert(matches("heartgold_nds", 250, 205, 45, 0, 0));
    assert(matches("soulsilver_nds", 249, 218, 45, 0, 0));

    assert(hasSpecies("platinum_nds", 492));
    assert(!hasSpecies("unknown", 492));
    assert(countForGame("unknown") == 0);

    std::cout << "Gen IV static/gift encounter evidence: PASS\n";
}
