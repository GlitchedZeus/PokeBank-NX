#pragma once
// L2 B08/B18: finite, independently attested Gen III ordinary-daycare
// egg histories across PID, original six-draw IV stage and all four moves.
// One consistent route does NOT prove authentic retail parent history;
// exhaustive local daycare failure does NOT prove overall Pokemon Invalid.
// Separate gift/event/trade origins and erased historical stages survive.
#include "Legality/Gen3JointEggPIDIVMoveHistory.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3EggFullHistoryAlternatives {
namespace Joint=Gen3JointEggPIDIVMoveHistory;
namespace PID=Gen3EggPIDCreationHistory;
enum class Status : uint8_t {
    UnsupportedGame, NoCandidates, CapacityExceededUnknown,
    UnknownOriginalEgg, DuplicateOrMissingRoute,
    OneConditionallyConsistentHistory, UnverifiedCandidateProvenance,
    UnresolvedCandidateHistory, UnknownAlternativeEnumeration,
    AllAttestedOrdinaryDaycareRoutesContradicted
};
struct Route {
    uint64_t routeIdentity=0;
    uint64_t claimedOriginalEggIdentity=0;
    bool originalSourceHistoryIndependentlyAttested=false;
    bool thisCandidateMethodEvidenceFullyAttested=false;
    Joint::Input history{};
};
struct Input {
    std::string_view game{};
    uint64_t originalEggIdentity=0;
    bool originalUnhatchedEggEvidenceIndependentlyVerified=false;
    uint16_t originalEggSpecies=0;
    uint32_t originalEggPID=0;
    std::array<uint8_t,6> originalEggIVs{};
    std::array<uint16_t,4> originalEggMoves{};
    bool originalEggMovesIndependentlyVerified=false;
    bool allOrdinaryDaycareAlternativesIndependentlyEnumerated=false;
    uint8_t count=0;
    std::array<Route,8> alternatives{};
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,consistent=0,contradicted=0,unresolved=0,unverified=0;
    std::array<Joint::Status,8> stageChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(PID::family(in.game)==PID::Family::Unsupported) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.count)return out;
    if(in.count>in.alternatives.size()) {
        out.status=Status::CapacityExceededUnknown;return out;
    }
    if(!in.originalEggIdentity ||
       !in.originalUnhatchedEggEvidenceIndependentlyVerified ||
       !in.originalEggMovesIndependentlyVerified ||
       !in.originalEggSpecies ||
       !Gen3EggIVInheritanceStage::validIVs(in.originalEggIVs)) {
        out.status=Status::UnknownOriginalEgg;return out;
    }
    for(uint8_t i=0;i<in.count;++i) {
        if(!in.alternatives[i].routeIdentity) {
            out.status=Status::DuplicateOrMissingRoute;return out;
        }
        for(uint8_t j=0;j<i;++j)
            if(in.alternatives[i].routeIdentity==
               in.alternatives[j].routeIdentity) {
                out.status=Status::DuplicateOrMissingRoute;return out;
            }
    }
    for(uint8_t i=0;i<in.count;++i) {
        const Route& r=in.alternatives[i];
        ++out.considered;
        const auto& x=r.history;
        if(!r.originalSourceHistoryIndependentlyAttested ||
           r.claimedOriginalEggIdentity!=in.originalEggIdentity ||
           x.pid.game!=in.game ||
           x.moveAndIV.moveHistory.game!=in.game ||
           x.originalEggSpecies!=in.originalEggSpecies ||
           x.moveAndIV.moveHistory.species!=in.originalEggSpecies ||
           x.pid.originalUnhatchedEggPID!=in.originalEggPID ||
           x.moveAndIV.observedUnhatchedIVs!=in.originalEggIVs ||
           x.moveAndIV.moveHistory.observed!=in.originalEggMoves) {
            ++out.unverified;continue;
        }
        const auto verdict=Joint::evaluate(x);
        out.stageChecks[i]=verdict.status;
        if(verdict.status==Joint::Status::AllThreeStagesConditionallyMatched) {
            ++out.consistent;continue;
        }
        const bool localContradiction=
            verdict.status==Joint::Status::OriginalMoveCandidateContradicted ||
            verdict.status==Joint::Status::OriginalIVCandidateContradicted ||
            verdict.status==Joint::Status::OriginalPIDCandidateContradicted;
        if(localContradiction && r.thisCandidateMethodEvidenceFullyAttested) {
            ++out.contradicted;continue;
        }
        ++out.unresolved;
    }
    if(out.consistent)out.status=Status::OneConditionallyConsistentHistory;
    else if(out.unverified)out.status=Status::UnverifiedCandidateProvenance;
    else if(out.unresolved)out.status=Status::UnresolvedCandidateHistory;
    else if(!in.allOrdinaryDaycareAlternativesIndependentlyEnumerated)
        out.status=Status::UnknownAlternativeEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllAttestedOrdinaryDaycareRoutesContradicted;
    else out.status=Status::UnresolvedCandidateHistory;
    return out;
}
} // namespace Legality::Gen3EggFullHistoryAlternatives
