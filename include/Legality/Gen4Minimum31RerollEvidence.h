#pragma once

#include "Legality/Gen4LeadFrameEvidence.h"

#include <cstdint>

namespace Legality::Gen4Minimum31Reroll {

inline constexpr uint8_t kMaximumAttempts = 4;

struct PreviousAttempt {
    uint32_t origin = 0;
    uint16_t iv1 = 0;
    uint16_t iv2 = 0;
    bool rejectedForNo31 = false;
};

// Pinned EncounterSlot4.IsRerollMinimum31: HG/SS Bug Catching Contest and every
// Safari encounter family sample-reject PID/IV attempts until at least one IV is
// 31, stopping after at most four attempts.
constexpr bool requiresMinimum31(uint64_t row) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    return type == 8 || Gen4LeadFrame::isSafari(type);
}

constexpr bool hasAny31(uint16_t word) noexcept {
    return Gen4LeadFrame::hasAny31IvWord(word);
}

// `origin` is the Method K state immediately after the nature call and before
// the two PID calls. PID low/high consume Next1/Next2; IV1/IV2 consume
// Next3/Next4.
constexpr bool attemptHasAny31(uint32_t origin) noexcept {
    return Gen4LeadFrame::directMinimum31Satisfied(origin);
}

// Pinned MethodK.RecurseReject reverses from the next attempt's origin through
// IV2, IV1, then three more calls to the prior attempt origin. This helper only
// reconstructs that rejected-attempt boundary; it does NOT prove that the prior
// nature/slot/lead/activation history itself was reachable.
constexpr PreviousAttempt previousAttempt(uint32_t nextAttemptOrigin) noexcept {
    uint32_t state = Gen3PidIv::Detail::prev(nextAttemptOrigin);
    const uint16_t iv2 = static_cast<uint16_t>(state >> 16);
    state = Gen3PidIv::Detail::prev(state);
    const uint16_t iv1 = static_cast<uint16_t>(state >> 16);
    state = Gen3PidIv::Detail::prev(
        Gen3PidIv::Detail::prev(Gen3PidIv::Detail::prev(state)));
    return {state, iv1, iv2, !hasAny31(iv1) && !hasAny31(iv2)};
}

// Test/data-audit utility: advance one complete ordinary Method K attempt from
// nature/origin through PID1, PID2, IV1, IV2, then the next nature/origin call.
// Production legality must still validate encounter/lead semantics separately.
constexpr uint32_t nextAttemptOrigin(uint32_t origin) noexcept {
    for (int i = 0; i < 5; ++i)
        origin = Gen3PidIv::Detail::next(origin);
    return origin;
}

} // namespace Legality::Gen4Minimum31Reroll
