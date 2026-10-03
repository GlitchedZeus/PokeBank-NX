#pragma once

#include "Legality/Gen3ColoShadowEncounter.h"
#include "Legality/Gen3ColoShadowTeamLock.h"
#include "Legality/Gen3CxdPidIvCorrelation.h"
#include "Legality/Gen3XdShadowEncounter.h"
#include "Legality/Gen3XdShadowTeamLock.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3GameCubeShadowEvidence {

enum class Family : uint8_t {
    None,
    Colosseum,
    XD,
};

enum class HistoryResult : uint8_t {
    None,
    Matched,
    NotMatched,
    SearchLimit,
};

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool isEgg = false;
    bool fateful = false;
    bool shiny = false;
    uint16_t tid = 0;
    uint16_t sid = 0;
    uint32_t pid = 0;
    std::array<uint8_t, 6> ivs{};
};

struct Result {
    Family family = Family::None;
    bool identityMatched = false;
    uint8_t identityCandidateCount = 0;
    bool xdRebattleLocation = false;
    Gen3CxdPidIv::Result correlation{};
    HistoryResult history = HistoryResult::None;
};

constexpr HistoryResult mapHistory(Gen3ColoShadowTeamLock::Result result) noexcept {
    using Source = Gen3ColoShadowTeamLock::Result;
    switch (result) {
        case Source::Matched: return HistoryResult::Matched;
        case Source::SearchLimit: return HistoryResult::SearchLimit;
        case Source::NotMatched: return HistoryResult::NotMatched;
    }
    return HistoryResult::NotMatched;
}

constexpr HistoryResult mapHistory(Gen3XdShadowTeamLock::Result result) noexcept {
    using Source = Gen3XdShadowTeamLock::Result;
    switch (result) {
        case Source::Matched: return HistoryResult::Matched;
        case Source::SearchLimit: return HistoryResult::SearchLimit;
        case Source::NotMatched: return HistoryResult::NotMatched;
    }
    return HistoryResult::NotMatched;
}

inline Result analyze(const Candidate& candidate) {
    Result result{};

    // PK3 stores both GameCube titles with version value 15. Choose an exact
    // source family from persistent encounter evidence before selecting RNG
    // semantics: normal Colosseum is standard CXD only; XD additionally permits
    // the source-defined CXDAnti target-PID reroll class.
    const auto colo = Gen3ColoShadowEncounter::match({
        candidate.species,
        candidate.originGame,
        candidate.metLevel,
        static_cast<uint8_t>(candidate.metLocation),
        candidate.isEgg,
        candidate.fateful,
    });
    if (colo.matched()) {
        result.family = Family::Colosseum;
        result.identityMatched = true;
        result.identityCandidateCount = static_cast<uint8_t>(colo.count);
        result.correlation = Gen3CxdPidIv::analyze(candidate.pid, candidate.ivs);
        if (!result.correlation.matched)
            return result;

        bool limited = false;
        for (std::size_t i = 0; i < colo.count; ++i) {
            const auto history = Gen3ColoShadowTeamLock::validate(
                Gen3ColoShadowEncounter::teamSet(*colo.entries[i]),
                result.correlation.originSeed);
            if (history == Gen3ColoShadowTeamLock::Result::Matched) {
                result.history = HistoryResult::Matched;
                return result;
            }
            if (history == Gen3ColoShadowTeamLock::Result::SearchLimit)
                limited = true;
        }
        result.history = limited ? HistoryResult::SearchLimit
                                 : HistoryResult::NotMatched;
        return result;
    }

    const auto xd = Gen3XdShadowEncounter::match({
        candidate.species,
        candidate.originGame,
        candidate.metLevel,
        candidate.metLocation,
        candidate.ball,
        candidate.isEgg,
        candidate.fateful,
        candidate.shiny,
    });
    if (xd.matched) {
        result.family = Family::XD;
        result.identityMatched = true;
        result.identityCandidateCount = 1;
        result.xdRebattleLocation = xd.rebattleLocation;
        result.correlation = Gen3CxdPidIv::analyzeWithTrainer(
            candidate.pid, candidate.ivs, candidate.tid, candidate.sid);
        if (!result.correlation.matched)
            return result;

        result.history = mapHistory(Gen3XdShadowTeamLock::validateXd(
            xd.index,
            result.correlation.originSeed,
            candidate.tid,
            candidate.sid));
        return result;
    }

    // A generic standard GameCube RNG correlation remains useful evidence, but
    // without a pinned shadow identity it cannot establish Colosseum or XD
    // provenance. CXDAnti is intentionally not attempted without exact XD identity.
    result.correlation = Gen3CxdPidIv::analyze(candidate.pid, candidate.ivs);
    return result;
}

} // namespace Legality::Gen3GameCubeShadowEvidence
