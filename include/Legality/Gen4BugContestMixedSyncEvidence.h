#pragma once

#include "Legality/Gen4BugContestSynchronizeEvidence.h"
#include "Legality/Gen4BugContestSynchronizeFailureEvidence.h"

#include <cstdint>

namespace Legality::Gen4BugContestMixedSync {

// Narrow positive reconstruction of one HG/SS BCC minimum-31 retry:
// first attempt succeeds Synchronize (fixed lead nature N), lacks any 31 IV,
// and is rejected; the second/persisted attempt FAILS Synchronize and therefore
// accepts an independently rolled nature. This means the two PID natures may
// differ without changing the active lead. There is no inference of lead ID
// from persistent PK4 data and no hard-Invalid classification on mismatch.
//
// Pinned reference: kwsch/PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
// PKHeX.Core/Legality/RNG/ClassicEra/Gen4/MethodK.cs
// TryGetMatch -> failed-Sync recursion -> RecurseReject -> Sync origin,
// with previous rejected PID/IV and nature/sync frame accounting.
// This intentionally omits 2/3 rerolls, consecutive PID nature-rejections,
// and other unknown lead sequences.

struct Result {
    uint32_t encounterSeed = 0;
    uint32_t originPid = 0;
    uint8_t originNature = 0xFF;
    uint8_t slot = 0xFF;
    uint8_t level = 0;
    uint8_t rerollDepth = 0;

    constexpr bool matched() const noexcept { return slot != 0xFF; }
};

constexpr Result matchSuccessThenFailure(uint64_t row,
                                         uint32_t finalNatureSeed,
                                         uint32_t finalPid,
                                         uint8_t metLevel) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)) ||
        Gen4LeadFrame::sequentialPid(finalNatureSeed) != finalPid ||
        ((finalNatureSeed >> 16) % 25u) != (finalPid % 25u) ||
        !Gen4LeadFrame::directMinimum31Satisfied(finalNatureSeed))
        return {};

    // The retained attempt uses an ordinary nature roll after a failed
    // Synchronize proc. The proc immediately precedes that nature roll.
    const uint32_t failedSyncSeed = Gen3PidIv::Detail::prev(finalNatureSeed);
    if (!Gen4BugContestSynchronizeFailure::synchronizeFails(failedSyncSeed) ||
        !Gen4LeadFrame::previousRerollAttemptRejected(failedSyncSeed))
        return {};

    // Prior generated attempt: old Sync proc, PID-low, PID-high, IV1, IV2,
    // then failedSyncSeed, final ordinary nature, final PID/IV.
    const uint32_t successfulSyncSeed =
        Gen4LeadFrame::previousRerollNatureSeed(failedSyncSeed);
    const uint32_t firstPid =
        Gen4LeadFrame::sequentialPid(successfulSyncSeed);
    const uint8_t firstNature = static_cast<uint8_t>(firstPid % 25u);
    const auto original =
        Gen4BugContestSynchronize::matchSuccessfulAttempt(
            row, successfulSyncSeed, firstPid, metLevel, firstNature);
    if (!original.matched())
        return {};

    return {original.encounterSeed, firstPid, firstNature,
            original.slot, original.level, 1};
}

} // namespace Legality::Gen4BugContestMixedSync
