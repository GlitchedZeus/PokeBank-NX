#pragma once
#include "Legality/Gen4BugContestSynchronizeEvidence.h"
#include "Legality/Gen4BugContestSynchronizeFailureEvidence.h"
#include <cstdint>

namespace Legality::Gen4BugContestMixedDepth {

// Positive-only Method K / HGSS Bug Contest mixed Synchronize histories.
// At most three rejected minimum-31 attempts. Bit i describes whether
// Synchronize succeeded on chronological attempt i, with i=0 the first
// attempt and i=depth the retained attempt. Reject unknown/PID nature
// rejection loops as unresolved, never hard Invalid.
// Pinned PKHeX: 6501f0ab46e8f8ca048539dbaf8cae8cb104e722,
// ClassicEra/Gen4/MethodK.cs::TryGetMatch and RecurseReject.
struct Result {
    uint32_t encounterSeed = 0;
    uint32_t originalPid = 0;
    uint8_t lockedNature = 0xFF;
    uint8_t slot = 0xFF;
    uint8_t level = 0;
    uint8_t depth = 0;
    uint8_t syncSuccessMask = 0;
    constexpr bool matched() const noexcept { return slot != 0xFF; }
};

constexpr Result match(uint64_t row, uint32_t finalSeed, uint32_t pid,
                       uint8_t level, uint8_t depth, uint8_t mask) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)) ||
        (depth != 2 && depth != 3) ||
        mask == 0 || mask >= (1u << (depth + 1u)) ||
        mask == ((1u << (depth + 1u)) - 1u) ||
        Gen4LeadFrame::sequentialPid(finalSeed) != pid ||
        (!Gen4LeadFrame::directMinimum31Satisfied(finalSeed) && depth != 3))
        return {};

    uint32_t s = finalSeed;
    uint8_t leadNature = 0xFF;
    for (int i = static_cast<int>(depth); i >= 0; --i) {
        const bool success = (mask & (1u << i)) != 0;
        const uint32_t attemptPid = Gen4LeadFrame::sequentialPid(s);
        const uint8_t nature = static_cast<uint8_t>(attemptPid % 25u);
        if (success) {
            if (!Gen4BugContestSynchronize::synchronizePass(
                    static_cast<uint16_t>(s >> 16)) ||
                (leadNature != 0xFF && nature != leadNature))
                return {};
            leadNature = nature;
        } else {
            if (!Gen4BugContestSynchronizeFailure::synchronizeFails(
                    Gen3PidIv::Detail::prev(s)) ||
                ((s >> 16) % 25u) != nature)
                return {};
        }
        if (i == 0) {
            if (leadNature == 0xFF)
                return {};
            if (success) {
                const auto origin =
                    Gen4BugContestSynchronize::matchSuccessfulAttempt(
                        row, s, attemptPid, level, leadNature);
                if (!origin.matched())
                    return {};
                return {origin.encounterSeed, attemptPid, leadNature,
                        origin.slot, origin.level, depth, mask};
            }
            const auto origin =
                Gen4BugContestSynchronizeFailure::matchImmediateAttempt(
                    row, s, attemptPid, level);
            if (!origin.matched())
                return {};
            return {origin.encounterSeed, attemptPid, leadNature,
                    origin.slot, origin.level, depth, mask};
        }
        // Failed-Sync attempts consume an additional proc before nature.
        // Previous two IV words must lack 31; then skip exactly the previous
        // attempt's PID/IV frames, preserving RNG frame provenance.
        const uint32_t trigger = success ? s : Gen3PidIv::Detail::prev(s);
        if (!Gen4LeadFrame::previousRerollAttemptRejected(trigger))
            return {};
        s = Gen4LeadFrame::previousRerollNatureSeed(trigger);
    }
    return {};
}
} // namespace Legality::Gen4BugContestMixedDepth
