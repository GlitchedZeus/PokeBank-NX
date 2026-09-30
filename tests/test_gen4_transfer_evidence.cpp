#include "Legality/Gen4TransferEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen4Transfer::Evidence;
    using Legality::Gen4Transfer::classify;
    using Legality::Gen4Transfer::invalid;

    assert(classify(4, 0, false) == Evidence::NotApplicable);
    assert(classify(3, 0x37, false) == Evidence::PalParkMarker);
    assert(classify(3, 0x37, true) == Evidence::InvalidEggTransfer);
    assert(classify(3, 0x36, false) == Evidence::InvalidMissingPalParkMarker);
    assert(classify(3, 0, false) == Evidence::InvalidMissingPalParkMarker);
    assert(invalid(Evidence::InvalidEggTransfer));
    assert(invalid(Evidence::InvalidMissingPalParkMarker));
    assert(!invalid(Evidence::PalParkMarker));

    std::cout << "Gen III -> IV Pal Park transfer evidence: PASS\n";
}
