#include "Legality/Gen4BugContestNoLeadEvidence.h"
#include "Legality/Gen4LeadFrameEvidence.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t makeRow(uint8_t type, uint8_t slot,
                           uint8_t minimum, uint8_t maximum,
                           uint8_t rate = 0) {
    return (static_cast<uint64_t>(minimum) << 17) |
           (static_cast<uint64_t>(maximum) << 24) |
           (static_cast<uint64_t>(type) << 31) |
           (static_cast<uint64_t>(slot) << 46) |
           (static_cast<uint64_t>(rate) << 50);
}
}

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

    // No-lead / Sweet Scent Bug Contest history uses the same minimum-31 retry
    // geometry but proves slot + level only at the original encounter attempt.
    // These vectors are discriminating: the persisted attempt does not prove
    // the same source row directly or at a shallower reroll depth.
    using namespace Legality::Gen4BugContestNoLead;

    constexpr uint32_t depth1Seed = 81u;
    constexpr uint32_t depth1Pid = sequentialPid(depth1Seed);
    constexpr uint64_t depth1Row = makeRow(8, 1, 7, 18, 25);
    static_assert(depth1Pid == 0x7EF1CFBFu);
    static_assert(directMinimum31Satisfied(depth1Seed));
    static_assert(!matchAttempt(depth1Row, depth1Seed, depth1Pid, 18).matched());
    constexpr auto depth1 = matchReroll(
        depth1Row, depth1Seed, depth1Pid, 18, 1);
    static_assert(depth1.matched());
    static_assert(depth1.rerollDepth == 1);
    static_assert(depth1.slot == 1);
    static_assert(depth1.level == 18);
    static_assert(match(depth1Row, depth1Seed, depth1Pid, 18).rerollDepth == 1);

    constexpr uint32_t depth2Seed = 280u;
    constexpr uint32_t depth2Pid = sequentialPid(depth2Seed);
    constexpr uint64_t depth2Row = makeRow(8, 3, 7, 18, 25);
    static_assert(depth2Pid == 0xCB57F0E6u);
    static_assert(directMinimum31Satisfied(depth2Seed));
    static_assert(!matchAttempt(depth2Row, depth2Seed, depth2Pid, 14).matched());
    static_assert(!matchReroll(
        depth2Row, depth2Seed, depth2Pid, 14, 1).matched());
    constexpr auto depth2 = matchReroll(
        depth2Row, depth2Seed, depth2Pid, 14, 2);
    static_assert(depth2.matched());
    static_assert(depth2.rerollDepth == 2);
    static_assert(depth2.slot == 3);
    static_assert(depth2.level == 14);
    static_assert(match(depth2Row, depth2Seed, depth2Pid, 14).rerollDepth == 2);

    constexpr uint32_t depth3Seed = 24094u;
    constexpr uint32_t depth3Pid = sequentialPid(depth3Seed);
    constexpr uint64_t depth3Row = makeRow(8, 5, 7, 18, 25);
    static_assert(depth3Pid == 0x6D3F8609u);
    static_assert(!directMinimum31Satisfied(depth3Seed));
    static_assert(!matchReroll(
        depth3Row, depth3Seed, depth3Pid, 18, 1).matched());
    static_assert(!matchReroll(
        depth3Row, depth3Seed, depth3Pid, 18, 2).matched());
    constexpr auto depth3 = matchReroll(
        depth3Row, depth3Seed, depth3Pid, 18, 3);
    static_assert(depth3.matched());
    static_assert(depth3.rerollDepth == 3);
    static_assert(depth3.slot == 5);
    static_assert(depth3.level == 18);
    static_assert(match(depth3Row, depth3Seed, depth3Pid, 18).rerollDepth == 3);

    std::cout << "Gen IV minimum-31 reroll frame geometry: PASS\n";
}
