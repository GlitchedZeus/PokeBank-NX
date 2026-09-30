#include "Legality/Gen4CuteCharmPid.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4CuteCharmPid;

    // Female Cute Charm output uses the nature itself as PID.
    assert(matchesSurface(7, 1, 0x7F));
    assert(!matchesSurface(25, 1, 0x7F));

    // 50/50 species: buffered male Hardy base is 150; nature 10 => PID 160.
    assert(expectedMalePid(0x7F, 10) == 160);
    assert(matchesSurface(160, 0, 0x7F));
    // 159 is also valid (nature 9 => buffered base 150 + 9). Use 149 as a real miss.
    assert(!matchesSurface(149, 0, 0x7F));

    // Azurill's 75% female ratio buffers male PIDs into 200..224.
    assert(isAzurillBufferedMale(205));
    assert(matchesSurface(205, 0, 0xBF));

    // Fixed-gender/genderless species are not Cute Charm PID-modified.
    assert(!matchesSurface(7, 1, 0xFE));
    assert(!matchesSurface(7, 2, 0xFF));
    assert(!matchesSurface(0x100, 0, 0x7F));

    const auto shedinja = remapEncounterIdentity(292, 2, 7);
    assert(shedinja.species == 290 && shedinja.deriveGenderFromPid);

    const auto gallade = remapEncounterIdentity(475, 0, 160);
    assert(gallade.species == 281 && gallade.gender == 0);

    const auto azumarill = remapEncounterIdentity(184, 0, 205);
    assert(azumarill.species == 298 && azumarill.gender == 0);

    std::cout << "Gen IV Cute Charm PID surface: PASS\n";
}
