#include "Legality/Gen2StaticEncounter.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen2Static;

    assert(countForGame("gold_gbc") > 20);
    assert(countForGame("silver_gbc") > 20);
    assert(countForGame("crystal_gbc") > 30);

    // Gold/Silver do not preserve original encounter level. Use a current level
    // between the version-specific encounter levels so compatibility remains truthful.
    assert(matches("gold_gbc", 250, 50, 0, false, false));   // Ho-Oh starts at 40.
    assert(!matches("silver_gbc", 250, 50, 0, false, false)); // Silver Ho-Oh starts at 70.
    assert(!matches("gold_gbc", 249, 50, 0, false, false));   // Gold Lugia starts at 70.
    assert(matches("silver_gbc", 249, 50, 0, false, false));  // Silver Lugia starts at 40.

    // Crystal Suicune at Tin Tower: caught data stores met level/location.
    const uint16_t suicuneCaught = static_cast<uint16_t>((40u << 8) | 23u);
    assert(matches("crystal_gbc", 245, 40, suicuneCaught, false, false));
    const uint16_t wrongLocation = static_cast<uint16_t>((40u << 8) | 24u);
    assert(!matches("crystal_gbc", 245, 40, wrongLocation, false, false));

    // Red Gyarados requires shiny evidence.
    const uint16_t lakeOfRage = static_cast<uint16_t>((30u << 8) | 38u);
    assert(matches("crystal_gbc", 130, 30, lakeOfRage, false, true));
    assert(!matches("crystal_gbc", 130, 30, lakeOfRage, false, false));

    // Crystal Odd Egg tranche is positive only while still unhatched.
    assert(matches("crystal_gbc", 172, 5, 0, true, false));
    assert(matches("crystal_gbc", 172, 5, 0, true, true));

    assert(hasSpecies("gold_gbc", 250));
    assert(hasSpecies("crystal_gbc", 172));
    assert(!hasSpecies("unknown", 172));

    std::cout << "Gen II static/gift encounter evidence: PASS\n";
}
