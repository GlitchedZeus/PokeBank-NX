#include "Legality/Gen3EggIVAttestedAlternatives.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen3EggIVAttestedAlternatives;
namespace IV=Legality::Gen3EggIVInheritanceStage;
constexpr std::array<uint8_t,6> initial{0,3,6,9,12,15};
constexpr std::array<uint8_t,6> slot0{7,12,17,22,27,0};
constexpr std::array<uint8_t,6> slot1{13,24,3,14,25,4};
A::Input candidate(const char* game,uint32_t rngSeed){
    A::Input in{};
    in.game=game;
    in.originalEggEventIdentity=0x33330001u;
    in.originalUnhatchedEggIdentityIndependentlyVerified=true;
    in.originalEggIVsIndependentlyVerified=true;
    in.allOriginalIVStageHypothesesIndependentlyEnumerated=true;
    in.count=1;
    const auto result=IV::replay(game,rngSeed,initial,slot0,slot1,true);
    assert(result.status==IV::Status::ReplayedStage);
    in.originalUnhatchedEggIVs=result.childIVs;
    auto& route=in.alternatives[0];
    route.routeIdentity=1;
    route.originalEggEventIdentity=in.originalEggEventIdentity;
    route.daycareParentStageToken=0xA001;
    route.ivParentStageToken=0xA001;
    route.originalDaycarePairIdentity=0xD3;
    route.ivDonorPairIdentity=0xD3;
    route.originalDaycareStageIndependentlyVerified=true;
    route.originalParentIVSnapshotsIndependentlyVerified=true;
    route.physicalDepositOrderIndependentlyVerified=true;
    route.originalPreInheritanceChildIVsIndependentlyVerified=true;
    route.knownFinalIVRNGStateIndependentlyVerified=true;
    route.candidateMethodFullyAttested=true;
    route.physicalSlot0ParentId=100;route.physicalSlot1ParentId=200;
    route.finalIVRNGState=result.finalSeed;
    route.originalBeforeInheritance=initial;
    route.physicalSlot0OriginalIVs=slot0;
    route.physicalSlot1OriginalIVs=slot1;
    return in;
}
int main() {
    unsigned positives=0;
    // Positive stage vector and full candidate-local witness for all 5
    // generation-III original retail families (no fake Gen IV dispatch).
    for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba",
                          "firered_gba","leafgreen_gba"}) {
        for(uint32_t seed=0;seed<4096;++seed) {
            const auto x=candidate(game,seed);
            const auto good=A::evaluate(x);
            assert(good.status==A::Status::OneConditionallyConsistentStage);
            assert(good.consistent==1 && good.contradicted==0 &&
                   good.unverified==0 && good.unresolved==0);
            // A source-referenced alternative that provably cannot
            // create the original egg's IV array may be ruled out ONLY
            // as a candidate-local historical stage, never globally.
            auto mismatch=x;
            for(size_t stat=0;stat<6;++stat) {
                const uint8_t alternate=static_cast<uint8_t>(
                    (x.originalUnhatchedEggIVs[stat]+1u)%32u);
                mismatch.alternatives[0].originalBeforeInheritance[stat]=alternate;
                mismatch.alternatives[0].physicalSlot0OriginalIVs[stat]=alternate;
                mismatch.alternatives[0].physicalSlot1OriginalIVs[stat]=alternate;
            }
            assert(A::evaluate(mismatch).status==
                   A::Status::AllEnumeratedIVStageHypothesesContradicted);
            mismatch.allOriginalIVStageHypothesesIndependentlyEnumerated=false;
            assert(A::evaluate(mismatch).status==
                   A::Status::UnknownHypothesisEnumeration);
            mismatch.count=2;
            mismatch.alternatives[1]=x.alternatives[0];
            mismatch.alternatives[1].routeIdentity=2;
            const auto rescued=A::evaluate(mismatch);
            assert(rescued.status==A::Status::OneConditionallyConsistentStage);
            assert(rescued.consistent==1 && rescued.contradicted==1);
            // Original six-draw operation is invariant; swapping parent
            // arrays with the same recorded RNG is NOT equivalent to
            // automatic father/mother role swapping.
            auto reverse=x;
            const auto old=reverse.alternatives[0].physicalSlot0OriginalIVs;
            reverse.alternatives[0].physicalSlot0OriginalIVs=
                reverse.alternatives[0].physicalSlot1OriginalIVs;
            reverse.alternatives[0].physicalSlot1OriginalIVs=old;
            const auto reverseCheck=A::evaluate(reverse);
            assert(reverseCheck.status==A::Status::OneConditionallyConsistentStage ||
                   reverseCheck.status==A::Status::AllEnumeratedIVStageHypothesesContradicted);
            ++positives;
        }
        auto x=candidate(game,0);
        auto noProof=x;noProof.originalEggIVsIndependentlyVerified=false;
        assert(A::evaluate(noProof).status==A::Status::UnknownOriginalEgg);
        noProof=x;noProof.originalUnhatchedEggIdentityIndependentlyVerified=false;
        assert(A::evaluate(noProof).status==A::Status::UnknownOriginalEgg);
        noProof=x;noProof.originalEggEventIdentity=0;
        assert(A::evaluate(noProof).status==A::Status::UnknownOriginalEgg);
        auto badIV=x;badIV.originalUnhatchedEggIVs[0]=32;
        assert(A::evaluate(badIV).status==A::Status::InvalidObservedIVDomain);
        auto noStage=x;noStage.alternatives[0].originalDaycareStageIndependentlyVerified=false;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        noStage=x;noStage.alternatives[0].originalParentIVSnapshotsIndependentlyVerified=false;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        noStage=x;noStage.alternatives[0].physicalDepositOrderIndependentlyVerified=false;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        noStage=x;noStage.alternatives[0].originalPreInheritanceChildIVsIndependentlyVerified=false;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        noStage=x;noStage.alternatives[0].knownFinalIVRNGStateIndependentlyVerified=false;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        noStage=x;noStage.alternatives[0].daycareParentStageToken++;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        noStage=x;noStage.alternatives[0].ivDonorPairIdentity++;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        noStage=x;noStage.alternatives[0].physicalSlot0ParentId=
            noStage.alternatives[0].physicalSlot1ParentId;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        auto noEnumeration=x;noEnumeration.alternatives[0].candidateMethodFullyAttested=false;
        noEnumeration.originalUnhatchedEggIVs[0]=static_cast<uint8_t>(
            (noEnumeration.originalUnhatchedEggIVs[0]+1u)%32u);
        // Outcome is not inherently guaranteed to be mismatched
        // if stat 0 never inherited; use a provably disjoint parent
        // set for the contradiction below.
        for(size_t i=0;i<6;++i) {
            noEnumeration.alternatives[0].originalBeforeInheritance[i]=0;
            noEnumeration.alternatives[0].physicalSlot0OriginalIVs[i]=0;
            noEnumeration.alternatives[0].physicalSlot1OriginalIVs[i]=0;
            noEnumeration.originalUnhatchedEggIVs[i]=31;
        }
        assert(A::evaluate(noEnumeration).status==A::Status::UnresolvedStageHistory);
        auto malformedParent=x;malformedParent.alternatives[0].physicalSlot1OriginalIVs[2]=32;
        assert(A::evaluate(malformedParent).status==A::Status::UnresolvedStageHistory);
        auto duplicate=x;duplicate.count=2;duplicate.alternatives[1]=duplicate.alternatives[0];
        const auto rescuedDuplicate=A::evaluate(duplicate);
        assert(rescuedDuplicate.status==A::Status::OneConditionallyConsistentStage);
        assert(rescuedDuplicate.consistent==1 && rescuedDuplicate.unverified==1);
        duplicate.alternatives[0].originalDaycareStageIndependentlyVerified=false;
        assert(A::evaluate(duplicate).status==A::Status::MissingOrDuplicateRouteIdentity);
        auto missingID=x;missingID.alternatives[0].routeIdentity=0;
        assert(A::evaluate(missingID).status==A::Status::MissingOrDuplicateRouteIdentity);
        missingID.count=2;missingID.alternatives[1]=x.alternatives[0];
        missingID.alternatives[1].routeIdentity=2;
        assert(A::evaluate(missingID).status==A::Status::OneConditionallyConsistentStage);
        auto tooMany=x;tooMany.count=17;
        assert(A::evaluate(tooMany).status==A::Status::CapacityExceededUnknown);
        auto empty=x;empty.count=0;
        assert(A::evaluate(empty).status==A::Status::NoCandidates);
    }
    auto out=candidate("ruby_gba",0);out.game="platinum_nds";
    assert(A::evaluate(out).status==A::Status::UnsupportedGame);
    out.game="heartgold_nds";
    assert(A::evaluate(out).status==A::Status::UnsupportedGame);
    std::cout<<"L2 original Gen3 IV-stage historical alternatives: "
             <<positives<<" seeded x 5-game cases plus bounded negative "
                "and Unknown controls passed\n";
}
