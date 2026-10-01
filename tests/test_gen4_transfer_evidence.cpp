#include "Legality/Gen4TransferEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen4Transfer::Evidence;
    using Legality::Gen4Transfer::classify;
    using Legality::Gen4Transfer::classifyStoredFields;
    using Legality::Gen4Transfer::invalid;
    using Legality::Gen4Transfer::validPalParkBallFields;

    assert(classify(4, 0, false) == Evidence::NotApplicable);
    assert(classify(3, 0x37, false) == Evidence::PalParkMarker);
    assert(classify(3, 0x37, true) == Evidence::InvalidEggTransfer);
    assert(classify(3, 0x36, false) == Evidence::InvalidMissingPalParkMarker);
    assert(classify(3, 0, false) == Evidence::InvalidMissingPalParkMarker);
    assert(invalid(Evidence::InvalidEggTransfer));
    assert(invalid(Evidence::InvalidMissingPalParkMarker));
    assert(!invalid(Evidence::PalParkMarker));

    assert(classifyStoredFields(4, 0, 0, false) == Evidence::NotApplicable);
    assert(classifyStoredFields(3, 0x37, 0, false) ==
           Evidence::PalParkDiamondPearlFields);
    assert(classifyStoredFields(3, 0x37, 0x37, false) ==
           Evidence::PalParkPtHgssFields);
    assert(classifyStoredFields(3, 0x37, 0x37, true) ==
           Evidence::InvalidEggTransfer);
    assert(classifyStoredFields(3, 0x36, 0, false) ==
           Evidence::InvalidMissingPalParkMarker);
    assert(classifyStoredFields(3, 0x37, 0x36, false) ==
           Evidence::InvalidSplitLocationFields);
    assert(classifyStoredFields(3, 0x36, 0x37, false) ==
           Evidence::InvalidSplitLocationFields);

    assert(validPalParkBallFields(Evidence::PalParkDiamondPearlFields, 4, 0));
    assert(!validPalParkBallFields(Evidence::PalParkDiamondPearlFields, 4, 4));
    assert(!validPalParkBallFields(Evidence::PalParkDiamondPearlFields, 13, 0));

    // Platinum leaves the HGSS ball byte at zero; HGSS copies the Gen III ball.
    assert(validPalParkBallFields(Evidence::PalParkPtHgssFields, 4, 0));
    assert(validPalParkBallFields(Evidence::PalParkPtHgssFields, 4, 4));
    assert(!validPalParkBallFields(Evidence::PalParkPtHgssFields, 4, 3));
    assert(!validPalParkBallFields(Evidence::PalParkPtHgssFields, 17, 17));

    std::cout << "Gen III -> IV Pal Park transfer evidence: PASS\n";
}
