#include "Legality/Gen4DiamondPearlEggAlternativeHistories.h"
#include "Legality/Gen4DiamondPearlEggFullJointHistories.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen4DiamondPearlEggAlternativeHistories;
namespace IV=Legality::Gen4DiamondPearlEggIVStage;
namespace PID=Legality::Gen4DiamondPearlEggPidHistory;
namespace J=Legality::Gen4DiamondPearlEggFullJointHistories;
namespace M=Legality::Gen4DiamondPearlEggMoveStage;
constexpr std::array<uint8_t,6> initial{0,3,6,9,12,15};
constexpr std::array<uint8_t,6> parent0{7,12,17,22,27,0};
constexpr std::array<uint8_t,6> parent1{13,24,3,14,25,4};
constexpr std::array<uint32_t,3> mt{0,1,32};
A::Input known(const char* game, uint32_t seed=0){
    A::Input in{};
    in.game=game;
    in.originalEggEventIdentity=0xD1A01234u;
    in.originalUnhatchedEggIdentityIndependentlyVerified=true;
    in.originalEggPIDIndependentlyVerified=true;
    in.originalEggIVsIndependentlyVerified=true;
    in.originalEggPID=32;
    const auto replay=IV::replay(game,seed,initial,parent0,parent1);
    assert(replay.status==IV::Status::Replayed);
    in.originalEggIVs=replay.child;
    in.count=1;
    in.allDaycareStageHypothesesIndependentlyEnumerated=true;
    auto& c=in.alternatives[0];
    c.routeIdentity=1;c.originalEggEventIdentity=in.originalEggEventIdentity;
    c.independentlyAttestedHypothesis=true;
    c.allMethodsForThisCandidateIndependentlyCovered=true;
    c.sharedPhysicalParentPairIndependentlyVerified=true;
    c.ivPhysicalParentPairIdentity=0xBEEFB001u;
    c.pidPhysicalParentPairIdentity=0xBEEFB001u;
    c.iv.game=game;c.iv.initialChild=initial;
    c.iv.physicalSlot0=parent0;c.iv.physicalSlot1=parent1;
    c.iv.originalEggIVs=in.originalEggIVs;
    c.iv.finalSeed=replay.finalSeed;
    c.iv.historicalParentIVsVerified=true;
    c.iv.physicalDepositOrderVerified=true;
    c.iv.preInheritanceChildIVsVerified=true;
    c.iv.originalUnhatchedEggIVsVerified=true;
    c.iv.finalSeedIndependentlyVerified=true;
    c.pid.game=game;
    c.pid.originalDaycarePairEligibilityVerified=true;
    c.pid.eligibleOrdinaryDaycarePair=true;
    c.pid.originalParentSnapshotsVerified=true;
    c.pid.originalDaycareSlotOrderVerified=true;
    c.pid.creationTimeItemsIndependentlyVerified=true;
    c.pid.parents[0]={25,1,7,229,true};
    c.pid.parents[1]={25,0,11,0,true};
    c.pid.creationStageIndependentlyVerified=true;
    c.pid.collectionStageIndependentlyVerified=true;
    c.pid.parentStageToken=in.originalEggEventIdentity;
    c.pid.mtStageToken=in.originalEggEventIdentity;
    c.pid.collectionStageToken=in.originalEggEventIdentity;
    c.pid.creationLCRNGStateIndependentlyVerified=true;
    c.pid.consecutiveMTOutputsIndependentlyVerified=true;
    c.pid.exactMTConsumptionIndependentlyVerified=true;
    c.pid.mtOutputs=mt;
    c.pid.originalParentLanguagesIndependentlyVerified=true;
    c.pid.originalParentLanguages={2,2};
    c.pid.originalCollectionTrainerIdIndependentlyVerified=true;
    c.pid.originalCollectionTrainerId=0;
    c.pid.originalUnhatchedEggPidIndependentlyVerified=true;
    c.pid.originalUnhatchedEggPid=32;
    return in;
}
J::Input joint(const A::Input& original){
    J::Input out{};
    out.original=original;
    out.allJointDaycareHistoriesIndependentlyEnumerated=true;
    auto& m=out.moveHistories[0];
    m.game=original.game;
    m.originalEggEventIdentity=original.originalEggEventIdentity;
    m.originalParentStageIdentity=original.originalEggEventIdentity;
    m.originalPhysicalPairIdentity=
        original.alternatives[0].ivPhysicalParentPairIdentity;
    m.sharedOriginalEventIndependentlyAttested=true;
    m.physicalDepositOrderIndependentlyAttested=true;
    m.physicalSlots[0]={101,25,1,{33,45,98,57},true,true};
    m.physicalSlots[1]={202,25,0,{33,45,98,57},true,true};
    m.fatherPhysicalSlot=1;m.fatherRoleIndependentlyAttested=true;
    m.originalEggSpecies=25;
    m.originalUnhatchedEggIdentityIndependentlyAttested=true;
    m.originalUnhatchedEggMovesIndependentlyAttested=true;
    m.originalUnhatchedEggMoves={33,45,98,57};
    m.base={33,45,0,0};m.paternalEgg={98,0,0,0};
    m.paternalMachine={57,0,0,0};m.mutualLevel={33,0,0,0};
    m.originalBaseMovesIndependentlyAttested=true;
    m.paternalEggMethodIndependentlyAttested=true;
    m.paternalMachineMethodIndependentlyAttested=true;
    m.mutualLevelMethodIndependentlyAttested=true;
    m.originalCreationItemHistoryIndependentlyAttested=true;
    return out;
}
int main(){
    unsigned cases=0;
    for(const char* game:{"diamond_nds","pearl_nds"}){
        // Two independent game tags, several thousand synthetic IV seeds.
        for(uint32_t seed=0;seed<4096;++seed){
            auto x=known(game,seed);
            const auto p=A::evaluate(x);
            assert(p.status==A::Status::OneConditionallyConsistentHistory);
            assert(p.consistent==1 && !p.contradicted && !p.unresolved);
            auto altered=x;
            // Another purported pair with all six historical IVs different
            // from the child must contradict this exact local hypothesis.
            for(size_t i=0;i<6;++i) {
                const auto opposite=static_cast<uint8_t>((x.originalEggIVs[i]+1u)%32u);
                altered.alternatives[0].iv.initialChild[i]=opposite;
                altered.alternatives[0].iv.physicalSlot0[i]=opposite;
                altered.alternatives[0].iv.physicalSlot1[i]=opposite;
            }
            assert(A::evaluate(altered).status==
                A::Status::AllAttestedDaycareStageHypothesesContradicted);
            altered.allDaycareStageHypothesesIndependentlyEnumerated=false;
            assert(A::evaluate(altered).status==A::Status::UnknownHypothesisEnumeration);
            // A distinct attested alternative can rescue a contradicted
            // candidate without changing the observed original egg bytes.
            altered.count=2;
            altered.alternatives[1]=x.alternatives[0];
            altered.alternatives[1].routeIdentity=2;
            assert(A::evaluate(altered).status==A::Status::OneConditionallyConsistentHistory);
            ++cases;
        }
        // Independent source-backed PID/IV replayers and four-slot move
        // constructor must agree on the SAME daycare parent history.
        for(uint32_t seed=0;seed<1024;++seed){
            auto one=joint(known(game,seed));
            const auto yes=J::evaluate(one);
            assert(yes.status==J::Status::OneConditionallyConsistentFullHistory);
            assert(yes.matched==1 && !yes.contradicted && !yes.unknown);
            // Two complementary half-truths must NEVER be joined:
            // candidate A matches PID/IV but not four moves; candidate B
            // matches moves but is IV-contradicted for the original egg.
            auto crossed=one;
            crossed.original.count=2;
            crossed.original.alternatives[1]=crossed.original.alternatives[0];
            crossed.original.alternatives[1].routeIdentity=2;
            crossed.moveHistories[1]=crossed.moveHistories[0];
            crossed.moveHistories[0].originalUnhatchedEggMoves[0]^=1;
            auto& impossible=crossed.original.alternatives[1].iv;
            for(size_t i=0;i<6;++i){
                impossible.initialChild[i]=31;
                impossible.physicalSlot0[i]=31;
                impossible.physicalSlot1[i]=31;
            }
            const auto separate=J::evaluate(crossed);
            assert(separate.status==
                   J::Status::AllEnumeratedLocalDaycareHistoriesContradicted);
            assert(separate.matched==0 && separate.contradicted==2);
            crossed.allJointDaycareHistoriesIndependentlyEnumerated=false;
            assert(J::evaluate(crossed).status==J::Status::UnknownJointEnumeration);
            // Restore a complete, independently attested matching route.
            crossed.moveHistories[1]=one.moveHistories[0];
            crossed.original.alternatives[1]=one.original.alternatives[0];
            crossed.original.alternatives[1].routeIdentity=2;
            assert(J::evaluate(crossed).status==
                   J::Status::OneConditionallyConsistentFullHistory);
            // Matching route in either position survives unrelated noise.
            auto reversed=crossed;
            const auto oldRoute=reversed.original.alternatives[0];
            reversed.original.alternatives[0]=reversed.original.alternatives[1];
            reversed.original.alternatives[1]=oldRoute;
            const auto oldMoves=reversed.moveHistories[0];
            reversed.moveHistories[0]=reversed.moveHistories[1];
            reversed.moveHistories[1]=oldMoves;
            assert(J::evaluate(reversed).status==
                   J::Status::OneConditionallyConsistentFullHistory);
            // Archive collision with a later coherent history survives;
            // duplicated contradictory rows cannot prove exhaustive lack.
            crossed.original.alternatives[1].routeIdentity=1;
            assert(J::evaluate(crossed).status==
                   J::Status::OneConditionallyConsistentFullHistory);
            crossed.moveHistories[1]=crossed.moveHistories[0];
            assert(J::evaluate(crossed).status==
                   J::Status::UnknownOrContradictoryRouteIdentity);
        }
        auto one=joint(known(game));
        auto wrongPair=one;
        wrongPair.moveHistories[0].originalPhysicalPairIdentity++;
        assert(J::evaluate(wrongPair).status==J::Status::UnverifiedOriginalParentSources);
        auto wrongGender=one;
        wrongGender.moveHistories[0].physicalSlots[1].gender=1;
        assert(J::evaluate(wrongGender).status==J::Status::UnverifiedOriginalParentSources);
        auto pichu=one;
        pichu.moveHistories[0].originalEggSpecies=172;
        pichu.moveHistories[0].originalUnhatchedEggMoves={45,98,57,344};
        pichu.moveHistories[0].originalEligibleLightBallHolder=true;
        pichu.original.alternatives[0].pid.parents[1].heldItem=236;
        assert(J::evaluate(pichu).status==
               J::Status::OneConditionallyConsistentFullHistory);
        pichu.original.alternatives[0].pid.parents[1].heldItem=0;
        assert(J::evaluate(pichu).status==J::Status::UnverifiedOriginalParentSources);
        one.original.count=0;
        assert(J::evaluate(one).status==J::Status::NoCandidates);
        one=joint(known(game));one.original.count=9;
        assert(J::evaluate(one).status==J::Status::CapacityExceededUnknown);
        auto x=known(game);
        auto wrongPID=x;wrongPID.alternatives[0].pid.originalCollectionTrainerId=1;
        // Force a distinct bounded MT transcript while preserving child PID.
        constexpr std::array<uint32_t,1> alteredMT{57};
        wrongPID.alternatives[0].pid.mtOutputs=alteredMT;
        assert(A::evaluate(wrongPID).status==
            A::Status::AllAttestedDaycareStageHypothesesContradicted);
        auto incomplete=wrongPID;
        incomplete.alternatives[0].pid.consecutiveMTOutputsIndependentlyVerified=false;
        incomplete.alternatives[0].iv.finalSeed^=0x111u;
        incomplete.alternatives[0].allMethodsForThisCandidateIndependentlyCovered=false;
        assert(A::evaluate(incomplete).status==A::Status::UnresolvedCandidateHistory);
        auto noHistory=x;noHistory.alternatives[0].independentlyAttestedHypothesis=false;
        assert(A::evaluate(noHistory).status==A::Status::UnknownCandidateProvenance);
        auto stageMismatch=x;stageMismatch.alternatives[0].originalEggEventIdentity++;
        assert(A::evaluate(stageMismatch).status==A::Status::UnknownCandidateProvenance);
        auto crossed=x;crossed.alternatives[0].pidPhysicalParentPairIdentity++;
        assert(A::evaluate(crossed).status==A::Status::UnknownCandidateProvenance);
        auto noPairProof=x;noPairProof.alternatives[0].sharedPhysicalParentPairIndependentlyVerified=false;
        assert(A::evaluate(noPairProof).status==A::Status::UnknownCandidateProvenance);
        auto zeroPair=x;zeroPair.alternatives[0].ivPhysicalParentPairIdentity=0;
        assert(A::evaluate(zeroPair).status==A::Status::UnknownCandidateProvenance);
        auto wrongPIDProjection=x;wrongPIDProjection.alternatives[0].pid.originalUnhatchedEggPid=31;
        assert(A::evaluate(wrongPIDProjection).status==A::Status::UnknownCandidateProvenance);
        auto wrongIVProjection=x;wrongIVProjection.alternatives[0].iv.originalEggIVs[0]^=1;
        assert(A::evaluate(wrongIVProjection).status==A::Status::UnknownCandidateProvenance);
        auto dup=x;dup.count=2;dup.alternatives[1]=dup.alternatives[0];
        const auto rescuedDuplicate=A::evaluate(dup);
        assert(rescuedDuplicate.status==A::Status::OneConditionallyConsistentHistory);
        assert(rescuedDuplicate.consistent==1 && rescuedDuplicate.unverified==1);
        auto noId=x;noId.alternatives[0].routeIdentity=0;
        assert(A::evaluate(noId).status==A::Status::MissingOrDuplicateCandidateIdentity);
        // A duplicate with a separately attested matching row is a
        // conditional POSITIVE, independent of row order. It is not
        // MissingOrDuplicateCandidateIdentity simply because its first
        // row lacks attestation (the old CI assertion was contradictory).
        dup.alternatives[0].independentlyAttestedHypothesis=false;
        const auto lateMatch=A::evaluate(dup);
        assert(lateMatch.status==A::Status::OneConditionallyConsistentHistory);
        assert(lateMatch.consistent==1 && lateMatch.unverified>=1);
        auto earlyMatch=dup;
        earlyMatch.alternatives[0]=dup.alternatives[1];
        earlyMatch.alternatives[1]=dup.alternatives[0];
        const auto early=A::evaluate(earlyMatch);
        assert(early.status==A::Status::OneConditionallyConsistentHistory);
        assert(early.consistent==1 && early.unverified>=1);
        // Neither order of two malformed duplicate archive rows proves
        // a daycare contradiction. With no matching witness, stay Unknown.
        auto malformedOnly=dup;
        malformedOnly.alternatives[1]=malformedOnly.alternatives[0];
        const auto noMatch=A::evaluate(malformedOnly);
        assert(noMatch.status==A::Status::MissingOrDuplicateCandidateIdentity);
        assert(noMatch.consistent==0 && noMatch.unverified>=1);
        // An independently attested but contradicted first history cannot
        // hide a later matching history with a colliding route identity.
        auto collision=x;
        collision.count=2;
        collision.alternatives[0]=wrongPID.alternatives[0];
        collision.alternatives[1]=x.alternatives[0];
        const auto contradictedFirst=A::evaluate(collision);
        assert(contradictedFirst.status==A::Status::OneConditionallyConsistentHistory);
        assert(contradictedFirst.consistent==1 && contradictedFirst.unverified>=1);
        collision.alternatives[0]=x.alternatives[0];
        collision.alternatives[1]=wrongPID.alternatives[0];
        const auto contradictedLast=A::evaluate(collision);
        assert(contradictedLast.status==A::Status::OneConditionallyConsistentHistory);
        assert(contradictedLast.consistent==1 && contradictedLast.unverified>=1);
        // Two independently attested but duplicate contradictory witnesses
        // cannot be treated as an exhaustive independent negative universe.
        collision.alternatives[0]=wrongPID.alternatives[0];
        collision.alternatives[1]=wrongPID.alternatives[0];
        const auto negativeDuplicates=A::evaluate(collision);
        assert(negativeDuplicates.status==A::Status::MissingOrDuplicateCandidateIdentity);
        assert(negativeDuplicates.consistent==0 && negativeDuplicates.unverified>=1);
        noId.count=2;noId.alternatives[1]=x.alternatives[0];
        noId.alternatives[1].routeIdentity=2;
        assert(A::evaluate(noId).status==A::Status::OneConditionallyConsistentHistory);
        auto noEgg=x;noEgg.originalUnhatchedEggIdentityIndependentlyVerified=false;
        assert(A::evaluate(noEgg).status==A::Status::UnknownOriginalEgg);
        noEgg=x;noEgg.originalEggIVsIndependentlyVerified=false;
        assert(A::evaluate(noEgg).status==A::Status::UnknownOriginalEgg);
        noEgg=x;noEgg.originalEggPIDIndependentlyVerified=false;
        assert(A::evaluate(noEgg).status==A::Status::UnknownOriginalEgg);
        auto noSeed=x;noSeed.alternatives[0].iv.finalSeedIndependentlyVerified=false;
        assert(A::evaluate(noSeed).status==A::Status::UnresolvedCandidateHistory);
        auto noLang=x;noLang.alternatives[0].pid.originalParentLanguagesIndependentlyVerified=false;
        assert(A::evaluate(noLang).status==A::Status::UnresolvedCandidateHistory);
        auto noParent=x;noParent.alternatives[0].iv.historicalParentIVsVerified=false;
        assert(A::evaluate(noParent).status==A::Status::UnresolvedCandidateHistory);
        auto noStage=x;noStage.alternatives[0].pid.mtStageToken=0;
        assert(A::evaluate(noStage).status==A::Status::UnknownCandidateProvenance);
        auto capacity=x;capacity.count=9;
        assert(A::evaluate(capacity).status==A::Status::CapacityExceededUnknown);
        auto empty=x;empty.count=0;
        assert(A::evaluate(empty).status==A::Status::NoCandidates);
        cases+=20;
    }
    auto unsupported=known("diamond_nds");unsupported.game="platinum_nds";
    assert(A::evaluate(unsupported).status==A::Status::UnsupportedGame);
    unsupported.game="heartgold_nds";
    assert(A::evaluate(unsupported).status==A::Status::UnsupportedGame);
    std::cout << "L2 B18 bounded DP PID+IV candidate histories: " << cases
              << " positive/local negative/Unknown combinations PASS\n";
}
