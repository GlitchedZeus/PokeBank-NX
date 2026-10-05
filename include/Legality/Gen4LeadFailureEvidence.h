#pragma once

#include "Legality/Gen4LeadEffectEvidence.h"
#include "Legality/Gen4LeadFrameEvidence.h"

#include <cstdint>

namespace Legality::Gen4LeadFailure {

enum class Lead : uint8_t {
    None,
    Synchronize,
    CuteCharm,
    PressureHustleVitalSpirit,
    StaticMagnetPull,
    IntimidateKeenEye,
};

struct Result {
    Lead lead = Lead::None;
    uint32_t encounterSeed = 0;
    uint8_t slot = 0;
    uint8_t rerollDepth = 0;

    constexpr bool matched() const noexcept { return lead != Lead::None; }
};

constexpr Gen4LeadEffect::Method leadMethod(bool hgss) noexcept {
    return hgss ? Gen4LeadEffect::Method::K : Gen4LeadEffect::Method::J;
}

constexpr bool postNatureFailureAllows(bool hgss, Lead lead,
                                       uint16_t rand16) noexcept {
    const auto method = leadMethod(hgss);
    switch (lead) {
        case Lead::Synchronize:
            return Gen4LeadEffect::synchronizeFail(method, rand16);
        case Lead::CuteCharm:
            return Gen4LeadEffect::cuteCharmFail(method, rand16);
        case Lead::PressureHustleVitalSpirit:
            return Gen4LeadEffect::pressureHustleVitalSpiritFail(method, rand16);
        case Lead::IntimidateKeenEye:
            return Gen4LeadEffect::intimidateKeenEyeEncounterContinues(method, rand16);
        case Lead::StaticMagnetPull:
        case Lead::None:
            break;
    }
    return false;
}

constexpr bool staticMagnetFailureAllows(bool hgss,
                                         uint16_t rand16) noexcept {
    return Gen4LeadEffect::staticMagnetFail(leadMethod(hgss), rand16);
}

constexpr bool supportedType(bool hgss, uint8_t type) noexcept {
    if (Gen4LeadFrame::isSafari(type))
        return false;
    if (Gen4LeadFrame::isBugContest(type))
        return hgss;
    if (type > 9)
        return false;
    if ((type == 5 || Gen4LeadFrame::isHeadbutt(type)) && !hgss)
        return false;
    if (Gen4LeadFrame::isHoneyTree(type) && hgss)
        return false;
    return true;
}

constexpr bool levelIsRandom(bool hgss, uint8_t type) noexcept {
    if (type == 0)
        return false;
    if (hgss && Gen4LeadFrame::isSafari(type))
        return false;
    return true;
}

constexpr uint8_t rolledSlot(bool hgss, uint64_t row,
                             uint16_t rand16) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (type == 0 || type == 1)
        return hgss
            ? Gen4LeadFrame::methodKSlot(type, rand16)
            : Gen4LeadFrame::methodJSlot(type, rand16);
    if (Gen4LeadFrame::isFishing(type))
        return Gen4LeadFrame::fishingSlot(hgss, type, rand16);
    if (type == 5)
        return Gen4LeadFrame::rockSmashSlot(rand16);
    if (Gen4LeadFrame::isHeadbutt(type))
        return Gen4LeadFrame::headbuttSlot(rand16);
    if (Gen4LeadFrame::isBugContest(type))
        return hgss ? Gen4LeadFrame::bugContestSlot(rand16) : 0xFF;
    if (Gen4LeadFrame::isHoneyTree(type))
        return Gen4Wild::slot(row);
    return 0xFF;
}

constexpr uint8_t rolledLevel(uint64_t row, uint16_t rand16) noexcept {
    return Gen4LeadFrame::isHoneyTree(Gen4Wild::method(row))
        ? Gen4LeadFrame::honeyTreeLevel(rand16)
        : Gen4LeadFrame::randomLevel(
            Gen4Wild::minLevel(row), Gen4Wild::maxLevel(row), rand16);
}

