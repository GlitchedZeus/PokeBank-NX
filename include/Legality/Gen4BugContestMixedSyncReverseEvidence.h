#pragma once

#include "Legality/Gen4BugContestSynchronizeEvidence.h"
#include "Legality/Gen4BugContestSynchronizeFailureEvidence.h"

#include <cstdint>

namespace Legality::Gen4BugContestMixedSyncReverse {

// Bounded positive-only Method K BCC history: the FIRST attempt failed its
// Synchronize proc and rolled its nature ordinarily; this PID/IV attempt was
// rejected (no IV=31). The SECOND/persisted attempt succeeds Synchronize,
// and its PID has the lead nature. Different first/final PID natures are
// permitted because the first proc failed.
//
// Pin: kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722,
// MethodK.TryGetMatch -> RecurseReject, including previous 31-IV checks
// and Synchronize lock state. This is one reroll with immediate PID acceptance,
// NOT all possible historical leads or full PKHeX equivalence. Unknown is
// always Incomplete / Unresolved, never hard Invalid.

struct Result {
    uint32_t encounterSeed = 0;
    uint32_t rejectedPid = 0;
    uint8_t rejectedNature = 0xFF;
    uint8_t slot = 0xFF;
    uint8_t level = 0;
    uint8_t rerollDepth = 0;
    constexpr bool matched() const noexcept { return slot != 0xFF; }
};

constexpr Result matchFailureThenSuccess(uint64_t row,
                                         uint32_t finalSyncSeed,
                                         uint32_t persistedPid,
                                         uint8_t metLevel) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)) ||
        Gen4LeadFrame::sequentialPid(finalSyncSeed) != persistedPid ||
        !Gen4BugContestSynchronize::synchronizePass(
            static_cast<uint16_t>(finalSyncSeed >> 16)) ||
        ((finalSyncSeed >> 16) % 25u) == (persistedPid % 25u) ||
        !Gen4LeadFrame::directMinimum31Satisfied(finalSyncSeed) ||
        !Gen4LeadFrame::previousRerollAttemptRejected(finalSyncSeed))
        return {};

    // Four previous PID+IV frames and one prior ordinary nature roll:
    // the rejected first attempt's nature is Prev5(final sync proc).
    const uint32_t rejectedNatureSeed =
        Gen4LeadFrame::previousRerollNatureSeed(finalSyncSeed);
    const uint32_t rejectedPid =
        Gen4LeadFrame::sequentialPid(rejectedNatureSeed);

    // The earlier Synchronize proc must FAIL; then an ordinary nature roll
    // must explain the rejected PID. The accepted helper proves exact slot,
    // level, and mandatory no-Sweet-Scent Bug Contest activation/rate.
    const auto original =
        Gen4BugContestSynchronizeFailure::matchImmediateAttempt(
            row, rejectedNatureSeed, rejectedPid, metLevel);
    if (!original.matched())
        return {};

    uint32_t activationSeed = rejectedNatureSeed;
    for (int i = 0; i < 4; ++i)
        activationSeed = Gen3PidIv::Detail::prev(activationSeed);

    return {activationSeed, rejectedPid,
            static_cast<uint8_t>(rejectedPid % 25u),
            original.slot, original.level, 1};
}

} // namespace Legality::Gen4BugContestMixedSyncReverse
