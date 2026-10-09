#pragma once

#include "Legality/Gen4LeadHistoryEvidence.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace Legality::Gen4LeadReporting {

struct ReportEvidence {
    Gen4LeadHistory::Result history{};

    constexpr bool matched() const noexcept { return history.matched(); }
};

inline ReportEvidence analyzeSupported(std::string_view exactGameId,
                                       uint16_t speciesId,
                                       uint16_t metLocation,
                                       uint8_t metLevel,
                                       uint8_t pokemonForm,
                                       uint32_t id32,
                                       uint32_t prePidSeed,
                                       uint32_t pid) noexcept {
    return {Gen4LeadHistory::analyzeSupported(
        exactGameId, speciesId, metLocation, metLevel, pokemonForm,
        id32, prePidSeed, pid)};
}

inline std::string describe(const ReportEvidence& evidence) {
    if (!evidence.matched())
        return {};

    constexpr std::array<Gen4LeadHistory::Path, 11> kOrder{
        Gen4LeadHistory::Path::StaticSuccess,
        Gen4LeadHistory::Path::MagnetPullSuccess,
        Gen4LeadHistory::Path::PressureSuccess,
        Gen4LeadHistory::Path::SynchronizeFailure,
        Gen4LeadHistory::Path::SynchronizeMixedSuccessThenFailure,
        Gen4LeadHistory::Path::SynchronizeMixedFailureThenSuccess,
        Gen4LeadHistory::Path::SynchronizeMixedMultipleRerolls,
        Gen4LeadHistory::Path::CuteCharmFailure,
        Gen4LeadHistory::Path::PressureFailure,
        Gen4LeadHistory::Path::StaticMagnetFailure,
        Gen4LeadHistory::Path::IntimidateContinue,
    };

    std::string detail =
        "PID/IV spread and wild source match extended Generation IV Method J/K lead history: ";
    bool first = true;
    for (const auto path : kOrder) {
        if (!evidence.history.has(path))
            continue;
        if (!first)
            detail += ", ";
        detail += Gen4LeadHistory::pathName(path);
        first = false;
    }
    detail += "; other unproven lead histories remain incomplete";
    return detail;
}

inline std::string describe(uint16_t mask) {
    ReportEvidence evidence{};
    evidence.history.mask = mask;
    return describe(evidence);
}

inline std::string describeWildResult(uint16_t leadHistoryMask,
                                      std::string_view methodName) {
    const std::string extended = describe(leadHistoryMask);
    if (!extended.empty())
        return extended;

    return "PID/IV spread and wild slot match Generation IV " +
           std::string(methodName) +
           "; uncovered lead and method-specific branches remain incomplete";
}

} // namespace Legality::Gen4LeadReporting
