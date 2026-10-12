#include "Legality/Gen4DiamondPearlEggMoveStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace M=Legality::Gen4DiamondPearlEggMoveStage;
M::Input base(const char* game="diamond_nds"){
    M::Input in{};
    in.game=game;
    in.originalEggEventIdentity=0xD1A01234u;
    in.originalParentStageIdentity=in.originalEggEventIdentity;
    in.originalPhysicalPairIdentity=0xBEEFB001u;
    in.sharedOriginalEventIndependentlyAttested=true;
    in.physicalDepositOrderIndependentlyAttested=true;
    in.physicalSlots[0]={101,25,1,{33,45,98,57},true,true};
    in.physicalSlots[1]={202,25,0,{33,45,98,57},true,true};
    in.fatherPhysicalSlot=1;in.fatherRoleIndependentlyAttested=true;
    in.originalEggSpecies=25;
    in.originalUnhatchedEggIdentityIndependentlyAttested=true;
    in.originalUnhatchedEggMovesIndependentlyAttested=true;
    in.base={33,45,0,0};in.paternalEgg={98,0,0,0};
    in.paternalMachine={57,0,0,0};in.mutualLevel={33,0,0,0};
    in.originalBaseMovesIndependentlyAttested=true;
    in.paternalEggMethodIndependentlyAttested=true;
    in.paternalMachineMethodIndependentlyAttested=true;
    in.mutualLevelMethodIndependentlyAttested=true;
    in.originalCreationItemHistoryIndependentlyAttested=true;
    in.originalUnhatchedEggMoves={33,45,98,57};
    return in;
}
int main(){
    uint32_t checks=0;
    for(const char* game:{"diamond_nds","pearl_nds"}){
        for(uint32_t seed=0;seed<4096;++seed){
            auto in=base(game);
            const uint16_t first=static_cast<uint16_t>(1+(seed%400));
            const uint16_t second=static_cast<uint16_t>(1+((seed*7+13)%400));
            // The authoritative source transcript is synthetic; a stage
            // result is CONDITIONAL until all four methods are externally
            // attested on the original physical parents and exact egg.
            in.base={first,second,0,0};
            in.physicalSlots[0].originalMoves={first,second,98,57};
            in.physicalSlots[1].originalMoves={first,second,98,57};
            in.mutualLevel={first,0,0,0};
            const auto ordered=M::evaluate(in);
            assert(ordered.status==M::Status::ConditionalMoveHistoryMatched ||
                   ordered.status==M::Status::LocalMoveHistoryContradiction);
            in.originalUnhatchedEggMoves=ordered.construction.moves;
            const auto yes=M::evaluate(in);
            assert(yes.status==M::Status::ConditionalMoveHistoryMatched);
            assert(yes.originalFatherSnapshot==202 && yes.originalMotherSnapshot==101);
            auto wrong=in;
            wrong.originalUnhatchedEggMoves[0]^=1;
            assert(M::evaluate(wrong).status==M::Status::LocalMoveHistoryContradiction);
            auto noFather=in;
            noFather.physicalSlots[1].originalMoveSlotsIndependentlyAttested=false;
            assert(M::evaluate(noFather).status==M::Status::UnknownOriginalParents);
            ++checks;
        }
        auto x=base(game);
        assert(M::evaluate(x).status==M::Status::ConditionalMoveHistoryMatched);
        auto swapped=x;
        swapped.physicalSlots[0]=x.physicalSlots[1];
        swapped.physicalSlots[1]=x.physicalSlots[0];
        swapped.fatherPhysicalSlot=0;
        const auto swap=M::evaluate(swapped);
        assert(swap.status==M::Status::ConditionalMoveHistoryMatched);
        assert(swap.originalFatherSnapshot==202 && swap.originalMotherSnapshot==101);
        auto incorrectRoles=x;incorrectRoles.fatherPhysicalSlot=0;
        assert(M::evaluate(incorrectRoles).status==M::Status::IncoherentPhysicalParentRoles);
        auto missingRoles=x;missingRoles.fatherRoleIndependentlyAttested=false;
        assert(M::evaluate(missingRoles).status==M::Status::UnknownPhysicalParentRoles);
        auto sameParent=x;sameParent.physicalSlots[1].snapshotIdentity=101;
        assert(M::evaluate(sameParent).status==M::Status::UnknownOriginalParents);
        auto wrongEgg=x;wrongEgg.originalUnhatchedEggIdentityIndependentlyAttested=false;
        assert(M::evaluate(wrongEgg).status==M::Status::UnknownOriginalEgg);
        auto missingMoves=x;missingMoves.originalUnhatchedEggMovesIndependentlyAttested=false;
        assert(M::evaluate(missingMoves).status==M::Status::UnknownOriginalEgg);
        auto missingEvent=x;missingEvent.sharedOriginalEventIndependentlyAttested=false;
        assert(M::evaluate(missingEvent).status==M::Status::UnknownSharedDaycareEvent);
        auto crossedEvent=x;crossedEvent.originalParentStageIdentity++;
        assert(M::evaluate(crossedEvent).status==M::Status::IncoherentSharedDaycareEvent);
        auto noPair=x;noPair.originalPhysicalPairIdentity=0;
        assert(M::evaluate(noPair).status==M::Status::UnknownSharedDaycareEvent);
        auto noMachineWitness=x;noMachineWitness.paternalMachineMethodIndependentlyAttested=false;
        assert(M::evaluate(noMachineWitness).status==M::Status::UnknownOriginalMoveMethods);
        auto noEggWitness=x;noEggWitness.paternalEggMethodIndependentlyAttested=false;
        assert(M::evaluate(noEggWitness).status==M::Status::UnknownOriginalMoveMethods);
        auto noLevelWitness=x;noLevelWitness.mutualLevelMethodIndependentlyAttested=false;
        assert(M::evaluate(noLevelWitness).status==M::Status::UnknownOriginalMoveMethods);
        auto fraudulentMachine=x;fraudulentMachine.paternalMachine={99,0,0,0};
        assert(M::evaluate(fraudulentMachine).status==M::Status::IncoherentMethodWitness);
        auto fraudulentShared=x;fraudulentShared.physicalSlots[0].originalMoves[0]=0;
        assert(M::evaluate(fraudulentShared).status==M::Status::IncoherentMethodWitness);
        auto sameGender=x;sameGender.physicalSlots[0].gender=0;
        assert(M::evaluate(sameGender).status==M::Status::IncoherentPhysicalParentRoles);
        auto dittoMale=x;
        dittoMale.physicalSlots[0]={101,132,2,{33,45,98,57},true,true};
        dittoMale.physicalSlots[1].gender=0;
        assert(M::evaluate(dittoMale).status==M::Status::ConditionalMoveHistoryMatched);
        auto dittoFemale=x;
        dittoFemale.physicalSlots[1]={202,132,2,{33,45,98,57},true,true};
        dittoFemale.physicalSlots[0].gender=1;
        assert(M::evaluate(dittoFemale).status==M::Status::ConditionalMoveHistoryMatched);
        auto dittoDitto=dittoMale;
        dittoDitto.physicalSlots[1].species=132;
        assert(M::evaluate(dittoDitto).status==M::Status::IncoherentPhysicalParentRoles);
        auto pichu=x;
        pichu.originalEggSpecies=172;
        pichu.originalUnhatchedEggMoves={45,98,57,344};
        pichu.originalEligibleLightBallHolder=true;
        assert(M::evaluate(pichu).status==M::Status::ConditionalMoveHistoryMatched);
        pichu.originalCreationItemHistoryIndependentlyAttested=false;
        assert(M::evaluate(pichu).status==M::Status::UnknownCreationItems);
        pichu.originalCreationItemHistoryIndependentlyAttested=true;
        pichu.originalEligibleLightBallHolder=false;
        assert(M::evaluate(pichu).status==M::Status::LocalMoveHistoryContradiction);
        pichu.originalEggSpecies=25;
        assert(M::evaluate(pichu).status==M::Status::LocalMoveHistoryContradiction);
        checks+=25;
    }
    auto unsupported=base("platinum_nds");
    assert(M::evaluate(unsupported).status==M::Status::UnsupportedGame);
    std::cout << "L2 DP 4-slot move + physical parent + creation-item source histories "<<checks
              <<" seeded scenarios PASS (not original retail captures)\n";
}
