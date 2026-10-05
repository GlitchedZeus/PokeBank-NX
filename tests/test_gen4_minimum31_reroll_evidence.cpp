#include "Legality/Gen4Minimum31RerollEvidence.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t makeRow(uint8_t type) {
    return static_cast<uint64_t>(type) << 31;
}
}

int main() {
    using namespace Legality::Gen4Minimum31Reroll;

    static_assert(kMaximumAttempts == 4);
    static_assert(requiresMinimum31(makeRow(8)));  // Bug Contest
    static_assert(requiresMinimum31(makeRow(10))); // Safari Grass
    static_assert(requiresMinimum31(makeRow(11))); // Safari Surf
    static_assert(requiresMinimum31(makeRow(12))); // Safari Old Rod
    static_assert(requiresMinimum31(makeRow(13))); // Safari Good Rod
    static_assert(requiresMinimum31(makeRow(14))); // Safari Super Rod
    static_assert(!requiresMinimum31(makeRow(0)));
    static_assert(!requiresMinimum31(makeRow(7)));
    static_assert(!requiresMinimum31(makeRow(9)));

    // Independently derived from the pinned LCRNG constants. Origin 0 produces
    // IV words 21105/12720 and has no 31 component. Origin 3 produces
    // 54250/64452 and does contain a 31 component.
    static_assert(!attemptHasAny31(0u));
    static_assert(attemptHasAny31(3u));

    constexpr uint32_t nextFromZero = nextAttemptOrigin(0u);
    static_assert(nextFromZero == 0x8e425287u);
    constexpr auto zeroPrevious = previousAttempt(nextFromZero);
    static_assert(zeroPrevious.origin == 0u);
    static_assert(zeroPrevious.iv1 == 21105u);
    static_assert(zeroPrevious.iv2 == 12720u);
    static_assert(zeroPrevious.rejectedForNo31);

    constexpr uint32_t nextFromThree = nextAttemptOrigin(3u);
    constexpr auto threePrevious = previousAttempt(nextFromThree);
    static_assert(threePrevious.origin == 3u);
    static_assert(!threePrevious.rejectedForNo31);

    // A deterministic four-attempt shape: attempts 1/2/3 have no 31 and the
    // fourth does. This pins the exact five-call attempt stride without claiming
    // that slot/nature/lead/activation history is valid; recursive reachability is
    // deliberately a later verifier layer.
    constexpr uint32_t attempt1 = 1u;
    constexpr uint32_t attempt2 = nextAttemptOrigin(attempt1);
    constexpr uint32_t attempt3 = nextAttemptOrigin(attempt2);
    constexpr uint32_t attempt4 = nextAttemptOrigin(attempt3);
    static_assert(attempt2 == 2044959428u);
    static_assert(attempt3 == 3729347387u);
    static_assert(attempt4 == 2789130134u);
    static_assert(!attemptHasAny31(attempt1));
    static_assert(!attemptHasAny31(attempt2));
    static_assert(!attemptHasAny31(attempt3));
    static_assert(attemptHasAny31(attempt4));
    static_assert(previousAttempt(attempt4).origin == attempt3);
    static_assert(previousAttempt(attempt3).origin == attempt2);
    static_assert(previousAttempt(attempt2).origin == attempt1);

    assert(previousAttempt(nextFromZero).rejectedForNo31);
    assert(!previousAttempt(nextFromThree).rejectedForNo31);

    std::cout << "Gen IV minimum-31 reroll foundation: PASS\n";
}
