#include "Legality/Gen4HatchLocationEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Hatch;

    static_assert(LocationPermitted.size() == 235);

    // Shared Sinnoh locations.
    assert(isValidForOrigin(1, 10));  // Diamond
    assert(isValidForOrigin(1, 11));  // Pearl
    assert(isValidForOrigin(1, 12));  // Platinum
    assert(!isValidForOrigin(1, 7));  // not HG/SS

    // Platinum-only locations include Flower Paradise / Seabreak Path area ids.
    assert(!isValidForOrigin(63, 10));
    assert(isValidForOrigin(63, 12));
    assert(!isValidForOrigin(85, 10));
    assert(isValidForOrigin(85, 12));

    // Johto/Kanto-side HG/SS hatch locations.
    assert(isValidForOrigin(126, 7));
    assert(isValidForOrigin(126, 8));
    assert(!isValidForOrigin(126, 12));

    // Location 80 is explicitly permitted across all three Gen IV groups.
    assert(isValidForOrigin(80, 10));
    assert(isValidForOrigin(80, 12));
    assert(isValidForOrigin(80, 7));

    // A traded egg keeps its origin version but may hatch in a different Gen IV game.
    assert(!isValidHatchedEgg(10, 2000, 126));
    assert(isValidHatchedEgg(10, LinkTrade4, 126));
    assert(isValidHatchedEgg(7, LinkTrade4, 63));

    assert(!isValidForOrigin(235, 12));
    assert(!isValidForOrigin(1, 0));
    assert(!isValidHatchedEgg(0, 2000, 1));

    std::cout << "Gen IV hatch-location evidence: PASS\n";
}
