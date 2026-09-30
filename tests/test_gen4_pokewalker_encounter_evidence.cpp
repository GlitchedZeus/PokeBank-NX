#include "Legality/Gen4PokewalkerEncounter.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4PokewalkerEncounter;

    assert(count() == 162);
    assert(hasSpecies(115)); // Kangaskhan, Refreshing Field slot A.
    assert(hasSpecies(239)); // Elekid, Amity Meadow slot A.
    assert(!hasSpecies(493));

    const std::array<uint16_t, 4> kangaskhan{4, 43, 252, 0};
    const auto refreshing = match(115, 8, 1, kangaskhan);
    assert(refreshing.matched);
    assert(refreshing.course == 0);
    assert(refreshing.slot == 0);

    const std::array<uint16_t, 4> elekid{98, 43, 9, 0};
    const auto amity = match(239, 5, 0, elekid);
    assert(amity.matched);
    assert(amity.course == 26);
    assert(amity.slot == 0);

    assert(!match(115, 9, 1, kangaskhan).matched);
    assert(!match(115, 8, 0, kangaskhan).matched);

    std::cout << "Gen IV PokéWalker encounter evidence: PASS\n";
}
