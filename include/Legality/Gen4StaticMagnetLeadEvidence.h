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

// Mirrors pinned IMagnetStatic.IsSlotValidStaticMagnet. Static is checked first,
// then Magnet Pull. A zero count means that lead cannot yield this source slot.
constexpr Lead attractedLead(uint32_t meta, uint16_t rand16) noexcept {
    const uint8_t sc = staticCount(meta);
    if (sc != 0 && (rand16 % sc) == staticIndex(meta))
        return Lead::Static;

    const uint8_t mc = magnetCount(meta);
    if (mc != 0 && (rand16 % mc) == magnetIndex(meta))
        return Lead::MagnetPull;
    return Lead::None;
}

// Positive-only reconstruction of a successful Static / Magnet Pull lead path.
// The 32-bit metadata is the source-area alias emitted alongside each packed
// Gen IV wild row by gen_legality_gen4_wild.py.
constexpr Result matchRow(bool hgss, uint64_t row, uint32_t leadMeta,
                          uint32_t prePidSeed, uint32_t pid,
                          uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (!Gen4LeadFailure::supportedType(hgss, type))
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
            const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
            const uint16_t prev2 = static_cast<uint16_t>(seed2 >> 16);

            if (Gen4LeadFailure::levelIsRandom(hgss, type)) {
                const uint32_t seed3 = Gen3PidIv::Detail::prev(seed2);
                const uint16_t prev3 = static_cast<uint16_t>(seed3 >> 16);

                // Pinned Method J/K: Prev3=50% proc, Prev2=attracted slot,
                // Prev1=ordinary level. The proc must activate.
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
                            hgss, row, activationSeed))
                        return {lead, candidate};
                }
            } else {
                // Fixed-level Grass differs between the two retail methods.
                // Pinned Method J consumes an extra frame before the 50% proc:
                // Prev3=proc, Prev1=attracted slot (Prev2 is skipped here).
                // Pinned Method K uses Prev2=proc, Prev1=attracted slot.
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
                        return {lead, candidate};
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
        case Lead::Static: return "Static";
        case Lead::MagnetPull: return "Magnet Pull";
        case Lead::None: break;
    }
    return "No Static/Magnet Pull evidence";
}

} // namespace Legality::Gen4StaticMagnetLead
