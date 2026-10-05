#pragma once

#include "Legality/Gen4LeadFailureEvidence.h"

#include <cstdint>

namespace Legality::Gen4PressureLead {

enum class Lead : uint8_t {
    None,
    PressureHustleVitalSpirit,
};

struct Result {
    Lead lead = Lead::None;
    uint32_t encounterSeed = 0;
    uint8_t slot = 0;
    uint8_t pressureLevel = 0;

    constexpr bool matched() const noexcept { return lead != Lead::None; }
};

// Pinned EncounterSlot4.PressureLevel:
//   Type != Grass ? LevelMax : Parent.GetPressureMax(Species, LevelMax)
//
// Parent-area identity is not yet retained for flattened Grass aliases, so this
// tranche proves only the exact non-Grass half. Returning 0 for Grass prevents a
// guessed parent-area maximum from becoming positive evidence.
constexpr uint8_t exactPressureLevel(uint64_t row) noexcept {
    return Gen4Wild::method(row) == 0 ? 0 : Gen4Wild::maxLevel(row);
}

// Positive-only reconstruction of a successful Pressure / Hustle / Vital Spirit
// lead branch for source families whose PressureLevel is exactly recoverable from
// the persisted encounter row. Successful pressure consumes the lead check at
// Prev1, consumes but ignores the ordinary random-level call at Prev2, then uses
// Prev3 for the ordinary encounter slot; the resulting level is PressureLevel.
//
// Grass remains deliberately unresolved until source-area PressureLevel metadata
// is retained. HG/SS Bug Contest and Safari remain excluded because their minimum-
// 31 rerolls / activation deadlocks need separate reconstruction.
constexpr Result matchRow(bool hgss, uint64_t row,
                          uint32_t prePidSeed, uint32_t pid,
                          uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    const uint8_t pressureLevel = exactPressureLevel(row);
    if (pressureLevel == 0 || metLevel != pressureLevel ||
        !Gen4LeadFailure::supportedType(hgss, type))
        return {};

    // All supported non-Grass families in this tranche use the random-level
    // Method J/K ordering. Keep this explicit so future family expansion cannot
    // silently reuse the wrong frame layout.
    if (!Gen4LeadFailure::levelIsRandom(hgss, type))
        return {};

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    const int frames = Gen4WildRng::reversalWindow(prePidSeed, nature);
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
            if (Gen4LeadEffect::pressureHustleVitalSpiritPass(
                    Gen4LeadFailure::leadMethod(hgss), prev1)) {
                // Prev2 is the ordinary random-level call. A successful pressure
                // effect still consumes it but overwrites the result with the
                // source PressureLevel, so only Prev3 determines the normal slot.
                const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
                const uint32_t seed3 = Gen3PidIv::Detail::prev(seed2);
                const uint16_t prev3 = static_cast<uint16_t>(seed3 >> 16);
                const uint8_t slot =
                    Gen4LeadFailure::rolledSlot(hgss, row, prev3);

                if (slot == Gen4Wild::slot(row)) {
                    const uint32_t activationSeed =
                        Gen3PidIv::Detail::prev(seed3);
                    // Pressure/Hustle/Vital Spirit cannot simultaneously provide
                    // Illuminate or Suction Cups / Sticky Hold. Require the normal
                    // activation branch for Rock Smash / fishing families.
                    if (Gen4LeadFailure::normalActivationAllows(
                            hgss, row, activationSeed)) {
                        return {
                            Lead::PressureHustleVitalSpirit,
                            candidate,
                            slot,
                            pressureLevel,
                        };
                    }
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
        case Lead::PressureHustleVitalSpirit:
            return "Pressure/Hustle/Vital Spirit";
        case Lead::None:
            break;
    }
    return "No successful pressure lead evidence";
}

} // namespace Legality::Gen4PressureLead
