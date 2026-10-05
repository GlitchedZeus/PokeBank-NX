#include "Legality/Gen4LeadFrameEvidence.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using namespace Legality::Gen4LeadFrame;
    using Legality::Gen3PidIv::Detail::prev;

    // Pinned Method K recursion geometry: immediately before a new attempt's
    // nature/sync frame sit IV2, IV1, PID-high, PID-low, and the previous
    // attempt's nature/sync frame.
    constexpr uint32_t seed = 0x12345678u;
    constexpr uint32_t p1 = prev(seed);
    constexpr uint32_t p2 = prev(p1);
    constexpr uint32_t p3 = prev(p2);
    constexpr uint32_t p4 = prev(p3);
    constexpr uint32_t p5 = prev(p4);
    static_assert(previousRerollIv2Word(seed) ==
                  static_cast<uint16_t>((p1 >> 16) & 0x7FFFu));
    static_assert(previousRerollIv1Word(seed) ==
                  static_cast<uint16_t>((p2 >> 16) & 0x7FFFu));
    static_assert(previousRerollNatureSeed(seed) == p5);

    // Seed 3 has a direct minimum-31 result and can structurally occur after
    // zero, one, or two earlier rejected attempts. These checks only validate
    // the IV gate; lead/nature/slot history still has to be reconstructed.
    static_assert(directMinimum31Satisfied(3u));
    static_assert(minimum31IvChainAllows(3u, 0));
    static_assert(minimum31IvChainAllows(3u, 1));
    static_assert(minimum31IvChainAllows(3u, 2));

    // Seed 2 has no 31 IV in the persisted attempt. Pinned HG/SS behavior can
    // accept such a result only after all three previous attempts were rejected
    // and the fourth/final attempt is exhausted.
    static_assert(!directMinimum31Satisfied(2u));
    static_assert(!minimum31IvChainAllows(2u, 0));
    static_assert(!minimum31IvChainAllows(2u, 1));
    static_assert(!minimum31IvChainAllows(2u, 2));
    static_assert(minimum31IvChainAllows(2u, 3));
    static_assert(!minimum31IvChainAllows(2u, 4));

    uint32_t cursor = 2u;
    for (int i = 0; i < 3; ++i) {
        assert(previousRerollAttemptRejected(cursor));
        cursor = previousRerollNatureSeed(cursor);
    }

    std::cout << "Gen IV minimum-31 reroll frame geometry: PASS\n";
}
