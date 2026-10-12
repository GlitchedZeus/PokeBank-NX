#pragma once
// L2 Stage 3: bounded SAME-EVENT original Pt/HGSS PID and IV candidates.
// Never combine a PID-positive row with an IV-positive different parent
// or creation-time held-item history. A contradicted daycare candidate
// NEVER implies a globally Invalid native Pokemon.
#include "Legality/Gen4PlatinumHGSSOriginalEggPIDHistory.h"
#include "Legality/Gen4EggIVInheritanceStage.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen4PlatinumHGSSJointEggHistories {
namespace PID=Gen4PlatinumHGSSOriginalEggPIDHistory;
namespace IV=Gen4EggIVInheritanceStage;
enum class Status : uint8_t {
    UnsupportedGame, NoCandidates, CapacityExceededUnknown,
    UnknownOriginalEgg, IncoherentOriginalEvent,
    OneConditionallyConsistentJointHistory,
    UnverifiedOrDuplicateHistory, UnresolvedSourceHistory,
    UnknownExhaustiveEnumeration,
    AllEnumeratedLocalDaycareCandidatesContradicted
};
struct Candidate {
    uint64_t routeIdentity=0,originalDaycarePairIdentity=0;
    bool originalSourceHistoryIndependentlyAttested=false;
    bool fullyAttestedForCandidateLocalExclusion=false;
    bool originalHeldItemsSameAcrossStagesIndependentlyVerified=false;
    PID::Input pid{};
    IV::KnownFinalStage iv{};
};
struct Input {
    std::string_view game{};
    uint64_t originalEggEventIdentity=0;
    bool originalUnhatchedEggIndependentlyVerified=false;
    uint32_t originalEggPID=0;
    std::array<uint8_t,6> originalEggIVs{};
    bool allOrdinaryDaycareAlternativesIndependentlyEnumerated=false;
    uint8_t count=0;
    std::array<Candidate,8> alternatives{};
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,matched=0,contradicted=0,unknown=0,unverified=0;
    std::array<PID::Status,8> pidChecks{};
    std::array<IV::CandidateCheck,8> ivChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(PID::family(in.game)==PID::Family::Unknown ||
       IV::family(in.game)==IV::Family::Unknown){
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.count)return out;
    if(in.count>in.alternatives.size()){
        out.status=Status::CapacityExceededUnknown;return out;
    }
    if(!in.originalUnhatchedEggIndependentlyVerified ||
       !in.originalEggEventIdentity || !IV::validIVs(in.originalEggIVs)){
        out.status=Status::UnknownOriginalEgg;return out;
    }
    bool malformedID=false;
    for(uint8_t i=0;i<in.count;++i){
        ++out.considered;
        const auto& c=in.alternatives[i];
        if(!c.routeIdentity){malformedID=true;++out.unverified;continue;}
        bool repeated=false,priorMatch=false;
        for(uint8_t j=0;j<i;++j)
            if(in.alternatives[j].routeIdentity==c.routeIdentity){
                repeated=true;
                if(out.pidChecks[j]==PID::Status::ConditionalOriginalEggPIDMatched &&
                   out.ivChecks[j]==IV::CandidateCheck::Matched)
                    priorMatch=true;
            }
        if(repeated){malformedID=true;++out.unverified;
            if(priorMatch)continue;
        }
        const auto& p=c.pid;
        const auto& v=c.iv;
        if(!c.originalDaycarePairIdentity ||
           !c.originalSourceHistoryIndependentlyAttested ||
           !c.originalHeldItemsSameAcrossStagesIndependentlyVerified ||
           p.game!=in.game || v.game!=in.game ||
           p.originalEggEventId!=in.originalEggEventIdentity ||
           p.parentStageToken!=in.originalEggEventIdentity ||
           p.pendingStageToken!=in.originalEggEventIdentity ||
           p.collectionStageToken!=in.originalEggEventIdentity ||
           !p.originalEggIdentityIndependentlyVerified ||
           p.originalUnhatchedEggPID!=in.originalEggPID ||
           v.observedChild!=in.originalEggIVs ||
           !p.physicalDepositOrderIndependentlyVerified ||
           !p.parents[0].snapshotId || !p.parents[1].snapshotId ||
           p.parents[0].snapshotId==p.parents[1].snapshotId ||
           !v.parentSnapshotsIndependentlyVerified ||
           !v.depositOrderIndependentlyVerified ||
           p.parents[0].heldItem!=v.heldItem0 ||
           p.parents[1].heldItem!=v.heldItem1){
            ++out.unverified;continue;
        }
        const auto pResult=PID::evaluate(p);
        out.pidChecks[i]=pResult.status;
        const auto vResult=IV::verifyKnownFinalStage(v);
        out.ivChecks[i]=vResult;
        if(pResult.status==PID::Status::ConditionalOriginalEggPIDMatched &&
           vResult==IV::CandidateCheck::Matched){
            ++out.matched;continue;
        }
        if(repeated)continue;
        const bool wrongPID=pResult.status==PID::Status::OriginalEggPIDContradicted ||
                            pResult.status==PID::Status::IncompatibleOrdinaryPair ||
                            pResult.status==PID::Status::ContradictoryMTConsumption;
        const bool wrongIV=vResult==IV::CandidateCheck::CandidateMismatch;
        if((wrongPID||wrongIV) && c.fullyAttestedForCandidateLocalExclusion)
            ++out.contradicted;
        else ++out.unknown;
    }
    if(out.matched)out.status=Status::OneConditionallyConsistentJointHistory;
    else if(malformedID||out.unverified)
        out.status=Status::UnverifiedOrDuplicateHistory;
    else if(out.unknown)out.status=Status::UnresolvedSourceHistory;
    else if(!in.allOrdinaryDaycareAlternativesIndependentlyEnumerated)
        out.status=Status::UnknownExhaustiveEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllEnumeratedLocalDaycareCandidatesContradicted;
    else out.status=Status::UnresolvedSourceHistory;
    return out;
}
} // namespace Legality::Gen4PlatinumHGSSJointEggHistories
