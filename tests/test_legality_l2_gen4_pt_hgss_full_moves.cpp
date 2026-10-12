#include "Legality/Gen4PlatinumHGSSFullEggHistories.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace F=Legality::Gen4PlatinumHGSSFullEggHistories;
namespace M=Legality::Gen4PlatinumHGSSOriginalEggMoves;
namespace J=Legality::Gen4PlatinumHGSSJointEggHistories;
namespace IV=Legality::Gen4EggIVInheritanceStage;
using A=std::array<uint8_t,6>;
constexpr A starting{0,3,6,9,12,15};
constexpr A left{7,12,17,22,27,0};
constexpr A right{13,24,3,14,25,4};
constexpr std::array<uint16_t,8> items{0,294,289,290,291,292,293,236};
constexpr std::array<uint32_t,1> mt{32},wrongMT{57};
F::Input source(const char* game,uint32_t seed,uint16_t item0,uint16_t item1){
    F::Input out{};
    auto& joint=out.original;
    joint.game=game;joint.originalEggEventIdentity=0xACE;
    joint.originalUnhatchedEggIndependentlyVerified=true;
    joint.originalEggPID=32;
    joint.count=1;
    joint.allOrdinaryDaycareAlternativesIndependentlyEnumerated=true;
    auto& route=joint.alternatives[0];
    route.routeIdentity=0x101;
    route.originalDaycarePairIdentity=0xCAFE;
    route.originalSourceHistoryIndependentlyAttested=true;
    route.fullyAttestedForCandidateLocalExclusion=true;
    route.originalHeldItemsSameAcrossStagesIndependentlyVerified=true;
    route.originalIVParentIdentityIndependentlyVerified=true;
    route.ivPhysicalSlot0SnapshotId=101;
    route.ivPhysicalSlot1SnapshotId=202;
    route.originalIVEventToken=0xACE;
    auto& pid=route.pid;
    pid.game=game;pid.originalEggEventId=0xACE;
    pid.originalEggIdentityIndependentlyVerified=true;
    pid.originalUnhatchedEggPID=32;
    pid.ordinaryPairEligibilityIndependentlyVerified=true;
    pid.eligibleOrdinaryPair=true;
    pid.physicalDepositOrderIndependentlyVerified=true;
    pid.parents[0]={101,25,item0,1,7,true,true,true};
    pid.parents[1]={202,25,item1,0,11,true,true,true};
    pid.parentStageToken=0xACE;
    pid.pendingStageToken=0xACE;
    pid.collectionStageToken=0xACE;
    pid.creationAndCollectionStagesIndependentlyVerified=true;
    pid.lcrngEntryIndependentlyVerified=true;
    pid.lcrngEntryState=seed;
    pid.consecutiveMTTranscriptIndependentlyVerified=true;
    pid.exactMTConsumptionIndependentlyVerified=true;
    pid.consecutiveMT=mt;
    pid.parentLanguagesAtCollectionIndependentlyVerified=true;
    pid.originalParentLanguages={2,2};
    pid.collectingTrainerFullOTIndependentlyVerified=true;
    pid.collectingTrainerFullOT=0;
    auto& iv=route.iv;
    iv.game=game;iv.initialChild=starting;
    iv.physicalSlot0=left;iv.physicalSlot1=right;
    iv.heldItem0=item0;iv.heldItem1=item1;
    const auto replay=IV::replay(game,seed,starting,left,right,item0,item1);
    assert(replay.status==IV::Status::ReplayedStage);
    iv.observedChild=replay.childIVs;
    joint.originalEggIVs=replay.childIVs;
    iv.finalSeed=replay.finalSeed;
    iv.parentSnapshotsIndependentlyVerified=true;
    iv.depositOrderIndependentlyVerified=true;
    iv.initialChildIVsIndependentlyVerified=true;
    iv.heldItemHistoryIndependentlyVerified=true;
    iv.finalRNGStateIndependentlyVerified=true;
    auto& m=out.moveStages[0];
    m.game=game;m.originalEggEventIdentity=0xACE;
    m.originalParentStageIdentity=0xACE;
    m.originalPhysicalPairIdentity=0xCAFE;
    m.independentlyAttestedSharedSourceEvent=true;
    m.independentlyAttestedPhysicalDepositOrder=true;
    m.independentlyAttestedDaycareEligibility=true;
    m.eligibleOrdinaryDaycarePair=true;
    m.physicalSlots[0]={101,25,1,item0,{33,45,98,57},true,true,true};
    m.physicalSlots[1]={202,25,0,item1,{33,45,98,57},true,true,true};
    m.moveFatherPhysicalSlot=1;
    m.moveFatherRoleIndependentlyAttested=true;
    m.originalEggSpecies=25;
    m.originalEggSpeciesOriginIndependentlyAttested=true;
    m.originalUnhatchedEggMovesIndependentlyAttested=true;
    m.originalUnhatchedEggMoves={33,45,98,57};
    m.originalBaseMoves={33,45,0,0};
    m.originalFatherEggMoves={98,0,0,0};
    m.originalFatherMachineMoves={57,0,0,0};
    m.originalSharedLevelMoves={33,0,0,0};
    m.baseLearnsetAtOriginalStageIndependentlyAttested=true;
    m.originalEggMoveCompatibilityIndependentlyAttested=true;
    m.originalTMHMCompatibilityIndependentlyAttested=true;
    m.originalSharedLearnsetIndependentlyAttested=true;
    out.originalEggSpecies=25;
    out.originalUnhatchedMovesIndependentlyAttested=true;
    out.originalEggMoves={33,45,98,57};
    out.allJointOrdinaryDaycareRoutesIndependentlyEnumerated=true;
    return out;
}
int main(){
    uint32_t positive=0,negative=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"}){
        for(uint32_t seed=0;seed<64;++seed){
            for(uint16_t a:items)for(uint16_t b:items){
                auto s=source(game,seed,a,b);
                const auto result=F::evaluate(s);
                assert(result.status==F::Status::OneConditionallyConsistentFullHistory);
                assert(result.matched==1 && result.contradicted==0 && result.unknown==0);
                assert(result.moveChecks[0]==M::Status::ConditionalMoveHistoryMatched);
                auto x=s;
                x.moveStages[0].originalUnhatchedEggMoves[0]^=1;
                assert(F::evaluate(x).status==F::Status::UnverifiedCandidateProvenance);
                x=s;x.moveStages[0].originalSharedLevelMoves={99,0,0,0};
                assert(F::evaluate(x).status==F::Status::UnresolvedCandidateHistory);
                x=s;x.moveStages[0].originalFatherMachineMoves={33,0,0,0};
                assert(F::evaluate(x).status==F::Status::AllAttestedOrdinaryDaycareRoutesContradicted);
                x=s;x.moveStages[0].originalPhysicalPairIdentity++;
                assert(F::evaluate(x).status==F::Status::UnverifiedCandidateProvenance);
                x=s;x.moveStages[0].physicalSlots[1].snapshotIdentity++;
                assert(F::evaluate(x).status==F::Status::UnverifiedCandidateProvenance);
                x=s;x.moveStages[0].physicalSlots[1].originalHeldItem=0xEEEE;
                assert(F::evaluate(x).status==F::Status::UnverifiedCandidateProvenance);
                x=s;x.moveStages[0].originalTMHMCompatibilityIndependentlyAttested=false;
                assert(F::evaluate(x).status==F::Status::UnresolvedCandidateHistory);
                x=s;x.moveStages[0].originalEggSpeciesOriginIndependentlyAttested=false;
                assert(F::evaluate(x).status==F::Status::UnresolvedCandidateHistory);
                x=s;x.moveStages[0].independentlyAttestedDaycareEligibility=false;
                assert(F::evaluate(x).status==F::Status::UnresolvedCandidateHistory);
                x=s;x.moveStages[0].originalParentStageIdentity++;
                assert(F::evaluate(x).status==F::Status::UnverifiedCandidateProvenance);
                x=s;x.moveStages[0].moveFatherPhysicalSlot=0;
                assert(F::evaluate(x).status==F::Status::AllAttestedOrdinaryDaycareRoutesContradicted);
                ++positive;negative+=11;
            }
        }
        auto s=source(game,0,0,0);
        // A matches PID+IV but has impossible four moves; B matches
        // four moves and IV but the original pending PID is contradicted.
        // Combining the candidate fragments is forbidden.
        s.original.count=2;
        s.original.alternatives[1]=s.original.alternatives[0];
        s.original.alternatives[1].routeIdentity=0x102;
        s.moveStages[1]=s.moveStages[0];
        s.moveStages[0].originalFatherMachineMoves={33,0,0,0};
        s.original.alternatives[1].pid.consecutiveMT=wrongMT;
        auto split=F::evaluate(s);
        assert(split.status==F::Status::AllAttestedOrdinaryDaycareRoutesContradicted);
        assert(split.matched==0 && split.contradicted==2);
        s.allJointOrdinaryDaycareRoutesIndependentlyEnumerated=false;
        assert(F::evaluate(s).status==F::Status::UnknownOrdinaryDaycareEnumeration);
        // A later same-parent matched route survives earlier contradiction.
        s.original.alternatives[1]=s.original.alternatives[0];
        s.original.alternatives[1].routeIdentity=0x102;
        assert(F::evaluate(s).status==F::Status::OneConditionallyConsistentFullHistory);
        // Duplicate archive identity with valid later route is not erased.
        s.original.alternatives[1].routeIdentity=0x101;
        assert(F::evaluate(s).status==F::Status::OneConditionallyConsistentFullHistory);
        // Reversing those two routes also preserves a conditional match.
        const auto tmp=s.original.alternatives[0];
        s.original.alternatives[0]=s.original.alternatives[1];
        s.original.alternatives[1]=tmp;
        auto moveTmp=s.moveStages[0];s.moveStages[0]=s.moveStages[1];s.moveStages[1]=moveTmp;
        assert(F::evaluate(s).status==F::Status::OneConditionallyConsistentFullHistory);
        s.original.alternatives[0]=s.original.alternatives[1];
        s.moveStages[0]=s.moveStages[1];
        assert(F::evaluate(s).status==F::Status::DuplicateOrMissingRouteIdentity);
        auto v=source(game,0,0,0);
        v.original.alternatives[0].routeIdentity=0;
        assert(F::evaluate(v).status==F::Status::DuplicateOrMissingRouteIdentity);
        v=source(game,0,0,0);v.original.count=0;
        assert(F::evaluate(v).status==F::Status::NoCandidates);
        v=source(game,0,0,0);v.original.count=9;
        assert(F::evaluate(v).status==F::Status::CapacityExceededUnknown);
        v=source(game,0,0,0);v.originalUnhatchedMovesIndependentlyAttested=false;
        assert(F::evaluate(v).status==F::Status::UnknownOriginalEgg);
        v=source(game,0,0,0);v.original.alternatives[0].originalIVEventToken++;
        assert(F::evaluate(v).status==F::Status::UnverifiedCandidateProvenance);
        v=source(game,0,0,0);v.original.alternatives[0].pid.parents[0].originalSnapshotIndependentlyVerified=false;
        assert(F::evaluate(v).status==F::Status::UnresolvedCandidateHistory);
        // Real original Pt/HGSS egg move history can have Ditto in either
        // physical slot, with its SOURCE move-father role different.
        v=source(game,0,0,0);
        v.original.alternatives[0].pid.parents[0].species=132;
        v.original.alternatives[0].pid.parents[0].gender=2;
        v.moveStages[0].physicalSlots[0].species=132;
        v.moveStages[0].physicalSlots[0].gender=2;
        assert(F::evaluate(v).status==F::Status::OneConditionallyConsistentFullHistory);
        v=source(game,0,0,0);
        v.original.alternatives[0].pid.parents[1].species=132;
        v.original.alternatives[0].pid.parents[1].gender=2;
        v.moveStages[0].physicalSlots[1].species=132;
        v.moveStages[0].physicalSlots[1].gender=2;
        v.moveStages[0].moveFatherPhysicalSlot=1;
        v.moveStages[0].originalFatherEggMoves={0,0,0,0};
        v.moveStages[0].originalFatherMachineMoves={0,0,0,0};
        v.moveStages[0].originalUnhatchedEggMoves={33,45,0,0};
        v.originalEggMoves=v.moveStages[0].originalUnhatchedEggMoves;
        assert(F::evaluate(v).status==F::Status::OneConditionallyConsistentFullHistory);
        // The source breaks paternal egg and shared-level passes at the
        // first empty father slot, while machine compatibility scans all.
        v=source(game,0,0,0);
        v.moveStages[0].physicalSlots[1].originalMoves={33,0,98,57};
        assert(F::evaluate(v).status==F::Status::UnresolvedCandidateHistory);
        v.moveStages[0].originalFatherEggMoves={0,0,0,0};
        v.moveStages[0].originalSharedLevelMoves={0,0,0,0};
        v.moveStages[0].originalUnhatchedEggMoves={33,45,57,0};
        v.originalEggMoves=v.moveStages[0].originalUnhatchedEggMoves;
        assert(F::evaluate(v).status==F::Status::OneConditionallyConsistentFullHistory);
        v=source(game,0,0,0);
        v.moveStages[0].originalFatherEggMoves={57,98,0,0};
        assert(F::evaluate(v).status==F::Status::UnresolvedCandidateHistory);
        // Source collection-time Light Ball is not derived from a held
        // item in a later decrypted PK4. Child Pichu requires original
        // held-item attestations for BOTH original physical slots.
        v=source(game,0,236,0);
        v.originalEggSpecies=172;
        v.moveStages[0].originalEggSpecies=172;
        v.originalEggMoves={45,98,57,344};
        v.moveStages[0].originalUnhatchedEggMoves=v.originalEggMoves;
        assert(F::evaluate(v).status==F::Status::OneConditionallyConsistentFullHistory);
        v.moveStages[0].physicalSlots[1].heldItemAtCreationIndependentlyAttested=false;
        assert(F::evaluate(v).status==F::Status::UnresolvedCandidateHistory);
        v=source(game,0,0,0);
        v.originalEggSpecies=172;
        v.moveStages[0].originalEggSpecies=172;
        v.originalEggMoves={45,98,57,344};
        v.moveStages[0].originalUnhatchedEggMoves=v.originalEggMoves;
        assert(F::evaluate(v).status==F::Status::AllAttestedOrdinaryDaycareRoutesContradicted);
        negative+=19;
    }
    auto unsupported=source("platinum_nds",0,0,0);
    unsupported.original.game="diamond_nds";
    assert(F::evaluate(unsupported).status==F::Status::UnsupportedGame);
    std::cout<<"L2 original Pt/HGSS same-parent PID+IV+4-move histories: "<<positive
             <<" positive seeds and "<<negative<<" scoped negative controls PASS\n";
}
