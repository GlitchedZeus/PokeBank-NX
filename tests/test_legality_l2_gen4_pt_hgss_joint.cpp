#include "Legality/Gen4PlatinumHGSSJointEggHistories.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace J=Legality::Gen4PlatinumHGSSJointEggHistories;
namespace P=Legality::Gen4PlatinumHGSSOriginalEggPIDHistory;
namespace IV=Legality::Gen4EggIVInheritanceStage;
constexpr std::array<uint8_t,6> childInitial{0,3,6,9,12,15};
constexpr std::array<uint8_t,6> left{7,12,17,22,27,0};
constexpr std::array<uint8_t,6> right{13,24,3,14,25,4};
constexpr std::array<uint32_t,1> oneMT{32};
constexpr std::array<uint32_t,1> otherMT{57};
constexpr std::array<uint16_t,8> itemOptions{0,294,289,290,291,292,293,229};
J::Input source(const char* game,uint32_t seed,uint16_t a,uint16_t b){
    J::Input in{};
    in.game=game;
    in.originalEggEventIdentity=0xD00D;
    in.originalUnhatchedEggIndependentlyVerified=true;
    in.originalEggPID=32;
    in.allOrdinaryDaycareAlternativesIndependentlyEnumerated=true;
    in.count=1;
    auto& route=in.alternatives[0];
    route.routeIdentity=0xB000;
    route.originalDaycarePairIdentity=0xCAFE;
    route.originalSourceHistoryIndependentlyAttested=true;
    route.fullyAttestedForCandidateLocalExclusion=true;
    route.originalHeldItemsSameAcrossStagesIndependentlyVerified=true;
    route.originalIVParentIdentityIndependentlyVerified=true;
    route.ivPhysicalSlot0SnapshotId=101;route.ivPhysicalSlot1SnapshotId=202;
    route.originalIVEventToken=in.originalEggEventIdentity;
    auto& pid=route.pid;
    pid.game=game;
    pid.originalEggEventId=in.originalEggEventIdentity;
    pid.originalEggIdentityIndependentlyVerified=true;
    pid.originalUnhatchedEggPID=in.originalEggPID;
    pid.ordinaryPairEligibilityIndependentlyVerified=true;
    pid.eligibleOrdinaryPair=true;
    pid.physicalDepositOrderIndependentlyVerified=true;
    pid.parents[0]={101,25,a,1,7,true,true,true};
    pid.parents[1]={202,25,b,0,11,true,true,true};
    pid.parentStageToken=in.originalEggEventIdentity;
    pid.pendingStageToken=in.originalEggEventIdentity;
    pid.collectionStageToken=in.originalEggEventIdentity;
    pid.creationAndCollectionStagesIndependentlyVerified=true;
    pid.lcrngEntryIndependentlyVerified=true;
    pid.lcrngEntryState=seed;
    pid.consecutiveMTTranscriptIndependentlyVerified=true;
    pid.exactMTConsumptionIndependentlyVerified=true;
    pid.consecutiveMT=oneMT;
    pid.parentLanguagesAtCollectionIndependentlyVerified=true;
    pid.originalParentLanguages={2,2};
    pid.collectingTrainerFullOTIndependentlyVerified=true;
    pid.collectingTrainerFullOT=0;
    auto& iv=route.iv;
    iv.game=game;
    iv.initialChild=childInitial;
    iv.physicalSlot0=left;
    iv.physicalSlot1=right;
    iv.heldItem0=a;iv.heldItem1=b;
    const auto replay=IV::replay(game,seed,childInitial,left,right,a,b);
    assert(replay.status==IV::Status::ReplayedStage);
    iv.finalSeed=replay.finalSeed;
    iv.observedChild=replay.childIVs;
    in.originalEggIVs=replay.childIVs;
    iv.parentSnapshotsIndependentlyVerified=true;
    iv.depositOrderIndependentlyVerified=true;
    iv.initialChildIVsIndependentlyVerified=true;
    iv.heldItemHistoryIndependentlyVerified=true;
    iv.finalRNGStateIndependentlyVerified=true;
    return in;
}
int main(){
    uint32_t seeds=0,controls=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"}){
        for(uint32_t seed=0;seed<256;++seed){
            for(uint16_t a:itemOptions)for(uint16_t b:itemOptions){
                // Stone on male alone: Pt must not inherit his nature;
                // if on female HGSS or Pt, fixed MT 32 matches her nature.
                // HGSS dual stone with male donor requires a different MT
                // history; retain explicitly modeled Unknown when necessary.
                auto good=source(game,seed,a,b);
                const auto native=J::evaluate(good);
                assert(native.status==J::Status::OneConditionallyConsistentJointHistory ||
                       native.status==J::Status::UnresolvedSourceHistory);
                if(native.status==J::Status::OneConditionallyConsistentJointHistory){
                    assert(native.matched==1 && native.unknown==0);
                    auto wrongItems=good;
                    wrongItems.alternatives[0].iv.heldItem0^=1u;
                    assert(J::evaluate(wrongItems).status==J::Status::UnverifiedOrDuplicateHistory);
                    auto incomplete=good;
                    incomplete.alternatives[0].originalHeldItemsSameAcrossStagesIndependentlyVerified=false;
                    assert(J::evaluate(incomplete).status==J::Status::UnverifiedOrDuplicateHistory);
                }
                ++seeds;
            }
        }
        auto x=source(game,0,0,0);
        assert(J::evaluate(x).status==J::Status::OneConditionallyConsistentJointHistory);
        auto c=x;
        c.count=2;
        c.alternatives[1]=c.alternatives[0];
        c.alternatives[1].routeIdentity=0xB001;
        // A is PID-matched and IV-contradicted; B is IV-matched and
        // PID-contradicted. Do NOT combine the two to invent an egg.
        c.alternatives[0].iv.initialChild.fill(31);
        c.alternatives[0].iv.physicalSlot0.fill(31);
        c.alternatives[0].iv.physicalSlot1.fill(31);
        c.alternatives[1].pid.consecutiveMT=otherMT;
        const auto split=J::evaluate(c);
        assert(split.status==J::Status::AllEnumeratedLocalDaycareCandidatesContradicted);
        assert(split.matched==0 && split.contradicted==2);
        c.allOrdinaryDaycareAlternativesIndependentlyEnumerated=false;
        assert(J::evaluate(c).status==J::Status::UnknownExhaustiveEnumeration);
        // Once an attested same-parent match is present, its position
        // cannot be hidden by contradictory sibling or duplicate ID.
        c.alternatives[1]=x.alternatives[0];
        c.alternatives[1].routeIdentity=0xB001;
        assert(J::evaluate(c).status==J::Status::OneConditionallyConsistentJointHistory);
        c.alternatives[1].routeIdentity=c.alternatives[0].routeIdentity;
        assert(J::evaluate(c).status==J::Status::OneConditionallyConsistentJointHistory);
        const auto matchFirst=c.alternatives[1];
        c.alternatives[1]=c.alternatives[0];c.alternatives[0]=matchFirst;
        assert(J::evaluate(c).status==J::Status::OneConditionallyConsistentJointHistory);
        // A completely duplicated contradiction does not establish
        // exhaustive proof that no original daycare history existed.
        c.alternatives[0]=c.alternatives[1];
        assert(J::evaluate(c).status==J::Status::UnverifiedOrDuplicateHistory);
        auto unknown=x;unknown.alternatives[0].pid.parents[0].creationItemIndependentlyVerified=false;
        assert(J::evaluate(unknown).status==J::Status::UnresolvedSourceHistory);
        unknown=x;unknown.alternatives[0].ivPhysicalSlot0SnapshotId++;
        assert(J::evaluate(unknown).status==J::Status::UnverifiedOrDuplicateHistory);
        unknown=x;unknown.alternatives[0].originalIVEventToken++;
        assert(J::evaluate(unknown).status==J::Status::UnverifiedOrDuplicateHistory);
        unknown=x;unknown.alternatives[0].originalIVParentIdentityIndependentlyVerified=false;
        assert(J::evaluate(unknown).status==J::Status::UnverifiedOrDuplicateHistory);
        unknown=x;unknown.alternatives[0].iv.finalRNGStateIndependentlyVerified=false;
        assert(J::evaluate(unknown).status==J::Status::UnresolvedSourceHistory);
        unknown=x;unknown.alternatives[0].routeIdentity=0;
        assert(J::evaluate(unknown).status==J::Status::UnverifiedOrDuplicateHistory);
        unknown=x;unknown.alternatives[0].pid.collectionStageToken++;
        assert(J::evaluate(unknown).status==J::Status::UnverifiedOrDuplicateHistory);
        unknown=x;unknown.originalUnhatchedEggIndependentlyVerified=false;
        assert(J::evaluate(unknown).status==J::Status::UnknownOriginalEgg);
        unknown=x;unknown.count=0;
        assert(J::evaluate(unknown).status==J::Status::NoCandidates);
        unknown=x;unknown.count=9;
        assert(J::evaluate(unknown).status==J::Status::CapacityExceededUnknown);
        controls+=14;
    }
    auto other=source("platinum_nds",0,0,0);other.game="diamond_nds";
    assert(J::evaluate(other).status==J::Status::UnsupportedGame);
    std::cout<<"L2 Pt/HGSS same-route PID+IV Power-item hypotheses "<<seeds
             <<" item/seed combinations and "<<controls<<" controls PASS\n";
}
