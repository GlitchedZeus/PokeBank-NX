#include "Legality/Gen3JointEggPIDIVHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace J=Legality::Gen3JointEggPIDIVHistory;
namespace P=Legality::Gen3EggPIDCreationHistory;
namespace V=Legality::Gen3EggIVInheritanceStage;
constexpr std::array<uint8_t,6> before{0,3,6,9,12,15};
constexpr std::array<uint8_t,6> mother{7,12,17,22,27,0};
constexpr std::array<uint8_t,6> father{13,24,3,14,25,4};
J::Input witness(const char* game,uint32_t seed) {
    J::Input x{};
    auto& p=x.pid;
    p.game=game;
    p.originalUnhatchedEggIndependentlyVerified=true;
    p.ordinaryDaycarePairIndependentlyVerified=true;
    p.ordinaryDaycarePairEligible=true;
    p.originalPhysicalDepositOrderIndependentlyVerified=true;
    p.physicalSlots[0]={10,25,1,7,false,true,true};
    p.physicalSlots[1]={20,25,0,11,false,true,true};
    p.originalCreationStageIndependentlyVerified=true;
    p.originalCollectionStageIndependentlyVerified=true;
    p.originalParentStageIdentity=0x88114422u;
    p.originalCreationStageIdentity=0x88114422u;
    p.originalCollectionStageIdentity=0x88114422u;
    p.pendingLCRNGStateIndependentlyVerified=true;
    p.pendingLCRNGState=seed;
    p.collectionLCRNGStateIndependentlyVerified=true;
    p.collectionLCRNGState=seed^0x13572468u;
    p.originalEmeraldVBlankCounterIndependentlyVerified=true;
    p.originalEmeraldVBlankCounter=static_cast<uint16_t>(seed);
    const auto pending=P::evaluate(p);
    assert(pending.status==P::Status::OriginalEggPIDContradicted ||
           pending.status==P::Status::ConditionallyMatchedOriginalEggPID);
    p.originalUnhatchedEggPID=pending.finalPersonality;
    const auto iv=V::replay(game,seed,before,mother,father,true);
    assert(iv.status==V::Status::ReplayedStage);
    x.originalPIDAndIVSameEggIndependentlyVerified=true;
    x.originalEggIVsIndependentlyVerified=true;
    x.originalUnhatchedEggIVs=iv.childIVs;
    x.sameOriginalParentSnapshotsForPIDAndIVIndependentlyVerified=true;
    x.ivPhysicalSlot0SnapshotIdentity=10;
    x.ivPhysicalSlot1SnapshotIdentity=20;
    x.originalEggEventIdentity=0x88114422u;
    x.originalIVParentStageIdentityIndependentlyVerified=true;
    x.originalIVParentStageIdentity=0x88114422u;
    x.originalPreInheritanceChildIVsIndependentlyVerified=true;
    x.originalPreInheritanceChildIVs=before;
    x.originalParentIVsIndependentlyVerified=true;
    x.ivPhysicalSlot0=mother;x.ivPhysicalSlot1=father;
    x.finalIVLCRNGStateIndependentlyVerified=true;
    x.finalIVLCRNGState=iv.finalSeed;
    return x;
}
int main() {
    unsigned seeded=0,controls=0;
    for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba",
                          "firered_gba","leafgreen_gba"}) {
        for(uint32_t seed=0;seed<1024;++seed){
            const auto x=witness(game,seed);
            const auto good=J::evaluate(x);
            assert(good.status==J::Status::BothOriginalStagesConditionallyMatched);
            assert(good.pid.status==P::Status::ConditionallyMatchedOriginalEggPID);
            assert(good.iv==V::CandidateCheck::Matched);
            auto wrongPID=x;wrongPID.pid.originalUnhatchedEggPID^=0x1u;
            assert(J::evaluate(wrongPID).status==J::Status::PIDCandidateContradicted);
            auto wrongIV=x;
            for(uint8_t i=0;i<6;++i){
                wrongIV.ivPhysicalSlot0[i]=31;
                wrongIV.ivPhysicalSlot1[i]=31;
                wrongIV.originalPreInheritanceChildIVs[i]=31;
                wrongIV.originalUnhatchedEggIVs[i]=0;
            }
            assert(J::evaluate(wrongIV).status==J::Status::IVCandidateContradicted);
            // A different hypothetical parent pair or missing source
            // event cannot be rescued with a matching IV/PID value.
            auto swap=x;swap.ivPhysicalSlot0SnapshotIdentity=20;
            swap.ivPhysicalSlot1SnapshotIdentity=10;
            assert(J::evaluate(swap).status==J::Status::IncoherentOriginalParentSnapshots);
            auto missing=x;missing.sameOriginalParentSnapshotsForPIDAndIVIndependentlyVerified=false;
            assert(J::evaluate(missing).status==J::Status::IncoherentOriginalParentSnapshots);
            ++seeded;
        }
        auto x=witness(game,42);
        x.originalPIDAndIVSameEggIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownSameOriginalEgg);
        x=witness(game,42);x.originalEggIVsIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownSameOriginalEgg);
        x=witness(game,42);x.originalIVParentStageIdentityIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownSharedOriginalDaycareEvent);
        x=witness(game,42);x.originalIVParentStageIdentity++;
        assert(J::evaluate(x).status==J::Status::IncoherentOriginalDaycareEvent);
        x=witness(game,42);x.originalPreInheritanceChildIVsIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalIVHistory);
        x=witness(game,42);x.finalIVLCRNGStateIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalIVHistory);
        x=witness(game,42);x.pid.pendingLCRNGStateIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalPIDHistory);
        x=witness(game,42);x.ivPhysicalSlot1[2]=32;
        assert(J::evaluate(x).status==J::Status::MalformedOriginalIVCandidate);
        x=witness(game,42);x.pid.physicalSlots[0].originalSnapshotIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalPIDHistory);
        x=witness(game,42);x.originalParentIVsIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalIVHistory);
        controls+=10;
    }
    auto bad=witness("ruby_gba",7);bad.pid.game="platinum_nds";
    assert(J::evaluate(bad).status==J::Status::UnsupportedGame);
    std::cout<<"L2 B07 source game joint PID/IV: "<<seeded
             <<" synthetic full histories and "<<controls
             <<" additional Unknown/malformed controls PASS\n";
}
