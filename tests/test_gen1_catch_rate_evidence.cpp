#include "Legality/Gen1CatchRateEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen1CatchRate;

    assert(expectedRate("red_gb", 1) == 45);
    assert(expectedRate("blue_gb", 1) == 45);
    assert(expectedRate("yellow_gb", 1) == 45);

    // Exact game differences in the pinned Gen I personal tables.
    assert(expectedRate("red_gb", 25) == 190);
    assert(expectedRate("yellow_gb", 25) == 163);
    assert(expectedRate("red_gb", 64) == 100);
    assert(expectedRate("yellow_gb", 64) == 96);
    assert(expectedRate("red_gb", 148) == 45);
    assert(expectedRate("yellow_gb", 148) == 27);
    assert(expectedRate("red_gb", 149) == 45);
    assert(expectedRate("yellow_gb", 149) == 9);

    assert(classify("red_gb", 1, 45) == Evidence::NativeSpeciesRate);
    assert(classify("yellow_gb", 25, 163) == Evidence::NativeSpeciesRate);

    // 0x01 is a valid Gen II item byte according to the pinned ItemConverter bitset.
    assert(isPossibleTimeCapsuleHeldItem(0x01));
    assert(classify("red_gb", 1, 0x01) == Evidence::PossibleTimeCapsuleHeldItem);

    assert(classify("unknown", 1, 45) == Evidence::Unknown);
    assert(expectedRate("red_gb", 0) == 0);

    std::cout << "Gen I catch-rate legality evidence: PASS\n";
}