constexpr bool failureCanSweetScent(Lead lead) noexcept {
    return lead == Lead::PressureHustleVitalSpirit ||
           lead == Lead::IntimidateKeenEye;
}

constexpr bool normalActivationAllows(bool hgss, uint64_t row,
                                      uint32_t activationSeed,
                                      bool bugContestCanSweetScent = true) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (type == 0 || type == 1 || Gen4LeadFrame::isHeadbutt(type) ||
        Gen4LeadFrame::isHoneyTree(type))
        return true;

    if (Gen4LeadFrame::isBugContest(type)) {
        return hgss && Gen4LeadFrame::bugContestActivationAllows(
            Gen4Wild::rate(row), activationSeed, bugContestCanSweetScent);
    }

    if (type == 5) {
        return Gen4LeadFrame::rockSmashActivationKind(
                   Gen4Wild::rate(row),
                   static_cast<uint16_t>(activationSeed >> 16)) ==
               Gen4LeadFrame::Activation::Normal;
    }

    if (Gen4LeadFrame::isFishing(type)) {
        if (!hgss && Gen4Wild::rate(row) == 0xFFu) {
            const uint16_t tileRand =
                static_cast<uint16_t>(activationSeed >> 16);
            if (Gen4Wild::species(row) == 349 &&
                !Gen4LeadFrame::feebasTileReplacement(tileRand))
                return false;
            activationSeed = Gen3PidIv::Detail::prev(activationSeed);
        }

        return Gen4LeadFrame::fishingActivationKind(
                   hgss, type,
                   static_cast<uint16_t>(activationSeed >> 16)) ==
               Gen4LeadFrame::Activation::Normal;
    }
    return false;
}

constexpr Result matchStaticMagnetFailure(bool hgss, uint64_t row,
                                          uint32_t candidate,
                                          uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    const uint32_t seed1 = Gen3PidIv::Detail::prev(candidate);
    const uint16_t prev1 = static_cast<uint16_t>(seed1 >> 16);
    const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
    const uint16_t prev2 = static_cast<uint16_t>(seed2 >> 16);

    if (levelIsRandom(hgss, type)) {
        const uint32_t seed3 = Gen3PidIv::Detail::prev(seed2);
        const uint16_t prev3 = static_cast<uint16_t>(seed3 >> 16);
        if (!staticMagnetFailureAllows(hgss, prev3) ||
            rolledLevel(row, prev1) != metLevel)
            return {};

        const uint8_t slot = rolledSlot(hgss, row, prev2);
        if (slot != Gen4Wild::slot(row))
            return {};

        const uint32_t activationSeed = Gen3PidIv::Detail::prev(seed3);
        if (!normalActivationAllows(hgss, row, activationSeed, false))
            return {};
        return {Lead::StaticMagnetPull, candidate, slot, 0};
    }

    if (!staticMagnetFailureAllows(hgss, prev2) ||
        !Gen4Wild::levelMatches(row, metLevel))
        return {};
    const uint8_t slot = rolledSlot(hgss, row, prev1);
    if (slot != Gen4Wild::slot(row))
        return {};
    return {Lead::StaticMagnetPull, candidate, slot, 0};
}

constexpr Result matchPostNatureFailure(bool hgss, uint64_t row,
                                        uint32_t candidate,
                                        uint8_t metLevel,
                                        Lead lead) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    const uint32_t seed1 = Gen3PidIv::Detail::prev(candidate);
    const uint16_t leadRand = static_cast<uint16_t>(seed1 >> 16);
    if (!postNatureFailureAllows(hgss, lead, leadRand))
        return {};

    const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
    const uint16_t prev2 = static_cast<uint16_t>(seed2 >> 16);
    uint32_t slotSeed = seed2;
    uint16_t slotRand = prev2;

    if (levelIsRandom(hgss, type)) {
        if (rolledLevel(row, prev2) != metLevel)
            return {};
        slotSeed = Gen3PidIv::Detail::prev(seed2);
        slotRand = static_cast<uint16_t>(slotSeed >> 16);
    } else if (!Gen4Wild::levelMatches(row, metLevel)) {
        return {};
    }

    const uint8_t slot = rolledSlot(hgss, row, slotRand);
    if (slot != Gen4Wild::slot(row))
        return {};

    const uint32_t activationSeed = Gen3PidIv::Detail::prev(slotSeed);
    if (!normalActivationAllows(
            hgss, row, activationSeed, failureCanSweetScent(lead)))
        return {};
    return {lead, candidate, slot, 0};
}

