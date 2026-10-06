#pragma once

#include "Legality/Gen4LeadFailureEvidence.h"
#include "Legality/Gen4LeadFrameEvidence.h"
#include "Legality/Gen4WildEncounter.h"

#include <cstdint>

namespace Legality::Gen4BugContestSynchronizeFailure {

struct Result {
    uint32_t encounterSeed = 0;
    uint8_t slot = 0xFF;
    uint8_t level = 0;
    uint8_t rerollDepth = 0;

    constexpr bool matched() const noexcept { return slot != 0xFF; }
};

constexpr bool synchronizeFails(uint32_t syncSeed) noexcept {
    return Gen4LeadEffect::synchronizeFail(
        Gen4LeadEffect::Method::K,
        static_cast<uint16_t>(syncSeed >> 16));
}

// A failed Method K Synchronize attempt consumes two lead/nature calls before
// PID generation: the Synchronize proc fails, then the ordinary nature roll is
// accepted. This is deliberately narrower than the full pinned PKHeX recursion:
// PID-nature rejection loops are not modeled here.
constexpr Result matchImmediateAttempt(uint64_t row,
                                       uint32_t natureSeed,
                                       uint32_t pid,
                                       uint8_t metLevel) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)) ||
        Gen4LeadFrame::sequentialPid(natureSeed) != pid ||
        ((natureSeed >> 16) % 25u) != (pid % 25u))
        return {};

    const uint32_t syncSeed = Gen3PidIv::Detail::prev(natureSeed);
    if (!synchronizeFails(syncSeed))
        return {};

    const auto failure = Gen4LeadFailure::matchPostNatureFailure(
        true, row, natureSeed, metLevel,
        Gen4LeadFailure::Lead::Synchronize);
    if (!failure.matched())
        return {};

    return {failure.encounterSeed, failure.slot, metLevel, 0};
}

// For a failed-Synchronize retry, the current attempt's nature seed is preceded
// by its failed Sync proc. The previous rejected attempt's IV2/IV1/PID/PID then
// sit behind that proc. Therefore the prior attempt's nature seed is six Prev
// calls behind the current nature seed (one to the Sync proc + the five frames
// already modeled by previousRerollNatureSeed).
constexpr uint32_t previousFailedSyncNatureSeed(
        uint32_t currentNatureSeed) noexcept {
    const uint32_t syncSeed = Gen3PidIv::Detail::prev(currentNatureSeed);
    return Gen4LeadFrame::previousRerollNatureSeed(syncSeed);
}

constexpr bool previousAttemptRejected(
        uint32_t currentNatureSeed) noexcept {
    const uint32_t syncSeed = Gen3PidIv::Detail::prev(currentNatureSeed);
    return Gen4LeadFrame::previousRerollAttemptRejected(syncSeed);
}

// Positive-only all-failed-Synchronize subset of pinned MethodK::RecurseReject.
// Every modeled retry must fail Synchronize and then immediately accept its
// ordinary nature roll. Mixed failed/successful Sync chains, nature-lock
// transitions, and PID nature-rejection loops remain unresolved.
constexpr Result matchReroll(uint64_t row,
                             uint32_t finalNatureSeed,
                             uint32_t pid,
                             uint8_t metLevel,
                             uint8_t depth) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)) ||
        depth == 0 || depth > 3 ||
        (!Gen4LeadFrame::directMinimum31Satisfied(finalNatureSeed) &&
         depth != 3))
        return {};

    uint32_t attemptSeed = finalNatureSeed;
    uint32_t attemptPid = pid;
    for (uint8_t i = 0; i < depth; ++i) {
        if (Gen4LeadFrame::sequentialPid(attemptSeed) != attemptPid ||
            ((attemptSeed >> 16) % 25u) != (attemptPid % 25u))
            return {};

        const uint32_t syncSeed = Gen3PidIv::Detail::prev(attemptSeed);
        if (!synchronizeFails(syncSeed) ||
            !Gen4LeadFrame::previousRerollAttemptRejected(syncSeed))
            return {};

        attemptSeed = Gen4LeadFrame::previousRerollNatureSeed(syncSeed);
        attemptPid = Gen4LeadFrame::sequentialPid(attemptSeed);
    }

    Result origin = matchImmediateAttempt(
        row, attemptSeed, attemptPid, metLevel);
    if (!origin.matched())
        return {};
    origin.rerollDepth = depth;
    return origin;
}

constexpr Result match(uint64_t row,
                       uint32_t finalNatureSeed,
                       uint32_t pid,
                       uint8_t metLevel) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)))
        return {};

    for (uint8_t depth = 1; depth <= 3; ++depth) {
        const Result result = matchReroll(
            row, finalNatureSeed, pid, metLevel, depth);
        if (result.matched())
            return result;
    }
    return {};
}

} // namespace Legality::Gen4BugContestSynchronizeFailure
