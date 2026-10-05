#pragma once

#include "Legality/Gen4LeadFrameEvidence.h"
#include "Legality/Gen4WildEncounter.h"

#include <cstdint>

namespace Legality::Gen4BugContestSynchronize {

struct Result {
    uint32_t encounterSeed = 0;
    uint8_t slot = 0xFF;
    uint8_t level = 0;
    uint8_t lockedNature = 0xFF;
    uint8_t rerollDepth = 0;

    constexpr bool matched() const noexcept { return slot != 0xFF; }
};

constexpr bool synchronizePass(uint16_t rand16) noexcept {
    return (rand16 & 1u) == 0u;
}

constexpr bool activationAllows(uint64_t row,
                                uint32_t activationSeed) noexcept {
    return Gen4LeadFrame::bugContestActivationAllows(
        Gen4Wild::rate(row), activationSeed, false);
}

// Proves one successful Method K Synchronize BCC attempt for a known lead
// nature. Successful Synchronize consumes the current RNG call as the sync proc;
// level and slot are immediately before it and, because Synchronize cannot use
// Sweet Scent during BCC, the preceding movement/rate activation must pass.
//
// This primitive deliberately models only immediate PID acceptance after the
// sync proc. PID-nature rejection loops remain unresolved rather than guessed.
constexpr Result matchSuccessfulAttempt(uint64_t row,
                                        uint32_t syncSeed,
                                        uint32_t pid,
                                        uint8_t metLevel,
                                        uint8_t lockedNature) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)) ||
        lockedNature >= 25 ||
        (pid % 25u) != lockedNature ||
        !synchronizePass(static_cast<uint16_t>(syncSeed >> 16)) ||
        Gen4LeadFrame::sequentialPid(syncSeed) != pid)
        return {};

    const uint32_t seed1 = Gen3PidIv::Detail::prev(syncSeed);
    const uint16_t levelRand = static_cast<uint16_t>(seed1 >> 16);
    const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
    const uint16_t slotRand = static_cast<uint16_t>(seed2 >> 16);
    const uint32_t activationSeed = Gen3PidIv::Detail::prev(seed2);

    const uint8_t slot = Gen4LeadFrame::bugContestSlot(slotRand);
    const uint8_t level = Gen4LeadFrame::randomLevel(
        Gen4Wild::minLevel(row), Gen4Wild::maxLevel(row), levelRand);
    if (slot != Gen4Wild::slot(row) || level != metLevel ||
        !activationAllows(row, activationSeed))
        return {};

    return {activationSeed, slot, level, lockedNature, 0};
}

// Conservative positive subset of pinned Method K RecurseReject for successful
// Synchronize. The persisted target proves Synchronize was required by having a
// different ordinary nature roll from its PID nature. Every retry is then
// required to be another successful Synchronize proc with the same locked lead
// nature and immediate PID acceptance. Mixed success/failure Sync chains and PID
// rejection loops are intentionally left unresolved for later dedicated work.
constexpr Result matchReroll(uint64_t row,
                             uint32_t syncSeed,
                             uint32_t pid,
                             uint8_t metLevel,
                             uint8_t depth) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)) ||
        depth == 0 || depth > 3 ||
        !Gen4LeadFrame::minimum31IvChainAllows(syncSeed, depth))
        return {};

    const uint8_t lockedNature = static_cast<uint8_t>(pid % 25u);
    const uint16_t finalRand = static_cast<uint16_t>(syncSeed >> 16);
    if (!synchronizePass(finalRand) ||
        (finalRand % 25u) == lockedNature ||
        Gen4LeadFrame::sequentialPid(syncSeed) != pid)
        return {};

    uint32_t attemptSeed = syncSeed;
    for (uint8_t i = 0; i < depth; ++i) {
        const uint32_t attemptPid = Gen4LeadFrame::sequentialPid(attemptSeed);
        if ((attemptPid % 25u) != lockedNature ||
            !synchronizePass(static_cast<uint16_t>(attemptSeed >> 16)) ||
            !Gen4LeadFrame::previousRerollAttemptRejected(attemptSeed))
            return {};
        attemptSeed = Gen4LeadFrame::previousRerollNatureSeed(attemptSeed);
    }

    const uint32_t originPid = Gen4LeadFrame::sequentialPid(attemptSeed);
    Result origin = matchSuccessfulAttempt(
        row, attemptSeed, originPid, metLevel, lockedNature);
    if (!origin.matched())
        return {};
    origin.rerollDepth = depth;
    return origin;
}

constexpr Result match(uint64_t row,
                       uint32_t syncSeed,
                       uint32_t pid,
                       uint8_t metLevel) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)))
        return {};

    const uint8_t lockedNature = static_cast<uint8_t>(pid % 25u);
    const uint16_t rand16 = static_cast<uint16_t>(syncSeed >> 16);
    if (Gen4LeadFrame::directMinimum31Satisfied(syncSeed) &&
        synchronizePass(rand16) &&
        (rand16 % 25u) != lockedNature) {
        const Result direct = matchSuccessfulAttempt(
            row, syncSeed, pid, metLevel, lockedNature);
        if (direct.matched())
            return direct;
    }

    for (uint8_t depth = 1; depth <= 3; ++depth) {
        const Result rerolled = matchReroll(
            row, syncSeed, pid, metLevel, depth);
        if (rerolled.matched())
            return rerolled;
    }
    return {};
}

} // namespace Legality::Gen4BugContestSynchronize
