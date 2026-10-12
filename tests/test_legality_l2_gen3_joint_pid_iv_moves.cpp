#include "Legality/Gen3JointEggPIDIVMoveHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace J=Legality::Gen3JointEggPIDIVMoveHistory;
namespace P=Legality::Gen3EggPIDCreationHistory;
namespace V=Legality::Gen3EggIVInheritanceStage;
namespace M=Legality::Gen34SameParentEggHistory;
namespace MV=Legality::Gen3JointEggCandidateEvidence;
constexpr std::array<uint8_t,6> initial{0,3,6,9,12,15};
constexpr std::array<uint8_t,6> dadIVs{7,12,17,22,27,0};
constexpr std::array<uint8_t,6> momIVs{13,24,3,14,25,4};
constexpr uint64_t event=0xC0052026u;
J::Input known(const char* game,uint32_t seed,bool fatherFirst) {
    J::Input x{};
    x.originalEggEventIdentity=event;
    x.sameOriginalEggAndEventIndependentlyVerified=true;
    x.originalEggSpeciesIndependentlyVerified=true;
    x.originalEggSpecies=1; // Bulbasaur
    x.originalUnhatchedEggMovesIndependentlyVerified=true;
    x.moveAndPIDSameParentSnapshotsIndependentlyVerified=true;
    x.moveAndPIDSamePhysicalPairIndependentlyVerified=true;
    auto& p=x.pid;
    p.game=game;
    p.originalUnhatchedEggIndependentlyVerified=true;
    p.ordinaryDaycarePairIndependentlyVerified=true;
    p.ordinaryDaycarePairEligible=true;
    p.originalPhysicalDepositOrderIndependentlyVerified=true;
    P::Parent father{101,1,0,7,false,true,true};
    P::Parent mother{202,1,1,11,false,true,true};
    p.physicalSlots[0]=fatherFirst?father:mother;
    p.physicalSlots[1]=fatherFirst?mother:father;
    p.originalCreationStageIndependentlyVerified=true;
    p.originalCollectionStageIndependentlyVerified=true;
    p.originalParentStageIdentity=event;
    p.originalCreationStageIdentity=event;
    p.originalCollectionStageIdentity=event;
    p.pendingLCRNGStateIndependentlyVerified=true;
    p.pendingLCRNGState=seed;
    p.collectionLCRNGStateIndependentlyVerified=true;
    p.collectionLCRNGState=seed^0x13572468u;
    p.originalEmeraldVBlankCounterIndependentlyVerified=true;
    p.originalEmeraldVBlankCounter=static_cast<uint16_t>(seed);
    p.originalUnhatchedEggPID=P::evaluate(p).finalPersonality;

    auto& mv=x.moveAndIV;
    auto& m=mv.moveHistory;
    m.game=game;m.species=1;m.fatherId=101;m.motherId=202;
    m.fatherSpecies=1;m.motherSpecies=1;
    m.pairProof=M::PairProof::IndependentlyCompatible;
    m.snapshotsKnown=true;
    m.lightBallKnown=true;
    m.base={33,45,0,0};
    m.observed=m.base; // No inherited father moves required for this baseline
    mv.father={101,true,dadIVs};
    mv.mother={202,true,momIVs};
    mv.daycareSlotOrderIndependentlyVerified=true;
    mv.daycareSlot0ParentId=p.physicalSlots[0].snapshotIdentity;
    mv.daycareSlot1ParentId=p.physicalSlots[1].snapshotIdentity;
    mv.sharedParentStageIndependentlyVerified=true;
    mv.moveParentStageToken=event;mv.ivParentStageToken=event;
    mv.independentlyKnownFinalIVStageState=true;
    mv.initialChildIVs=initial;
    const auto& physical0=fatherFirst?dadIVs:momIVs;
    const auto& physical1=fatherFirst?momIVs:dadIVs;
    const auto replay=V::replay(game,seed,initial,physical0,physical1,true);
    assert(replay.status==V::Status::ReplayedStage);
    mv.finalIVStageRNGState=replay.finalSeed;
    mv.observedUnhatchedIVs=replay.childIVs;
    return x;
}
int main() {
    unsigned seeded=0,controls=0;
    for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba",
                          "firered_gba","leafgreen_gba"}) {
        for(uint32_t seed=0;seed<256;++seed) {
            for(bool fatherFirst:{false,true}) {
                auto x=known(game,seed,fatherFirst);
                const auto yes=J::evaluate(x);
                assert(yes.status==J::Status::AllThreeStagesConditionallyMatched);
                assert(yes.moveAndIV.status==MV::Status::KnownCandidateConsistent);
                assert(yes.pid.status==P::Status::ConditionallyMatchedOriginalEggPID);
                auto anotherDad=x;
                anotherDad.pid.physicalSlots[fatherFirst?0:1].snapshotIdentity=303;
                assert(J::evaluate(anotherDad).status==
                       J::Status::IncoherentPhysicalParentPair);
                auto wrongParentRoles=x;
                wrongParentRoles.pid.physicalSlots[fatherFirst?0:1].gender=1;
                assert(J::evaluate(wrongParentRoles).status==
                       J::Status::IncoherentHistoricalDonorRoles);
                auto wrongPid=x;wrongPid.pid.originalUnhatchedEggPID^=1;
                assert(J::evaluate(wrongPid).status==
                       J::Status::OriginalPIDCandidateContradicted);
                auto wrongMoves=x;wrongMoves.moveAndIV.moveHistory.observed={15,33,45,0};
                assert(J::evaluate(wrongMoves).status==
                       J::Status::OriginalMoveCandidateContradicted);
                auto wrongIV=x;
                for(size_t i=0;i<6;++i) {
                    wrongIV.moveAndIV.father.originalIVs[i]=31;
                    wrongIV.moveAndIV.mother.originalIVs[i]=31;
                    wrongIV.moveAndIV.initialChildIVs[i]=31;
                    wrongIV.moveAndIV.observedUnhatchedIVs[i]=0;
                }
                assert(J::evaluate(wrongIV).status==
                       J::Status::OriginalIVCandidateContradicted);
                ++seeded;
            }
        }
        auto x=known(game,42,true);
        x.sameOriginalEggAndEventIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalEggEvidence);
        x=known(game,42,true);x.originalEggSpecies++;
        assert(J::evaluate(x).status==J::Status::IncoherentOriginalEggSpecies);
        x=known(game,42,true);x.originalEggEventIdentity++;
        assert(J::evaluate(x).status==J::Status::IncoherentOriginalDaycareEvent);
        x=known(game,42,true);x.moveAndPIDSamePhysicalPairIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalPairEvidence);
        x=known(game,42,true);x.originalUnhatchedEggMovesIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalMoveEvidence);
        x=known(game,42,true);x.pid.pendingLCRNGStateIndependentlyVerified=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalPIDHistory);
        x=known(game,42,true);x.moveAndIV.independentlyKnownFinalIVStageState=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalIVHistory);
        x=known(game,42,true);x.moveAndIV.moveHistory.snapshotsKnown=false;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalPairEvidence);
        x=known(game,42,true);x.moveAndIV.moveHistory.pairProof=M::PairProof::Unknown;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalPairEvidence);
        x=known(game,42,true);x.moveAndIV.initialChildIVs[0]=32;
        assert(J::evaluate(x).status==J::Status::MalformedOriginalIVCandidate);
        x=known(game,42,true);x.moveAndIV.moveParentStageToken=0;
        assert(J::evaluate(x).status==J::Status::UnknownOriginalDaycareEvent);
        // The source's father role can be non-male in the Ditto case.
        x=known(game,42,true);
        x.pid.physicalSlots[0].species=132;
        x.pid.physicalSlots[0].gender=2;
        x.pid.physicalSlots[1].gender=1;
        x.moveAndIV.moveHistory.fatherSpecies=132;
        assert(J::evaluate(x).status==J::Status::AllThreeStagesConditionallyMatched);
        x.pid.physicalSlots[1].gender=0;
        assert(J::evaluate(x).status==J::Status::IncoherentHistoricalDonorRoles);
        x=known(game,42,false);
        x.pid.physicalSlots[1].species=132;
        x.pid.physicalSlots[1].gender=2;
        x.pid.physicalSlots[0].gender=0;
        x.moveAndIV.moveHistory.fatherSpecies=132;
        assert(J::evaluate(x).status==J::Status::IncoherentHistoricalDonorRoles);
        x=known(game,42,true);
        x.pid.physicalSlots[1].species=132;
        x.pid.physicalSlots[1].gender=2;
        x.pid.physicalSlots[0].gender=0;
        x.moveAndIV.moveHistory.motherSpecies=132;
        assert(J::evaluate(x).status==J::Status::AllThreeStagesConditionallyMatched);
        controls+=15;
    }
    auto bad=known("ruby_gba",0,true);bad.moveAndIV.moveHistory.game="platinum_nds";
    assert(J::evaluate(bad).status==J::Status::UnsupportedGame);
    std::cout<<"L2 B08 triple-source Gen III egg witness: "
             <<seeded<<" seeded parent-order histories + "<<controls
             <<" independent provenance/role controls PASS\n";
}
