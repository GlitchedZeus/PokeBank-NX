#pragma once

#include "Legality/Gen4LeadFailureEvidence.h"

#include <cstdint>

namespace Legality::Gen4StaticMagnetLead {

enum class Lead : uint8_t {
    None,
    Static,
    MagnetPull,
};

struct Result {
    Lead lead = Lead::None;
    uint32_t encounterSeed = 0;
    uint8_t rerollDepth = 0;

    constexpr bool matched() const noexcept { return lead != Lead::None; }
};

constexpr uint8_t magnetIndex(uint32_t meta) noexcept {
    return static_cast<uint8_t>(meta & 0xFFu);
}
constexpr uint8_t magnetCount(uint32_t meta) noexcept {
    return static_cast<uint8_t>((meta >> 8) & 0xFFu);
}
constexpr uint8_t staticIndex(uint32_t meta) noexcept {
    return static_cast<uint8_t>((meta >> 16) & 0xFFu);
}
constexpr uint8_t staticCount(uint32_t meta) noexcept {
    return static_cast<uint8_t>((meta >> 24) & 0xFFu);
}

constexpr Lead attractedLead(uint32_t meta, uint16_t rand16) noexcept {
    const uint8_t sc = staticCount(meta);
    if (sc != 0 && (rand16 % sc) == staticIndex(meta))
        return Lead::Static;

    const uint8_t mc = magnetCount(meta);
    if (mc != 0 && (rand16 % mc) == magnetIndex(meta))
        return Lead::MagnetPull;
    return Lead::None;
}

// Proves one complete Static/Magnet Pull origin attempt. For HG/SS minimum-31
// encounters this deliberately does not enforce the persisted-attempt IV gate;
// an earlier rejected origin has no 31 IV by definition and is validated by the
// reroll wrapper below.
constexpr Result matchAttempt(bool hgss, uint64_t row, uint32_t leadMeta,
                              uint32_t prePidSeed, uint32_t pid,
                              uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (!Gen4LeadFailure::supportedType(hgss, type))
        return {};

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    const int frames = Gen4LeadFrame::reversalWindow(prePidSeed, nature);
    if (frames < 0)
        return {};

    uint32_t candidate = prePidSeed;
    for (int i = 0; i <= frames; ++i) {
        const uint16_t natureRand = static_cast<uint16_t>(candidate >> 16);
        const uint32_t natureRoll = hgss
            ? (natureRand % 25u)
            : (natureRand / 0x0A3Eu);

        if (natureRoll == nature) {
            const uint32_t seed1 = Gen3PidIv::Detail::prev(candidate);
            const uint16_t prev1 = static_cast<uint16_t>(seed1 >> 16);
            const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
            const uint16_t prev2 = static_cast<uint16_t>(seed2 >> 16);

            if (Gen4LeadFailure::levelIsRandom(hgss, type)) {
                const uint32_t seed3 = Gen3PidIv::Detail::prev(seed2);
                const uint16_t prev3 = static_cast<uint16_t>(seed3 >> 16);

                if (!Gen4LeadEffect::staticMagnetPass(
                        Gen4LeadFailure::leadMethod(hgss), prev3) ||
                    Gen4LeadFailure::rolledLevel(row, prev1) != metLevel) {
                    candidate = Gen3PidIv::Detail::prev(
                        Gen3PidIv::Detail::prev(candidate));
                    continue;
                }

                const Lead lead = attractedLead(leadMeta, prev2);
                if (lead != Lead::None) {
                    const uint32_t activationSeed =
                        Gen3PidIv::Detail::prev(seed3);
                    if (Gen4LeadFailure::normalActivationAllows(
                            hgss, row, activationSeed, false))
                        return {lead, candidate, 0};
                }
            } else {
                if (!Gen4Wild::levelMatches(row, metLevel)) {
                    candidate = Gen3PidIv::Detail::prev(
                        Gen3PidIv::Detail::prev(candidate));
                    continue;
                }

                uint16_t procRand = prev2;
                if (!hgss) {
                    const uint32_t seed3 = Gen3PidIv::Detail::prev(seed2);
                    procRand = static_cast<uint16_t>(seed3 >> 16);
                }

                if (Gen4LeadEffect::staticMagnetPass(
                        Gen4LeadFailure::leadMethod(hgss), procRand)) {
                    const Lead lead = attractedLead(leadMeta, prev1);
                    if (lead != Lead::None)
                        return {lead, candidate, 0};
                }
            }
        }

        candidate = Gen3PidIv::Detail::prev(
            Gen3PidIv::Detail::prev(candidate));
    }
    return {};
}

// Mirrors pinned Method K's non-Synchronize RecurseReject path for HG/SS Bug
// Catching Contest. Slot attraction / lead proc / level / movement-rate proof is
// performed once at the pre-reroll origin. Every later generated attempt proves
// only its Method K nature roll and the immediately preceding minimum-31
// rejection. This prevents rejected PID/IV words from being misread as fresh
// Static/Magnet Pull encounter frames.
constexpr Result matchBugContestReroll(bool hgss, uint64_t row,
                                       uint32_t leadMeta,
                                       uint32_t prePidSeed, uint32_t pid,
                                       uint8_t metLevel,
                                       uint8_t depth) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (!hgss || !Gen4LeadFrame::isBugContest(type) ||
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

    Result origin = matchAttempt(
        hgss, row, leadMeta, attemptSeed, attemptPid, metLevel);
    if (!origin.matched())
        return {};
    origin.rerollDepth = depth;
    return origin;
}

constexpr Result matchRow(bool hgss, uint64_t row, uint32_t leadMeta,
                          uint32_t prePidSeed, uint32_t pid,
                          uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (!Gen4LeadFailure::supportedType(hgss, type))
        return {};

    if (!Gen4LeadFrame::isBugContest(type))
        return matchAttempt(hgss, row, leadMeta, prePidSeed, pid, metLevel);

    if (Gen4LeadFrame::directMinimum31Satisfied(prePidSeed)) {
        const Result direct = matchAttempt(
            hgss, row, leadMeta, prePidSeed, pid, metLevel);
        if (direct.matched())
            return direct;
    }

    for (uint8_t depth = 1; depth <= 3; ++depth) {
        const Result rerolled = matchBugContestReroll(
            hgss, row, leadMeta, prePidSeed, pid, metLevel, depth);
        if (rerolled.matched())
            return rerolled;
    }
    return {};
}

constexpr const char* leadName(Lead lead) noexcept {
    switch (lead) {
        case Lead::Static: return "Static";
        case Lead::MagnetPull: return "Magnet Pull";
        case Lead::None: break;
    }
    return "No Static/Magnet Pull evidence";
}

} // namespace Legality::Gen4StaticMagnetLead
