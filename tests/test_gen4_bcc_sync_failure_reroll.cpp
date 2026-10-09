#include "Legality/Gen4BugContestSynchronizeFailureEvidence.h"

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
    using namespace Legality;
    using namespace Gen4BugContestSynchronizeFailure;
    using Gen4LeadFrame::sequentialPid;

    // Depth 1: final attempt itself maps to a different BCC slot/level. Only
    // walking through one rejected failed-Sync attempt reaches slot 0 / level 9.
    constexpr uint32_t depth1Seed = 4423u;
    constexpr uint32_t depth1Pid = sequentialPid(depth1Seed);
    constexpr uint64_t depth1Row = makeRow(8, 0, 7, 18, 100);
    static_assert(depth1Pid == 0xB0D86935u);
    static_assert(previousFailedSyncNatureSeed(depth1Seed) == 0x05E84661u);
    static_assert(previousAttemptRejected(depth1Seed));
    static_assert(!Gen4LeadFailure::matchAttempt(
        true, depth1Row, depth1Seed, depth1Pid, 9,
        Gen4LeadFailure::Lead::Synchronize).matched());
    constexpr auto depth1 = matchReroll(
        depth1Row, depth1Seed, depth1Pid, 9, 1);
    static_assert(depth1.matched());
    static_assert(depth1.slot == 0);
    static_assert(depth1.level == 9);
    static_assert(depth1.rerollDepth == 1);
    static_assert(depth1.encounterSeed == 0x05E84661u);
    static_assert(!matchReroll(
        depth1Row, depth1Seed, depth1Pid, 9, 0).matched());
    static_assert(!matchReroll(
        depth1Row, depth1Seed, depth1Pid, 9, 4).matched());

    // Depth 2: direct evidence and a one-retry history both map elsewhere.
    // The second rejected failed-Sync attempt reaches the requested row.
    constexpr uint32_t depth2Seed = 1857192u;
    constexpr uint32_t depth2Pid = sequentialPid(depth2Seed);
    constexpr uint64_t depth2Row = makeRow(8, 0, 7, 18, 100);
    static_assert(depth2Pid == 0x278F5C68u);
    static_assert(!Gen4LeadFailure::matchAttempt(
        true, depth2Row, depth2Seed, depth2Pid, 17,
        Gen4LeadFailure::Lead::Synchronize).matched());
    static_assert(!matchReroll(
        depth2Row, depth2Seed, depth2Pid, 17, 1).matched());
    constexpr auto depth2 = matchReroll(
        depth2Row, depth2Seed, depth2Pid, 17, 2);
    static_assert(depth2.matched());
    static_assert(depth2.slot == 0);
    static_assert(depth2.level == 17);
    static_assert(depth2.rerollDepth == 2);
    static_assert(depth2.encounterSeed == 0x2B6E1AFCu);

    // Depth 3: the persisted fourth attempt itself has no 31 IV. That is valid
    // only after all three earlier attempts were rejected for lacking a 31.
    constexpr uint32_t depth3Seed = 0xEEADB1AFu;
    constexpr uint32_t depth3Pid = sequentialPid(depth3Seed);
    constexpr uint64_t depth3Row = makeRow(8, 1, 7, 18, 100);
    static_assert(depth3Pid == 0x5A5D6872u);
    static_assert(!Gen4LeadFrame::directMinimum31Satisfied(depth3Seed));
    static_assert(!Gen4LeadFailure::matchAttempt(
        true, depth3Row, depth3Seed, depth3Pid, 16,
        Gen4LeadFailure::Lead::Synchronize).matched());
    static_assert(!matchReroll(
        depth3Row, depth3Seed, depth3Pid, 16, 1).matched());
    static_assert(!matchReroll(
        depth3Row, depth3Seed, depth3Pid, 16, 2).matched());
    constexpr auto depth3 = matchReroll(
        depth3Row, depth3Seed, depth3Pid, 16, 3);
    static_assert(depth3.matched());
    static_assert(depth3.slot == 1);
    static_assert(depth3.level == 16);
    static_assert(depth3.rerollDepth == 3);
    static_assert(depth3.encounterSeed == 0xE37F5F2Du);
    constexpr auto recovered = match(
        depth3Row, depth3Seed, depth3Pid, 16);
    static_assert(recovered.matched());
    static_assert(recovered.rerollDepth == 3);

    // Failed Synchronize cannot Sweet Scent in BCC, so activation remains
    // mandatory. Safari and non-BCC rows are outside this primitive.
    constexpr uint64_t zeroRate = makeRow(8, 0, 7, 18, 0);
    static_assert(!matchReroll(
        zeroRate, depth1Seed, depth1Pid, 9, 1).matched());
    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 100);
    static_assert(!matchReroll(
        safari, depth1Seed, depth1Pid, 15, 1).matched());

    assert(match(depth1Row, depth1Seed, depth1Pid, 9).matched());
    assert(match(depth2Row, depth2Seed, depth2Pid, 17).rerollDepth == 2);
    assert(match(depth3Row, depth3Seed, depth3Pid, 16).rerollDepth == 3);

    std::cout << "Gen IV BCC failed-Synchronize reroll evidence: PASS\n";
}
