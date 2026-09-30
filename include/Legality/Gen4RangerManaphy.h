#pragma once

#include "Legality/Gen4MysteryGiftPid.h"
#include "Legality/Gen4PidIvCorrelation.h"

#include <array>
#include <cstdint>

namespace Legality::Gen4RangerManaphy {

inline constexpr uint16_t SpeciesManaphy = 490;
inline constexpr uint16_t LocationLinkTrade4 = 2002;
inline constexpr uint16_t LocationRanger4 = 3001;
inline constexpr uint8_t BallPoke = 4;

struct Candidate {
    uint16_t species = 0;
    uint8_t language = 0;
    uint8_t gender = 0;
    bool isEgg = false;
    uint16_t eggLocation = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool fateful = false;
};

constexpr bool validRangerLanguage(uint8_t language) noexcept {
    // Ranger Manaphy creation disallows unused 0/6 and Korean/future language values.
    return language >= 1 && language <= 7 && language != 6;
}

enum class PidEvidence : uint8_t {
    None,
    Method1,
    AntiShinyRecipient,
    TradedRecipientUnresolved,
};

struct PidResult {
    PidEvidence evidence = PidEvidence::None;
    uint32_t originSeed = 0;
    uint8_t arngRerolls = 0;

    constexpr bool matched() const noexcept {
        return evidence != PidEvidence::None;
    }
};

constexpr bool shinyForTrainer(uint32_t pid, uint16_t tid, uint16_t sid) noexcept {
    return ((static_cast<uint16_t>(pid) ^ static_cast<uint16_t>(pid >> 16) ^ tid ^ sid) >> 3) == 0;
}

constexpr bool matches(const Candidate& c) noexcept {
    if (c.species != SpeciesManaphy || !validRangerLanguage(c.language))
        return false;
    if (c.gender != 2 || c.ball != BallPoke || !c.fateful)
        return false;

    if (c.isEgg)
        return c.eggLocation == LocationRanger4 &&
               (c.metLocation == 0 || c.metLocation == LocationLinkTrade4);

    // A hatched Ranger Manaphy either retains Ranger provenance directly or
    // records that it crossed a link trade while still an egg.
    return c.eggLocation == LocationRanger4 ||
           c.eggLocation == LocationLinkTrade4;
}

constexpr PidResult analyzePidIv(const Candidate& c,
                                 uint32_t pid,
                                 const std::array<uint8_t, 6>& ivs,
                                 uint16_t tid,
                                 uint16_t sid) noexcept {
    if (!matches(c))
        return {};

    // Ranger Manaphy starts from normal Method-1 PID/IV generation.
    const auto method1 = Gen4PidIv::analyze(pid, ivs);
    if (method1.matched()) {
        // An untraded Ranger egg cannot be shiny for its recipient.
        if (c.isEgg && c.metLocation == 0 && shinyForTrainer(pid, tid, sid))
            return {};
        return {PidEvidence::Method1, method1.originSeed, 0};
    }

    // When the original PID would have been shiny, Gen IV can advance the PID
    // with ARNG. Reuse the independently-tested Mystery Gift reverse correlation.
    const auto anti = Gen4MysteryGiftPid::analyze(pid, ivs);
    if (anti.matched) {
        const bool originalWasShiny =
            shinyForTrainer(anti.originalPid, tid, sid);
        if (originalWasShiny)
            return {PidEvidence::AntiShinyRecipient, anti.originSeed, anti.rerolls};

        // Once a Ranger egg has crossed a link trade and hatched, the original
        // recipient TID/SID relationship can no longer be reconstructed from
        // the current PK4 alone. Preserve this as unresolved positive structure.
        if (!c.isEgg && c.eggLocation == LocationLinkTrade4)
            return {PidEvidence::TradedRecipientUnresolved, anti.originSeed, anti.rerolls};
    }

    return {};
}

constexpr const char* pidEvidenceName(PidEvidence evidence) noexcept {
    switch (evidence) {
        case PidEvidence::Method1: return "Ranger Manaphy Method 1";
        case PidEvidence::AntiShinyRecipient: return "Ranger Manaphy anti-shiny ARNG";
        case PidEvidence::TradedRecipientUnresolved: return "Ranger Manaphy traded-recipient ARNG";
        case PidEvidence::None: break;
    }
    return "No Ranger Manaphy PID/IV match";
}

} // namespace Legality::Gen4RangerManaphy
