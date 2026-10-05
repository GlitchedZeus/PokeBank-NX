#pragma once

#include "Legality/Gen4LeadEffectEvidence.h"
#include "Legality/Gen4WildRngCorrelation.h"

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

    constexpr bool matched() const noexcept { return lead != Lead::None; }
};

constexpr bool failureAllowsEncounter(bool hgss, Lead lead,
                                      uint16_t rand16) noexcept {
    const auto method = hgss
        ? Gen4LeadEffect::Method::K
        : Gen4LeadEffect::Method::J;
    switch (lead) {
        case Lead::Synchronize:
            return Gen4LeadEffect::synchronizeFail(method, rand16);
        case Lead::CuteCharm:
            return Gen4LeadEffect::cuteCharmFail(method, rand16);
        case Lead::PressureHustleVitalSpirit:
            return Gen4LeadEffect::pressureHustleVitalSpiritFail(method, rand16);
        case Lead::StaticMagnetPull:
            return Gen4LeadEffect::staticMagnetFail(method, rand16);
        case Lead::IntimidateKeenEye:
            // A successful Intimidate/Keen Eye check aborts the encounter.
            return Gen4LeadEffect::intimidateKeenEyeEncounterContinues(method, rand16);
        case Lead::None:
            break;
    }
    return false;
}

constexpr bool supportedType(bool hgss, uint8_t type) noexcept {
    // Keep BCC/Safari out of this first tranche: Method K has minimum-31 rerolls,
    // and BCC also has lead-dependent Sweet Scent/activation deadlocks.
    if (type == 8 || Gen4WildRng::isSafari(type))
        return false;
    if (type > 9)
        return false;
    if ((type == 5 || Gen4WildRng::isHeadbutt(type)) && !hgss)
        return false;
    if (Gen4WildRng::isHoneyTree(type) && hgss)
        return false;
    return true;
}

constexpr bool levelIsRandom(bool hgss, uint8_t type) noexcept {
    // Pinned SlotType4Extensions: D/P/Pt randomize level for every non-Grass
    // slot; HG/SS randomizes every non-Grass, non-Safari slot.
    if (type == 0)
        return false;
    if (hgss && Gen4WildRng::isSafari(type))
        return false;
    return true;
}

constexpr uint8_t rolledSlot(bool hgss, uint64_t row,
                             uint16_t rand16) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (type == 0 || type == 1)
        return hgss
            ? Gen4WildRng::methodKSlot(type, rand16)
            : Gen4WildRng::methodJSlot(type, rand16);
    if (Gen4WildRng::isFishing(type))
        return Gen4WildRng::fishingSlot(hgss, type, rand16);
    if (type == 5)
        return Gen4WildRng::rockSmashSlot(rand16);
    if (Gen4WildRng::isHeadbutt(type))
        return Gen4WildRng::headbuttSlot(rand16);
    if (Gen4WildRng::isHoneyTree(type))
        return Gen4Wild::slot(row); // pre-determined before Method J
    return 0xFF;
}

constexpr bool normalActivationAllows(bool hgss, uint64_t row,
                                      uint32_t activationSeed) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (type == 0 || type == 1 || Gen4WildRng::isHeadbutt(type) ||
        Gen4WildRng::isHoneyTree(type))
        return true;

    if (type == 5) {
        // The active lead is not Illuminate, so only the ordinary Rock Smash
        // activation branch can support this history.
        return Gen4WildRng::rockSmashActivationKind(
                   Gen4Wild::rate(row),
                   static_cast<uint16_t>(activationSeed >> 16)) ==
               Gen4WildRng::Activation::Normal;
    }

    if (Gen4WildRng::isFishing(type)) {
        if (!hgss && Gen4Wild::rate(row) == 0xFFu) {
            const uint16_t tileRand =
                static_cast<uint16_t>(activationSeed >> 16);
            if (Gen4Wild::species(row) == 349 &&
                !Gen4WildRng::feebasTileReplacement(tileRand))
                return false;
            activationSeed =
                Gen3PidIv::Detail::prev(activationSeed);
        }

        // None of the leads represented here is Suction Cups / Sticky Hold.
        return Gen4WildRng::fishingActivationKind(
                   hgss, type,
                   static_cast<uint16_t>(activationSeed >> 16)) ==
               Gen4WildRng::Activation::Normal;
    }
    return false;
}

// Positive-only reconstruction of the pinned Method J/K branches where a lead
// ability consumes its RNG check but does NOT activate, after which the ordinary
// slot/level routine produces the encounter. Intimidate/Keen Eye is represented
// by its encounter-continues branch because its opposite result aborts the battle.
constexpr Result matchRow(bool hgss, uint64_t row,
                          uint32_t prePidSeed, uint32_t pid,
                          uint8_t metLevel, Lead lead) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (lead == Lead::None || !supportedType(hgss, type))
        return {};

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    const int frames = Gen4WildRng::reversalWindow(prePidSeed, nature);
    if (frames < 0)
        return {};

    uint32_t candidate = prePidSeed;
    for (int i = 0; i <= frames; ++i) {
        const uint16_t natureRand = static_cast<uint16_t>(candidate >> 16);
        const uint32_t rolledNature = hgss
            ? (natureRand % 25u)
            : (natureRand / 0x0A3Eu);

        // Pinned TryGetMatchNoSync explores these failure branches only from a
        // regular-nature frame. Successful Synchronize is a separate path.
        if (rolledNature == nature) {
            const uint32_t seed1 = Gen3PidIv::Detail::prev(candidate);
            const uint16_t leadRand = static_cast<uint16_t>(seed1 >> 16);
            if (failureAllowsEncounter(hgss, lead, leadRand)) {
                const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
                const uint16_t prev2 = static_cast<uint16_t>(seed2 >> 16);

                uint32_t slotSeed = seed2;
                uint16_t slotRand = prev2;

                if (levelIsRandom(hgss, type)) {
                    const uint8_t level = Gen4WildRng::isHoneyTree(type)
                        ? Gen4WildRng::honeyTreeLevel(prev2)
                        : Gen4WildRng::randomLevel(
                            Gen4Wild::minLevel(row),
                            Gen4Wild::maxLevel(row), prev2);
                    if (level != metLevel) {
                        candidate = Gen3PidIv::Detail::prev(
                            Gen3PidIv::Detail::prev(candidate));
                        continue;
                    }
                    slotSeed = Gen3PidIv::Detail::prev(seed2);
                    slotRand = static_cast<uint16_t>(slotSeed >> 16);
                } else if (!Gen4Wild::levelMatches(row, metLevel)) {
                    candidate = Gen3PidIv::Detail::prev(
                        Gen3PidIv::Detail::prev(candidate));
                    continue;
                }

                const uint8_t slot = rolledSlot(hgss, row, slotRand);
                if (slot == Gen4Wild::slot(row)) {
                    const uint32_t activationSeed =
                        Gen3PidIv::Detail::prev(slotSeed);
                    if (normalActivationAllows(hgss, row, activationSeed))
                        return {lead, candidate, slot};
                }
            }
        }

        candidate = Gen3PidIv::Detail::prev(
            Gen3PidIv::Detail::prev(candidate));
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
