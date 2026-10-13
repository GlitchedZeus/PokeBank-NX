#pragma once
// L2 B04/B05/B06/B11/B18: explicit original-stage Gen III ordinary egg IV
// candidates. Source retail: pret/pokeruby@5784633c src/daycare.c,
// pret/pokeemerald@731ad5bf src/daycare.c (BUGFIX off),
// pret/pokefirered@037335f4 src/daycare.c. This composes the accepted
// independent Gen3EggIVInheritanceStage replay; no stage RNG or historical
// parent is inferred from a detached PK3. Candidate contradiction is LOCAL.
#include "Legality/Gen3EggIVInheritanceStage.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3EggIVAttestedAlternatives {
namespace IV=Gen3EggIVInheritanceStage;
enum class Status : uint8_t {
    UnsupportedGame, NoCandidates, CapacityExceededUnknown,
    UnknownOriginalEgg, InvalidObservedIVDomain,
    MissingOrDuplicateRouteIdentity, UnknownCandidateProvenance,
    OneConditionallyConsistentStage, UnresolvedStageHistory,
    UnknownHypothesisEnumeration,
    AllEnumeratedIVStageHypothesesContradicted
};
struct Route {
    uint64_t routeIdentity=0;
    uint64_t originalEggEventIdentity=0;
    uint64_t daycareParentStageToken=0,ivParentStageToken=0;
    uint64_t originalDaycarePairIdentity=0,ivDonorPairIdentity=0;
    bool originalDaycareStageIndependentlyVerified=false;
    bool originalParentIVSnapshotsIndependentlyVerified=false;
    bool physicalDepositOrderIndependentlyVerified=false;
    bool originalPreInheritanceChildIVsIndependentlyVerified=false;
    bool knownFinalIVRNGStateIndependentlyVerified=false;
    bool candidateMethodFullyAttested=false;
    uint32_t physicalSlot0ParentId=0,physicalSlot1ParentId=0;
    uint32_t finalIVRNGState=0;
    std::array<uint8_t,6> originalBeforeInheritance{};
    std::array<uint8_t,6> physicalSlot0OriginalIVs{};
    std::array<uint8_t,6> physicalSlot1OriginalIVs{};
};
struct Input {
    std::string_view game{};
    uint64_t originalEggEventIdentity=0;
    bool originalUnhatchedEggIdentityIndependentlyVerified=false;
    bool originalEggIVsIndependentlyVerified=false;
    std::array<uint8_t,6> originalUnhatchedEggIVs{};
    bool allOriginalIVStageHypothesesIndependentlyEnumerated=false;
    uint8_t count=0;
    std::array<Route,16> alternatives{};
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,consistent=0,contradicted=0,unresolved=0,unverified=0;
    std::array<IV::CandidateCheck,16> ivChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(IV::family(in.game)==IV::Family::Unknown){
        out.status=Status::UnsupportedGame;return out;
    }
    if(in.count==0)return out;
    if(in.count>in.alternatives.size()){
        out.status=Status::CapacityExceededUnknown;return out;
    }
    if(!in.originalEggEventIdentity ||
       !in.originalUnhatchedEggIdentityIndependentlyVerified ||
       !in.originalEggIVsIndependentlyVerified){
        out.status=Status::UnknownOriginalEgg;return out;
    }
    if(!IV::validIVs(in.originalUnhatchedEggIVs)){
        out.status=Status::InvalidObservedIVDomain;return out;
    }
    // Ignore duplicate/zero historical route identities for matching
    // and count them unresolved for exhaustion. They cannot conceal a
    // different, independently sourced *matching* original parent history.
    bool malformedRouteIdentity=false;
    for(uint8_t i=0;i<in.count;++i){
        const Route& route=in.alternatives[i];
        ++out.considered;
        if(!route.routeIdentity){
            malformedRouteIdentity=true;
            ++out.unverified;
            continue;
        }
        bool repeated=false,priorMatch=false;
        for(uint8_t j=0;j<i;++j)
            if(route.routeIdentity==in.alternatives[j].routeIdentity){
                repeated=true;
                if(out.ivChecks[j]==IV::CandidateCheck::Matched)
                    priorMatch=true;
            }
        if(repeated){
            malformedRouteIdentity=true;
            ++out.unverified;
            if(priorMatch)continue;
        }
        // Physical deposited slots must be the SAME original pair whose
        // parent IVs are being replayed. Never silently swap father/mother
        // because the child's IVs happened to match a different order.
        if(!route.originalDaycareStageIndependentlyVerified ||
           !route.originalParentIVSnapshotsIndependentlyVerified ||
           !route.physicalDepositOrderIndependentlyVerified ||
           !route.originalPreInheritanceChildIVsIndependentlyVerified ||
           !route.knownFinalIVRNGStateIndependentlyVerified ||
           !route.originalEggEventIdentity ||
           route.originalEggEventIdentity!=in.originalEggEventIdentity ||
           !route.daycareParentStageToken ||
           route.daycareParentStageToken!=route.ivParentStageToken ||
           !route.originalDaycarePairIdentity ||
           route.originalDaycarePairIdentity!=route.ivDonorPairIdentity ||
           !route.physicalSlot0ParentId || !route.physicalSlot1ParentId ||
           route.physicalSlot0ParentId==route.physicalSlot1ParentId) {
            ++out.unverified;continue;
        }
        const auto check=IV::verifyKnownStage(
            in.game,route.finalIVRNGState,in.originalUnhatchedEggIVs,
            route.originalBeforeInheritance,
            route.physicalSlot0OriginalIVs,route.physicalSlot1OriginalIVs,
            true);
        out.ivChecks[i]=check;
        if(check==IV::CandidateCheck::Matched){++out.consistent;continue;}
        if(repeated)continue; // duplicated evidence cannot exclude all routes
        if(check==IV::CandidateCheck::CandidateMismatch &&
           route.candidateMethodFullyAttested){
            ++out.contradicted;continue;
        }
        // Historical fields outside 0..31 invalidate this hypothetical
        // route's input; they do not prove the source egg itself Illegal.
        ++out.unresolved;
    }
    if(out.consistent)out.status=Status::OneConditionallyConsistentStage;
    else if(malformedRouteIdentity)
        out.status=Status::MissingOrDuplicateRouteIdentity;
    else if(out.unverified)out.status=Status::UnknownCandidateProvenance;
    else if(out.unresolved)out.status=Status::UnresolvedStageHistory;
    else if(!in.allOriginalIVStageHypothesesIndependentlyEnumerated)
        out.status=Status::UnknownHypothesisEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllEnumeratedIVStageHypothesesContradicted;
    else out.status=Status::UnresolvedStageHistory;
    return out;
}
static_assert(IV::family("ruby_gba")==IV::Family::RubySapphire);
static_assert(IV::family("sapphire_gba")==IV::Family::RubySapphire);
static_assert(IV::family("emerald_gba")==IV::Family::Emerald);
static_assert(IV::family("firered_gba")==IV::Family::FireRedLeafGreen);
static_assert(IV::family("leafgreen_gba")==IV::Family::FireRedLeafGreen);
} // namespace Legality::Gen3EggIVAttestedAlternatives