// Proves one complete lead-failure / encounter-continues origin attempt without
// applying the HG/SS minimum-31 persisted-attempt gate. Earlier rejected origins
// necessarily lack a 31 IV and are validated by matchBugContestReroll below.
constexpr Result matchAttempt(bool hgss, uint64_t row,
                              uint32_t prePidSeed, uint32_t pid,
                              uint8_t metLevel, Lead lead) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (lead == Lead::None || !supportedType(hgss, type))
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
            const Result match = lead == Lead::StaticMagnetPull
                ? matchStaticMagnetFailure(hgss, row, candidate, metLevel)
                : matchPostNatureFailure(hgss, row, candidate, metLevel, lead);
            if (match.matched())
                return match;
        }

        candidate = Gen3PidIv::Detail::prev(
            Gen3PidIv::Detail::prev(candidate));
    }
    return {};
}

// Conservative non-Synchronize subset of pinned Method K RecurseReject. The
// complete fixed-lead failure/continue history is proven at the pre-reroll
// origin. Later attempts prove only their generated nature and preceding
// minimum-31 rejection. Synchronize is intentionally excluded because pinned
// Method K carries a separate forced-nature lock across recursion.
constexpr Result matchBugContestReroll(bool hgss, uint64_t row,
                                       uint32_t prePidSeed, uint32_t pid,
                                       uint8_t metLevel, Lead lead,
                                       uint8_t depth) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (!hgss || !Gen4LeadFrame::isBugContest(type) ||
        lead == Lead::None || lead == Lead::Synchronize ||
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
        hgss, row, attemptSeed, attemptPid, metLevel, lead);
    if (!origin.matched())
        return {};
    origin.rerollDepth = depth;
    return origin;
}

constexpr Result matchRow(bool hgss, uint64_t row,
                          uint32_t prePidSeed, uint32_t pid,
                          uint8_t metLevel, Lead lead) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (lead == Lead::None || !supportedType(hgss, type))
        return {};

    if (!Gen4LeadFrame::isBugContest(type))
        return matchAttempt(hgss, row, prePidSeed, pid, metLevel, lead);

    if (Gen4LeadFrame::directMinimum31Satisfied(prePidSeed)) {
        const Result direct = matchAttempt(
            hgss, row, prePidSeed, pid, metLevel, lead);
        if (direct.matched())
            return direct;
    }

    if (lead == Lead::Synchronize)
        return {};

    for (uint8_t depth = 1; depth <= 3; ++depth) {
        const Result rerolled = matchBugContestReroll(
            hgss, row, prePidSeed, pid, metLevel, lead, depth);
        if (rerolled.matched())
            return rerolled;
    }
    return {};
}

constexpr const char* leadName(Lead lead) noexcept {
    switch (lead) {
        case Lead::Synchronize: return "Synchronize fail";
        case Lead::CuteCharm: return "Cute Charm fail";
        case Lead::PressureHustleVitalSpirit:
            return "Pressure/Hustle/Vital Spirit fail";
        case Lead::StaticMagnetPull: return "Static/Magnet Pull fail";
        case Lead::IntimidateKeenEye:
            return "Intimidate/Keen Eye encounter-continues";
        case Lead::None: break;
    }
    return "No failed lead evidence";
}

} // namespace Legality::Gen4LeadFailure
