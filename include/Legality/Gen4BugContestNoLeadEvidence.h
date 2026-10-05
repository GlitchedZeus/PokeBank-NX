#pragma once

#include "Legality/Gen4LeadFrameEvidence.h"
#include "Legality/Gen4WildEncounter.h"

#include <cstdint>

namespace Legality::Gen4BugContestNoLead {

struct Result {
    uint32_t encounterSeed = 0;
    uint8_t slot = 0xFF;
    uint8_t level = 0;
    uint8_t rerollDepth = 0;

    constexpr bool matched() const noexcept { return slot != 0xFF; }
};

// Proves one complete HG/SS Bug Catching Contest generation origin without a
// lead requirement. No-lead can trigger BCC through Sweet Scent, so unlike
// Synchronize/Cute Charm/Static/Magnet Pull there is no movement-rate deadlock
// frame to prove before slot selection. This helper intentionally does not
// enforce the minimum-31 gate; an earlier rejected origin is checked by the
// reroll wrapper below.
constexpr Result matchAttempt(uint64_t row,
                              uint32_t prePidSeed,
                              uint32_t pid,
                              uint8_t metLevel) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)))
        return {};

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    const int frames = Gen4LeadFrame::reversalWindow(prePidSeed, nature);
    if (frames < 0)
        return {};

    uint32_t candidate = prePidSeed;
    for (int i = 0; i <= frames; ++i) {
        const uint16_t natureRand = static_cast<uint16_t>(candidate >> 16);
        if ((natureRand % 25u) == nature) {
            const uint32_t seed1 = Gen3PidIv::Detail::prev(candidate);
            const uint16_t levelRand = static_cast<uint16_t>(seed1 >> 16);
            const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
            const uint16_t slotRand = static_cast<uint16_t>(seed2 >> 16);

            const uint8_t slot = Gen4LeadFrame::bugContestSlot(slotRand);
            const uint8_t level = Gen4LeadFrame::randomLevel(
                Gen4Wild::minLevel(row), Gen4Wild::maxLevel(row), levelRand);
            if (slot == Gen4Wild::slot(row) && level == metLevel)
                return {candidate, slot, level, 0};
        }

        candidate = Gen3PidIv::Detail::prev(
            Gen3PidIv::Detail::prev(candidate));
    }
    return {};
}

// Mirrors the non-Synchronize Method K RecurseReject path. Each later attempt
// must land on its generated PID nature and must be preceded by a minimum-31
// rejection. After the requested number of rejected attempts, the earliest
// generation attempt must prove the source BCC slot and level. Rejected PID/IV
// words are never reinterpreted as fresh encounter slot/level frames.
constexpr Result matchReroll(uint64_t row,
                             uint32_t prePidSeed,
                             uint32_t pid,
                             uint8_t metLevel,
                             uint8_t depth) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)) ||
        depth == 0 || depth > 3 ||
        !Gen4LeadFrame::minimum31IvChainAllows(prePidSeed, depth))
        return {};

    uint32_t attemptSeed = prePidSeed;
    uint32_t attemptPid = pid;
    for (uint8_t i = 0; i < depth; ++i) {
        const uint32_t generatedPid = Gen4LeadFrame::sequentialPid(attemptSeed);
        if (generatedPid != attemptPid ||
            ((attemptSeed >> 16) % 25u) != (attemptPid % 25u) ||
            !Gen4LeadFrame::previousRerollAttemptRejected(attemptSeed))
            return {};

        attemptSeed = Gen4LeadFrame::previousRerollNatureSeed(attemptSeed);
        attemptPid = Gen4LeadFrame::sequentialPid(attemptSeed);
    }

    Result origin = matchAttempt(row, attemptSeed, attemptPid, metLevel);
    if (!origin.matched())
        return {};
    origin.rerollDepth = depth;
    return origin;
}

constexpr Result match(uint64_t row,
                       uint32_t prePidSeed,
                       uint32_t pid,
                       uint8_t metLevel) noexcept {
    if (!Gen4LeadFrame::isBugContest(Gen4Wild::method(row)))
        return {};

    if (Gen4LeadFrame::directMinimum31Satisfied(prePidSeed)) {
        const Result direct = matchAttempt(row, prePidSeed, pid, metLevel);
        if (direct.matched())
            return direct;
    }

    for (uint8_t depth = 1; depth <= 3; ++depth) {
        const Result rerolled = matchReroll(
            row, prePidSeed, pid, metLevel, depth);
        if (rerolled.matched())
            return rerolled;
    }
    return {};
}

} // namespace Legality::Gen4BugContestNoLead
