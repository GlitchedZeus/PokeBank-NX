#include "Legality/Gen2WildEncounter.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen2Wild;
    assert(countForGame(Game::Gold)==3272);
    assert(countForGame(Game::Silver)==3272);
    assert(countForGame(Game::Crystal)==3312);

    assert(hasSpecies("gold_gbc",72));   // Tentacool
    assert(hasSpecies("crystal_gbc",72));
    assert(!hasSpecies("unknown",72));

    // Crystal Route/location 1 Surf Tentacool level 20, time 1 (allowed by Any mask).
    const uint16_t caught=static_cast<uint16_t>((1u<<14)|(20u<<8)|1u);
    assert(matchesCrystalCaughtData(72,caught));
    const uint16_t badLevel=static_cast<uint16_t>((1u<<14)|(63u<<8)|1u);
    assert(!matchesCrystalCaughtData(72,badLevel));
    const uint16_t badLocation=static_cast<uint16_t>((1u<<14)|(20u<<8)|127u);
    assert(!matchesCrystalCaughtData(72,badLocation));

    std::cout<<"Gen II wild encounter evidence: PASS\n";
}
