#include "Legality/Gen4PlatinumHGSSOriginalEggMoves.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace M=Legality::Gen4PlatinumHGSSOriginalEggMoves;
M::Input source(const char* game){
    M::Input a{};
    a.game=game;
    a.originalEggEventIdentity=0xF00DA;
    a.originalParentStageIdentity=0xF00DA;
    a.originalPhysicalPairIdentity=0xAB;
    a.independentlyAttestedSharedSourceEvent=true;
    a.independentlyAttestedPhysicalDepositOrder=true;
    a.independentlyAttestedDaycareEligibility=true;
    a.eligibleOrdinaryDaycarePair=true;
    a.physicalSlots[0]={101,25,1,0,{33,45,98,57},true,true,true};
    a.physicalSlots[1]={202,25,0,0,{33,45,98,57},true,true,true};
    a.moveFatherPhysicalSlot=1;
    a.moveFatherRoleIndependentlyAttested=true;
    a.originalEggSpecies=25;
    a.originalEggSpeciesOriginIndependentlyAttested=true;
    a.originalUnhatchedEggMovesIndependentlyAttested=true;
    a.originalUnhatchedEggMoves={33,45,98,57};
    a.originalBaseMoves={33,45,0,0};
    a.originalFatherEggMoves={98,0,0,0};
    a.originalFatherMachineMoves={57,0,0,0};
    a.originalSharedLevelMoves={33,0,0,0};
    a.baseLearnsetAtOriginalStageIndependentlyAttested=true;
    a.originalEggMoveCompatibilityIndependentlyAttested=true;
    a.originalTMHMCompatibilityIndependentlyAttested=true;
    a.originalSharedLearnsetIndependentlyAttested=true;
    return a;
}
int main(){
    uint32_t checked=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"}) {
        auto a=source(game);
        assert(M::evaluate(a).status==M::Status::ConditionalMoveHistoryMatched);
        assert(M::evaluate(a).fatherSnapshot==202);
        assert(M::evaluate(a).motherSnapshot==101);
        for(uint32_t seed=0;seed<4096;++seed){
            a=source(game);
            a.originalBaseMoves={static_cast<uint16_t>(1u+(seed%400u)),
                                 static_cast<uint16_t>(1u+((seed*7u+11u)%400u)),0,0};
            a.originalUnhatchedEggMoves={a.originalBaseMoves[0],a.originalBaseMoves[1],98,57};
            const auto yes=M::evaluate(a);
            assert(yes.status==M::Status::ConditionalMoveHistoryMatched ||
                   yes.status==M::Status::LocalMoveHistoryContradicted);
            // When a reused base move or source move collides, the FIFO
            // must still produce a stable original four-slot outcome.
            a.originalUnhatchedEggMoves=yes.replay.moves;
            assert(M::evaluate(a).status==M::Status::ConditionalMoveHistoryMatched);
            a.originalUnhatchedEggMoves[0]^=1u;
            assert(M::evaluate(a).status==M::Status::LocalMoveHistoryContradicted);
            ++checked;
        }
        a=source(game);a.originalParentStageIdentity++;
        assert(M::evaluate(a).status==M::Status::IncoherentOriginalEvent);
        a=source(game);a.originalPhysicalPairIdentity=0;
        assert(M::evaluate(a).status==M::Status::UnknownOriginalEvent);
        a=source(game);a.physicalSlots[0].snapshotIdentity=202;
        assert(M::evaluate(a).status==M::Status::UnknownParentSources);
        a=source(game);a.independentlyAttestedDaycareEligibility=false;
        assert(M::evaluate(a).status==M::Status::UnknownDaycareEligibility);
        a=source(game);a.eligibleOrdinaryDaycarePair=false;
        assert(M::evaluate(a).status==M::Status::IncompatibleDaycareCandidate);
        a=source(game);a.moveFatherRoleIndependentlyAttested=false;
        assert(M::evaluate(a).status==M::Status::UnknownParentSources);
        a=source(game);a.moveFatherPhysicalSlot=0;
        assert(M::evaluate(a).status==M::Status::IncompatibleDaycareCandidate);
        a=source(game);a.originalEggSpeciesOriginIndependentlyAttested=false;
        assert(M::evaluate(a).status==M::Status::UnknownEggSpeciesOrigin);
        a=source(game);a.baseLearnsetAtOriginalStageIndependentlyAttested=false;
        assert(M::evaluate(a).status==M::Status::UnknownOriginalMoveMethods);
        a=source(game);a.originalEggMoveCompatibilityIndependentlyAttested=false;
        assert(M::evaluate(a).status==M::Status::UnknownOriginalMoveMethods);
        a=source(game);a.originalTMHMCompatibilityIndependentlyAttested=false;
        assert(M::evaluate(a).status==M::Status::UnknownOriginalMoveMethods);
        a=source(game);a.originalSharedLearnsetIndependentlyAttested=false;
        assert(M::evaluate(a).status==M::Status::UnknownOriginalMoveMethods);
        a=source(game);a.physicalSlots[1].originalMoves={33,0,98,57};
        assert(M::evaluate(a).status==M::Status::IncoherentMethodAttestations);
        a.originalFatherEggMoves={0,0,0,0};
        a.originalSharedLevelMoves={0,0,0,0};
        a.originalUnhatchedEggMoves={33,45,57,0};
        assert(M::evaluate(a).status==M::Status::ConditionalMoveHistoryMatched);
        a=source(game);a.originalFatherMachineMoves={57,98,0,0};
        assert(M::evaluate(a).status==M::Status::IncoherentMethodAttestations);
        a=source(game);a.originalFatherEggMoves={57,98,0,0};
        assert(M::evaluate(a).status==M::Status::IncoherentMethodAttestations);
        a=source(game);a.originalSharedLevelMoves={98,33,0,0};
        assert(M::evaluate(a).status==M::Status::IncoherentMethodAttestations);
        a=source(game);a.originalFatherEggMoves={99,0,0,0};
        assert(M::evaluate(a).status==M::Status::IncoherentMethodAttestations);
        a=source(game);a.originalEggSpecies=172;
        a.originalUnhatchedEggMoves={45,98,57,344};
        assert(M::evaluate(a).status==M::Status::LocalMoveHistoryContradicted);
        a.physicalSlots[0].heldItemAtCreationIndependentlyAttested=false;
        assert(M::evaluate(a).status==M::Status::UnknownOriginalCreationItems);
        a.physicalSlots[0].heldItemAtCreationIndependentlyAttested=true;
        a.physicalSlots[0].originalHeldItem=236;
        assert(M::evaluate(a).status==M::Status::ConditionalMoveHistoryMatched);
        a.physicalSlots[1].heldItemAtCreationIndependentlyAttested=false;
        assert(M::evaluate(a).status==M::Status::UnknownOriginalCreationItems);
        a=source(game);a.physicalSlots[0].species=132;
        a.physicalSlots[0].gender=2;
        assert(M::evaluate(a).status==M::Status::ConditionalMoveHistoryMatched);
        a=source(game);a.physicalSlots[1].species=132;
        a.physicalSlots[1].gender=2;
        a.originalFatherEggMoves={0,0,0,0};
        a.originalFatherMachineMoves={0,0,0,0};
        a.originalUnhatchedEggMoves={33,45,0,0};
        assert(M::evaluate(a).status==M::Status::ConditionalMoveHistoryMatched);
        a=source(game);a.physicalSlots[0].species=132;
        a.physicalSlots[1].species=132;
        a.physicalSlots[0].gender=2;
        a.physicalSlots[1].gender=2;
        assert(M::evaluate(a).status==M::Status::IncompatibleDaycareCandidate);
        checked+=24;
    }
    auto other=source("diamond_nds");
    assert(M::evaluate(other).status==M::Status::UnsupportedGame);
    std::cout<<"L2 Pt/HGSS original four-move acquisition: "<<checked
             <<" seeded and source-order controls PASS\n";
}
