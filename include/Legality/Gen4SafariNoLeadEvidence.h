#pragma once

#include "Legality/Gen4LeadFrameEvidence.h"
#include "Legality/Gen4WildEncounter.h"

#include <cstdint>

namespace Legality::Gen4SafariNoLead {

struct Result {
    uint32_t encounterSeed = 0;
    uint8_t slot = 0xFF;
    uint8_t rerollDepth = 0;
    bool suctionCups = false;

    constexpr bool matched() const noexcept { return slot != 0xFF; }
};

// Proves one complete HG/SS Safari no-lead attempt. Safari slots use rand%10
// and Safari levels are fixed in Method K. Fishing Safari additionally checks
// the rod activation frame immediately before slot selection; Old Rod may need
// the no-lead Suction Cups / Sticky Hold fallback after the +50 following-Pokemon
// rate bonus.
//
// This primitive intentionally does not enforce the minimum-31 gate so it can
// validate the original failed attempt reached by the reroll wrapper below.
constexpr Result matchAttempt(uint64_t row,
                              uint32_t prePidSeed,
                              uint32_t pid,
                              uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (!Gen4LeadFrame::isSafari(type) ||
        !Gen4Wild::levelMatches(row, metLevel))
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
            const uint16_t slotRand = static_cast<uint16_t>(seed1 >> 16);
            const uint8_t slot = Gen4LeadFrame::safariSlot(slotRand);
            if (slot == Gen4Wild::slot(row)) {
                bool suction = false;
                if (Gen4LeadFrame::isSafariFishing(type)) {
                    const uint32_t activationSeed =
                        Gen3PidIv::Detail::prev(seed1);
                    const auto activation = Gen4LeadFrame::fishingActivationKind(
                        true, type,
                        static_cast<uint16_t>(activationSeed >> 16));
                    if (activation == Gen4LeadFrame::Activation::None) {
                        candidate = Gen3PidIv::Detail::prev(
                            Gen3PidIv::Detail::prev(candidate));
                        continue;
                    }
                    suction = activation == Gen4LeadFrame::Activation::SuctionCups;
                }
                return {candidate, slot, 0, suction};
            }
        }

        candidate = Gen3PidIv::Detail::prev(
            Gen3PidIv::Detail::prev(candidate));
    }
    return {};
}

// Positive-only minimum-31 retry reconstruction for Safari. Every persisted
// retry must land on its generated Method K nature and be preceded by a rejected
// no-31 attempt. After the requested depth, the original Safari slot / fixed
// level / optional fishing activation path must be proven in full.
constexpr Result matchReroll(uint64_t row,
                             uint32_t prePidSeed,
                             uint32_t pid,
                             uint8_t metLevel,
                             uint8_t depth) noexcept {
    if (!Gen4LeadFrame::isSafari(Gen4Wild::method(row)) ||
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
    if (!Gen4LeadFrame::isSafari(Gen4Wild::method(row)))
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

} // namespace Legality::Gen4SafariNoLead
