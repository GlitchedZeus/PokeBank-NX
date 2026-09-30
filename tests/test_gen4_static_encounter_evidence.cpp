#include "Legality/Gen4StaticEncounter.h"

#include <cassert>
#include <iostream>
#include <string_view>

int main() {
    using namespace Legality::Gen4Static;

    assert(countForGame("diamond_nds") == 24);
    assert(countForGame("pearl_nds") == 24);
    assert(countForGame("platinum_nds") == 35);
    assert(countForGame("heartgold_nds") == 57);
    assert(countForGame("soulsilver_nds") == 57);

    const auto match = [](std::string_view game, uint16_t species,
                          uint16_t location, uint8_t level, uint8_t form,
                          uint16_t eggLocation, uint8_t ball = 4,
                          uint8_t gender = 0, uint8_t nature = 0,
                          bool shiny = false, bool fateful = false) {
        return matches(
            game, species, location, level, form, eggLocation, ball,
            gender, nature, shiny, fateful);
    };

    // Diamond Dialga / Pearl Palkia are version-specific static encounters.
    assert(match("diamond_nds", 483, 51, 47, 0, 0));
    assert(!match("pearl_nds", 483, 51, 47, 0, 0));
    assert(match("pearl_nds", 484, 51, 47, 0, 0));

    // Platinum Distortion World Giratina is Origin Forme.
    assert(match("platinum_nds", 487, 117, 47, 1, 0));
    assert(!match("platinum_nds", 487, 117, 47, 0, 0));

    // D/P Riolu egg requires its exact egg-location evidence and fixed Poke Ball.
    assert(match("diamond_nds", 447, 40, 0, 0, 2010, 4));
    assert(!match("diamond_nds", 447, 40, 0, 0, 2010, 2));
    assert(!match("diamond_nds", 447, 40, 0, 0, 0, 4));
    assert(!match("diamond_nds", 447, 40, 1, 0, 2010, 4));

    // Diamond gift Eevee is fixed to a Poke Ball.
    assert(match("diamond_nds", 133, 10, 5, 0, 0, 4));
    assert(!match("diamond_nds", 133, 10, 5, 0, 0, 1));

    // Platinum Shaymin's released static template is fateful.
    assert(match("platinum_nds", 492, 63, 30, 0, 0, 4, 0, 0, false, true));
    assert(!match("platinum_nds", 492, 63, 30, 0, 0, 4, 0, 0, false, false));

    // Lake of Rage Gyarados is forced shiny.
    assert(match("heartgold_nds", 130, 135, 30, 0, 0, 4, 0, 0, true, false));
    assert(match("soulsilver_nds", 130, 135, 30, 0, 0, 4, 0, 0, true, false));
    assert(!match("heartgold_nds", 130, 135, 30, 0, 0, 4, 0, 0, false, false));

    // HG/SS Spiky-eared Pichu is fixed female, Naughty, non-shiny, form 1.
    assert(match("heartgold_nds", 172, 214, 30, 1, 0, 4, 1, 4, false, false));
    assert(!match("heartgold_nds", 172, 214, 30, 1, 0, 4, 0, 4, false, false));
    assert(!match("heartgold_nds", 172, 214, 30, 1, 0, 4, 1, 3, false, false));
    assert(!match("heartgold_nds", 172, 214, 30, 1, 0, 4, 1, 4, true, false));

    // Roamers use a permitted route set rather than one fixed met location.
    assert(match("platinum_nds", 481, 20, 50, 0, 0));
    assert(!match("platinum_nds", 481, 100, 50, 0, 0));
    assert(match("heartgold_nds", 243, 180, 40, 0, 0));

    // Version-exclusive cover legends.
    assert(match("heartgold_nds", 250, 205, 45, 0, 0));
    assert(match("soulsilver_nds", 249, 218, 45, 0, 0));

    assert(hasSpecies("platinum_nds", 492));
    assert(!hasSpecies("unknown", 492));
    assert(countForGame("unknown") == 0);

    std::cout << "Gen IV static/gift encounter evidence: PASS\n";
}
