#pragma once
// L2 B18 — separately attested Diamond/Pearl original egg PID+IV-stage
// hypotheses. Historical egg creation cannot be reconstructed from a PK4.
// Entirely read-only; candidate contradictions never prove global Invalid.
// Sources: pret/pokediamond@b623abef1b4dfe204ca50aab42447ac00c134290
// arm9/overlays/05/asm/ov05_021EC458.s ov05_021EC9D4,
// ov05_021EC944 and ov05_Daycare_CreateEgg. Uses independently
// implemented B12/B13 source-backed stage replayers.
#include "Legality/Gen4DiamondPearlEggIVStage.h"
#include "Legality/Gen4DiamondPearlEggPidHistory.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4DiamondPearlEggAlternativeHistories {
namespace IV=Gen4DiamondPearlEggIVStage;
namespace PID=Gen4DiamondPearlEggPidHistory;
enum class Status : uint8_t {
    UnsupportedGame, NoCandidates, CapacityExceededUnknown,
    UnknownOriginalEgg, MissingOrDuplicateCandidateIdentity,
    OneConditionallyConsistentHistory, UnknownCandidateProvenance,
    UnresolvedCandidateHistory, UnknownHypothesisEnumeration,
    AllAttestedDaycareStageHypothesesContradicted
};
struct Candidate {
    uint64_t routeIdentity=0;
    uint64_t originalEggEventIdentity=0;
    bool independentlyAttestedHypothesis=false;
    bool allMethodsForThisCandidateIndependentlyCovered=false;
    IV::OriginalStage iv{};
    PID::Input pid{};
};
struct Input {
    std::string_view game{};
    uint64_t originalEggEventIdentity=0;
    bool originalUnhatchedEggIdentityIndependentlyVerified=false;
    bool originalEggPIDIndependentlyVerified=false;
    bool originalEggIVsIndependentlyVerified=false;
    uint32_t originalEggPID=0;
    std::array<uint8_t,6> originalEggIVs{};
    bool allDaycareStageHypothesesIndependentlyEnumerated=false;
    std::array<Candidate,8> alternatives{};
    uint8_t count=0;
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,consistent=0,contradicted=0,unresolved=0,unverified=0;
    std::array<IV::Check,8> ivChecks{};
    std::array<PID::Status,8> pidChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!IV::supported(in.game)){
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.count)return out;
    if(in.count>in.alternatives.size()){
        out.status=Status::CapacityExceededUnknown;return out;
    }
    if(!in.originalEggEventIdentity ||
       !in.originalUnhatchedEggIdentityIndependentlyVerified ||
       !in.originalEggPIDIndependentlyVerified ||
       !in.originalEggIVsIndependentlyVerified ||
       !IV::valid(in.originalEggIVs)) {
        out.status=Status::UnknownOriginalEgg;return out;
    }
    // A repeated/zero archive route cannot be independently exhaustive.
    // Refuse the WHOLE candidate set before looking at any outcome.
    for(uint8_t i=0;i<in.count;++i){
        if(!in.alternatives[i].routeIdentity){
            out.status=Status::MissingOrDuplicateCandidateIdentity;return out;
        }
        for(uint8_t j=0;j<i;++j){
            if(in.alternatives[i].routeIdentity==in.alternatives[j].routeIdentity){
                out.status=Status::MissingOrDuplicateCandidateIdentity;return out;
            }
        }
    }
    for(uint8_t i=0;i<in.count;++i){
        ++out.considered;
        const auto& c=in.alternatives[i];
        if(!c.independentlyAttestedHypothesis ||
           c.originalEggEventIdentity!=in.originalEggEventIdentity ||
           c.iv.game!=in.game || c.pid.game!=in.game ||
           c.pid.parentStageToken!=in.originalEggEventIdentity ||
           c.pid.mtStageToken!=in.originalEggEventIdentity ||
           c.pid.collectionStageToken!=in.originalEggEventIdentity ||
           !c.iv.originalUnhatchedEggIVsVerified ||
           !c.pid.originalUnhatchedEggPidIndependentlyVerified ||
           c.iv.originalEggIVs!=in.originalEggIVs ||
           c.pid.originalUnhatchedEggPid!=in.originalEggPID) {
            ++out.unverified;continue;
        }
        out.ivChecks[i]=IV::verify(c.iv);
        const auto pid=PID::evaluate(c.pid);
        out.pidChecks[i]=pid.status;
        if(out.ivChecks[i]==IV::Check::Matched &&
           pid.status==PID::Status::ConditionalOriginalEggPIDMatched){
            ++out.consistent;continue;
        }
        // These contradictions exclude only this externally attested
        // candidate, never the egg's entire obtainable history.
        const bool impossibleIV=out.ivChecks[i]==IV::Check::Contradicted;
        const bool impossiblePID=
            pid.status==PID::Status::OriginalEggPIDContradicted ||
            pid.status==PID::Status::MTConsumptionContradicted ||
            pid.status==PID::Status::IncompatibleDaycarePair;
        if((impossibleIV || impossiblePID) &&
           c.allMethodsForThisCandidateIndependentlyCovered) {
            ++out.contradicted;continue;
        }
        ++out.unresolved;
    }
    if(out.consistent)out.status=Status::OneConditionallyConsistentHistory;
    else if(out.unverified)out.status=Status::UnknownCandidateProvenance;
    else if(out.unresolved)out.status=Status::UnresolvedCandidateHistory;
    else if(!in.allDaycareStageHypothesesIndependentlyEnumerated)
        out.status=Status::UnknownHypothesisEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllAttestedDaycareStageHypothesesContradicted;
    else out.status=Status::UnresolvedCandidateHistory;
    return out;
}
static_assert(!IV::supported("heartgold_nds"));
static_assert(PID::supports("pearl_nds"));
} // namespace Legality::Gen4DiamondPearlEggAlternativeHistories
