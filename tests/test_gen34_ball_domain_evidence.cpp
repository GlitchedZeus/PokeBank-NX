#include "Legality/Gen34BallDomainEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen34BallDomain::Result;
    using Legality::Gen34BallDomain::classify;

    // Zero remains unresolved: this contract proves representable nonzero IDs only.
    static_assert(classify(3, 0) == Result::Unresolved);
    static_assert(classify(4, 0) == Result::Unresolved);

    // PKHeX Legal.MaxBallID_3 == 12.
    static_assert(classify(3, 1) == Result::WithinGenerationDomain);
    static_assert(classify(3, 12) == Result::WithinGenerationDomain);
    static_assert(classify(3, 13) == Result::Invalid);
    static_assert(classify(3, 24) == Result::Invalid);

    // PKHeX Legal.MaxBallID_4 == 24.
    static_assert(classify(4, 1) == Result::WithinGenerationDomain);
    static_assert(classify(4, 12) == Result::WithinGenerationDomain);
    static_assert(classify(4, 16) == Result::WithinGenerationDomain); // Cherish: provenance-specific.
    static_assert(classify(4, 23) == Result::WithinGenerationDomain); // Moon/Apricorn: game-specific.
    static_assert(classify(4, 24) == Result::WithinGenerationDomain); // Sport: encounter-specific.
    static_assert(classify(4, 25) == Result::Invalid);
    static_assert(classify(4, 37) == Result::Invalid);

    // Gen I/II do not persist ball info in their entity formats; later generations are out of lane.
    static_assert(classify(1, 4) == Result::Unresolved);
    static_assert(classify(2, 4) == Result::Unresolved);
    static_assert(classify(5, 4) == Result::Unresolved);

    assert(Legality::Gen34BallDomain::isInvalid(3, 13));
    assert(Legality::Gen34BallDomain::isInvalid(4, 25));
    assert(!Legality::Gen34BallDomain::isInvalid(3, 12));
    assert(!Legality::Gen34BallDomain::isInvalid(4, 24));

    std::cout << "Gen III-IV ball domain evidence tests passed\n";
    return 0;
}
