#include "Legality/Gen4DiamondPearlNativeEggHistory.h"
#include "Encryption/Encryption4.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace N=Legality::Gen4DiamondPearlNativeEggHistory;
namespace IV=Legality::Gen4DiamondPearlEggIVStage;
namespace PID=Legality::Gen4DiamondPearlEggPidHistory;
void w16(std::vector<std::byte>& p,size_t off,uint16_t value) {
    p[off]=static_cast<std::byte>(value&255u);
    p[off+1]=static_cast<std::byte>((value>>8)&255u);
}
void w32(std::vector<std::byte>& p,size_t off,uint32_t value) {
    w16(p,off,static_cast<uint16_t>(value));
    w16(p,off+2,static_cast<uint16_t>(value>>16));
}
std::vector<std::byte> native(uint32_t pid,uint16_t presentItem,
        std::array<uint8_t,6> ivs,bool egg,uint8_t gender,
        bool partyRecord=false,
        std::array<uint16_t,4> moves={}) {
    std::vector<std::byte> p(
        partyRecord?Encryption::SIZE_PARTY4:Encryption::SIZE_STORED4,
        std::byte{0});
    w32(p,0,pid);
    w16(p,8,25); // original valid Pikachu records
    w16(p,10,presentItem);
    uint32_t packed=egg?(1u<<30):0;
    for(unsigned i=0;i<6;++i)packed|=static_cast<uint32_t>(ivs[i])<<(i*5);
    w32(p,0x38,packed);
    for(size_t i=0;i<moves.size();++i)w16(p,0x28+2*i,moves[i]);
    p[0x40]=static_cast<std::byte>(gender<<1);
    p[0x17]=std::byte{2};
    p[0x5F]=std::byte{10}; // Diamond, original-version identity.
    w16(p,6,Encryption::checksum4(p));
    return Encryption::encryptArray4(p);
}
int main() {
    constexpr std::array<uint8_t,6> base{0,3,6,9,12,15};
    constexpr std::array<uint8_t,6> slot0{7,12,17,22,27,0};
    constexpr std::array<uint8_t,6> slot1{13,24,3,14,25,4};
    constexpr std::array<uint32_t,3> mt{0,1,32};
    const auto stage=IV::replay("diamond_nds",0,base,slot0,slot1);
    assert((stage.child==std::array<uint8_t,6>({7,3,6,22,12,0})));
    assert(stage.finalSeed==0xE2CCA5EEu);
    // Present items are deliberately zero. Original Everstone is supplied
    // only as separately attested parent creation-time history.
    auto raw0=native(7,0,slot0,false,1);
    auto raw1=native(11,0,slot1,false,0);
    auto rawEgg=native(32,0,stage.child,true,0);
    const auto copy0=raw0,copy1=raw1,copyEgg=rawEgg;
    Pokemon::Pokemon4ReadOnly p0(raw0,Enums::GameVersion::DP);
    Pokemon::Pokemon4ReadOnly p1(raw1,Enums::GameVersion::DP);
    Pokemon::Pokemon4ReadOnly child(rawEgg,Enums::GameVersion::DP);
    assert(p0.valid()&&p1.valid()&&child.valid());
    assert(!p0.isEgg()&&!p1.isEgg()&&child.isEgg());
    N::Input in{};
    in.game="diamond_nds";
    in.physicalSlot0=&p0;in.physicalSlot1=&p1;in.originalEgg=&child;
    in.verifiedOriginalParentSnapshots=true;
    in.verifiedOriginalPhysicalDaycareOrder=true;
    in.verifiedOriginalUnhatchedEggSnapshot=true;
    in.verifiedContainingGame=true;
    in.independentlyVerifiedSameCreationAndCollectionEvent=true;
    in.parentStageToken=42;in.originalEggStageToken=42;
    in.iv.initialChild=base;
    in.iv.preInheritanceChildIVsVerified=true;
    in.iv.finalSeed=stage.finalSeed;
    in.iv.finalSeedIndependentlyVerified=true;
    in.pid.originalDaycarePairEligibilityVerified=true;
    in.pid.eligibleOrdinaryDaycarePair=true;
    in.pid.creationTimeItemsIndependentlyVerified=true;
    in.pid.parents[0].heldItem=229;in.pid.parents[1].heldItem=0;
    in.pid.creationStageIndependentlyVerified=true;
    in.pid.collectionStageIndependentlyVerified=true;
    in.pid.creationLCRNGStateIndependentlyVerified=true;
    in.pid.creationLCRNGState=0;
    in.pid.consecutiveMTOutputsIndependentlyVerified=true;
    in.pid.exactMTConsumptionIndependentlyVerified=true;
    in.pid.mtOutputs=mt;
    in.pid.originalParentLanguagesIndependentlyVerified=true;
    in.pid.originalParentLanguages={2,2};
    in.pid.originalCollectionTrainerIdIndependentlyVerified=true;
    in.pid.originalCollectionTrainerId=0;
    const auto good=N::evaluate(in);
    assert(good.status==N::Status::BothConditionalStagesMatched);
    assert(good.iv==IV::Check::Matched);
    assert(good.pid.status==PID::Status::ConditionalOriginalEggPIDMatched);
    assert(good.pid.pendingPid==32 && good.pid.originalFinalPid==32);
    assert(good.pid.mtConsumed==3 && good.pid.everstoneDonor==0);
    // Independent original-moves evidence must agree with THE SAME
    // immutable encrypted PK4 parent and egg records as PID and IV.
    // No game-specific original TM compatibility is synthesized here.
    constexpr std::array<uint16_t,4> moves{33,45,98,57};
    auto m0raw=native(7,0,slot0,false,1,false,moves);
    auto m1raw=native(11,0,slot1,false,0,false,moves);
    auto eggMovesRaw=native(32,0,stage.child,true,0,false,moves);
    const auto m0Copy=m0raw,m1Copy=m1raw,eggMoveCopy=eggMovesRaw;
    Pokemon::Pokemon4ReadOnly m0(m0raw,Enums::GameVersion::DP);
    Pokemon::Pokemon4ReadOnly m1(m1raw,Enums::GameVersion::DP);
    Pokemon::Pokemon4ReadOnly eggMoves(eggMovesRaw,Enums::GameVersion::DP);
    assert(m0.moves()==moves && m1.moves()==moves && eggMoves.moves()==moves);
    auto three=in;
    three.physicalSlot0=&m0;three.physicalSlot1=&m1;
    three.originalEgg=&eggMoves;
    three.originalMoveHistorySupplied=true;
    auto& m=three.moves;
    m.game=three.game;
    m.originalEggEventIdentity=three.parentStageToken;
    m.originalParentStageIdentity=three.parentStageToken;
    m.originalPhysicalPairIdentity=0xBEEFB001u;
    m.sharedOriginalEventIndependentlyAttested=true;
    m.physicalDepositOrderIndependentlyAttested=true;
    m.originalDaycarePairEligibilityIndependentlyAttested=true;
    m.eligibleOriginalDaycarePair=true;
    m.physicalSlots[0]={101,25,1,moves,true,true};
    m.physicalSlots[1]={202,25,0,moves,true,true};
    m.fatherPhysicalSlot=1;m.fatherRoleIndependentlyAttested=true;
    m.originalEggSpecies=25;
    m.originalUnhatchedEggIdentityIndependentlyAttested=true;
    m.originalUnhatchedEggMovesIndependentlyAttested=true;
    m.originalUnhatchedEggMoves=moves;
    m.base={33,45,0,0};m.paternalEgg={98,0,0,0};
    m.paternalMachine={57,0,0,0};m.mutualLevel={33,0,0,0};
    m.originalBaseMovesIndependentlyAttested=true;
    m.paternalEggMethodIndependentlyAttested=true;
    m.paternalMachineMethodIndependentlyAttested=true;
    m.mutualLevelMethodIndependentlyAttested=true;
    m.originalCreationItemHistoryIndependentlyAttested=true;
    const auto all=N::evaluate(three);
    assert(all.status==N::Status::AllThreeConditionalStagesMatched);
    assert(all.iv==IV::Check::Matched);
    assert(all.pid.status==PID::Status::ConditionalOriginalEggPIDMatched);
    assert(all.moves.status==
           Legality::Gen4DiamondPearlEggMoveStage::Status::ConditionalMoveHistoryMatched);
    auto partyMovesRaw=native(32,0,stage.child,true,0,true,moves);
    Pokemon::Pokemon4ReadOnly partyMoves(partyMovesRaw,Enums::GameVersion::DP);
    const auto partyCopy=partyMovesRaw;
    auto sameParty=three;sameParty.originalEgg=&partyMoves;
    assert(N::evaluate(sameParty).status==N::Status::AllThreeConditionalStagesMatched);
    assert(partyMovesRaw==partyCopy);
    auto noOriginalMethod=three;
    noOriginalMethod.moves.paternalMachineMethodIndependentlyAttested=false;
    assert(N::evaluate(noOriginalMethod).status==N::Status::StageHistoryUnresolved);
    auto wrongMachine=three;wrongMachine.moves.paternalMachine={33,0,0,0};
    assert(N::evaluate(wrongMachine).status==N::Status::LocalCandidateContradiction);
    auto forgedMachine=three;forgedMachine.moves.paternalMachine={99,0,0,0};
    assert(N::evaluate(forgedMachine).status==N::Status::StageHistoryUnresolved);
    auto missingPair=three;missingPair.moves.originalPhysicalPairIdentity=0;
    assert(N::evaluate(missingPair).status==N::Status::StageHistoryUnresolved);
    auto missingParent=three;
    missingParent.moves.physicalSlots[0].originalSnapshotIndependentlyAttested=false;
    assert(N::evaluate(missingParent).status==N::Status::StageHistoryUnresolved);
    auto crossedEvent=three;crossedEvent.moves.originalParentStageIdentity++;
    assert(N::evaluate(crossedEvent).status==N::Status::IncoherentNativeMoveSnapshots);
    auto swappedFather=three;swappedFather.moves.fatherPhysicalSlot=0;
    assert(N::evaluate(swappedFather).status==N::Status::StageHistoryUnresolved);
    auto crossedMoves=three;crossedMoves.moves.originalUnhatchedEggMoves[0]++;
    assert(N::evaluate(crossedMoves).status==N::Status::IncoherentNativeMoveSnapshots);
    auto crossedParent=three;crossedParent.moves.physicalSlots[1].originalMoves[0]++;
    assert(N::evaluate(crossedParent).status==N::Status::IncoherentNativeMoveSnapshots);
    auto unmodified=three;unmodified.originalMoveHistorySupplied=false;
    assert(N::evaluate(unmodified).status==N::Status::BothConditionalStagesMatched);
    assert(m0raw==m0Copy && m1raw==m1Copy && eggMovesRaw==eggMoveCopy);
    // The native source can be a stored (136-byte) or party (236-byte)
    // encrypted original egg; neither format implies creation-time items.
    auto partyEggRaw=native(32,0,stage.child,true,0,true);
    const auto copyPartyEgg=partyEggRaw;
    Pokemon::Pokemon4ReadOnly partyEgg(partyEggRaw,Enums::GameVersion::DP);
    assert(partyEgg.valid() && partyEgg.isParty() && partyEgg.isEgg());
    auto partyWitness=in;partyWitness.originalEgg=&partyEgg;
    assert(N::evaluate(partyWitness).status==N::Status::BothConditionalStagesMatched);
    assert(partyEggRaw==copyPartyEgg);
    // Two possible original held-item histories can refer to exactly
    // the same encrypted original child. A contradicting Everstone-free
    // transcript rules out only THAT hypothetical original stage.
    auto otherItems=in;otherItems.pid.parents[0].heldItem=0;
    const auto otherResult=N::evaluate(otherItems);
    assert(otherResult.status==N::Status::LocalCandidateContradiction);
    assert(N::evaluate(in).status==N::Status::BothConditionalStagesMatched);
    // Same native original child but a contradictory PROPOSED IV seed.
    auto wrongIV=in;wrongIV.iv.finalSeed^=0x1111u;
    assert(N::evaluate(wrongIV).status==N::Status::LocalCandidateContradiction);
    // Same native child but a different original-stage MT PID transcript.
    constexpr std::array<uint32_t,1> wrongMT{57};
    auto wrongPID=in;wrongPID.pid.mtOutputs=wrongMT;
    assert(N::evaluate(wrongPID).status==N::Status::LocalCandidateContradiction);
    auto noItems=in;noItems.pid.creationTimeItemsIndependentlyVerified=false;
    assert(N::evaluate(noItems).status==N::Status::StageHistoryUnresolved);
    auto noParent=in;noParent.verifiedOriginalParentSnapshots=false;
    assert(N::evaluate(noParent).status==N::Status::UnknownOriginalParentSnapshots);
    auto noOrder=in;noOrder.verifiedOriginalPhysicalDaycareOrder=false;
    assert(N::evaluate(noOrder).status==N::Status::UnknownOriginalParentSnapshots);
    auto noEgg=in;noEgg.verifiedOriginalUnhatchedEggSnapshot=false;
    assert(N::evaluate(noEgg).status==N::Status::UnknownOriginalEggStage);
    auto noStage=in;noStage.independentlyVerifiedSameCreationAndCollectionEvent=false;
    assert(N::evaluate(noStage).status==N::Status::UnknownSharedEvent);
    auto wrongStage=in;wrongStage.originalEggStageToken=43;
    assert(N::evaluate(wrongStage).status==N::Status::IncoherentSharedEvent);
    auto noInitial=in;noInitial.iv.preInheritanceChildIVsVerified=false;
    assert(N::evaluate(noInitial).status==N::Status::StageHistoryUnresolved);
    auto pt=in;pt.game="platinum_nds";
    assert(N::evaluate(pt).status==N::Status::UnsupportedGame);
    auto wrongSource=in;wrongSource.verifiedContainingGame=false;
    assert(N::evaluate(wrongSource).status==N::Status::UnknownSourceGame);
    auto hatchedRaw=native(32,0,stage.child,false,0);
    Pokemon::Pokemon4ReadOnly hatched(hatchedRaw,Enums::GameVersion::DP);
    auto hatchedIn=in;hatchedIn.originalEgg=&hatched;
    assert(N::evaluate(hatchedIn).status==N::Status::UnknownOriginalEggStage);
    auto corruptedRaw=rawEgg;corruptedRaw[10]^=std::byte{0x11};
    Pokemon::Pokemon4ReadOnly bad(corruptedRaw,Enums::GameVersion::DP);
    auto badIn=in;badIn.originalEgg=&bad;
    assert(N::evaluate(badIn).status==N::Status::InvalidNativeRecord);
    assert(raw0==copy0 && raw1==copy1 && rawEgg==copyEgg);
    std::cout<<"L2 B12/B13 original Diamond/Pearl encrypted PK4 history PASS; "
                "positive, contradictory candidate and Unknown remain scoped\n";
}
