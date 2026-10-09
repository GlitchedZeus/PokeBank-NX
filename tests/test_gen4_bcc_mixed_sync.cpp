#include "Legality/Gen4BugContestMixedSyncEvidence.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t makeRow(uint8_t type, uint8_t slot,
                           uint8_t min, uint8_t max, uint8_t rate) {
    return (static_cast<uint64_t>(min) << 17) |
           (static_cast<uint64_t>(max) << 24) |
           (static_cast<uint64_t>(type) << 31) |
           (static_cast<uint64_t>(slot) << 46) |
           (static_cast<uint64_t>(rate) << 50);
}
}

int main() {
    using namespace Legality;
    namespace Mixed = Gen4BugContestMixedSync;

    // Independently enumerated bounded Method K frame. First HG/SS Bug
    // Contest attempt: 0x880C24A4, successful Sync (even), PID 0x7C4C9B2D
    // nature 20, slot 3 and level 9, activation 0xC3DD8433 (roll 41 < 50).
    // Its IV words 20995 and 5252 have no 31, so the contest rerolls.
    // Persisted attempt: ordinary nature roll at 0x00000572 (nature 0) after
    // failed Sync proc 0xFA992D9B (odd), PID 0xC2EB29D7 nature 0,
    // and retained IVs contain a 31. Thus the natures differ as expected
    // when Synchronize fails on the second attempt.
    constexpr uint32_t finalSeed = 0x00000572u;
    constexpr uint32_t finalPid = 0xC2EB29D7u;
    constexpr uint64_t row = makeRow(8, 3, 7, 18, 50);
    static_assert(Gen4LeadFrame::sequentialPid(finalSeed) == finalPid);
    static_assert(Gen4LeadFrame::directMinimum31Satisfied(finalSeed));
    constexpr uint32_t failSeed = Gen3PidIv::Detail::prev(finalSeed);
    static_assert(failSeed == 0xFA992D9Bu);
    static_assert(Gen4BugContestSynchronizeFailure::synchronizeFails(failSeed));
    static_assert(Gen4LeadFrame::previousRerollAttemptRejected(failSeed));
    constexpr uint32_t firstSeed =
        Gen4LeadFrame::previousRerollNatureSeed(failSeed);
    static_assert(firstSeed == 0x880C24A4u);
    static_assert(Gen4LeadFrame::sequentialPid(firstSeed) == 0x7C4C9B2Du);
    static_assert(((firstSeed >> 16) & 1u) == 0u);

    constexpr auto matched =
        Mixed::matchSuccessThenFailure(row, finalSeed, finalPid, 9);
    static_assert(matched.matched());
    static_assert(matched.encounterSeed == 0xC3DD8433u);
    static_assert(matched.originPid == 0x7C4C9B2Du);
    static_assert(matched.originNature == 20u);
    static_assert(matched.slot == 3u);
    static_assert(matched.level == 9u);
    static_assert(matched.rerollDepth == 1u);
    static_assert((finalPid % 25u) == 0u);

    // Old all-failed proof cannot accept this frame, as the origin's proc
    // actually succeeded. The new mixed path is separate positive evidence.
    static_assert(!Gen4BugContestSynchronizeFailure::matchReroll(
        row, finalSeed, finalPid, 9, 1).matched());

    // Negative boundaries are Unresolved, NEVER proof of impossibility.
    static_assert(!Mixed::matchSuccessThenFailure(
        row, finalSeed, finalPid, 10).matched()); // wrong capture level
    static_assert(!Mixed::matchSuccessThenFailure(
        makeRow(8, 2, 7, 18, 50), finalSeed, finalPid, 9).matched());
    static_assert(!Mixed::matchSuccessThenFailure(
        makeRow(8, 3, 7, 18, 0), finalSeed, finalPid, 9).matched());
    static_assert(!Mixed::matchSuccessThenFailure(
        makeRow(10, 3, 7, 18, 50), finalSeed, finalPid, 9).matched());
    static_assert(!Mixed::matchSuccessThenFailure(
        row, finalSeed, finalPid ^ 1u, 9).matched());

    // Second independent success-then-failure frame, slot 2 at level 9.
    // Its original successful-Sync PID nature is 18; retained nature is 0.
    constexpr uint32_t secondSeed = 0x00000728u;
    constexpr uint32_t secondPid = 0xC51FB321u;
    constexpr auto second = Mixed::matchSuccessThenFailure(
        makeRow(8, 2, 7, 18, 50), secondSeed, secondPid, 9);
    static_assert(second.matched());
    static_assert(second.encounterSeed == 0x8FD9AF41u);
    static_assert(second.originPid == 0xF14C469Cu);
    static_assert(second.originNature == 18u);
    static_assert(second.rerollDepth == 1u);
    static_assert(!Mixed::matchSuccessThenFailure(
        makeRow(8, 2, 7, 18, 50), secondSeed, secondPid, 10).matched());

    assert(matched.matched() && second.matched());
    std::cout << "Gen IV BCC mixed Synchronize single reroll: PASS\n";
}
