#include "Legality/Gen14CandidateMovePalParkNativeRetail.h"
#include <array>
#include <cassert>
#include <iostream>
namespace R=Legality::Gen14CandidateMovePalParkNativeRetail;
namespace J=Legality::Gen14CandidateMovePalParkNativeJoint;
namespace H=Legality::Gen14CandidateMoveJointEvolutionHistory;
using G=J::Game;using S=J::Status;using A=J::Action;
constexpr std::array<H::Game,5> gba{H::Game::Ruby,H::Game::Sapphire,H::Game::Emerald,H::Game::FireRed,H::Game::LeafGreen};
constexpr std::array<G,5> nds{G::Diamond,G::Pearl,G::Platinum,G::HeartGold,G::SoulSilver};
constexpr J::Input base(H::Game source,G destination){
    J::Input a{};a.candidateId=9943;a.species=1;a.arrivalGame=destination;a.observedGame=destination;
    a.arrivalLevel=20;a.observedLevel=20;
    a.samePokemonThroughTransferVerified=true;a.preTransferSnapshotVerified=true;
    a.immediateArrivalSnapshotVerified=true;a.nativeActionsCompleteVerified=true;
    auto& p=a.departure;p.candidateId=a.candidateId;p.origin=source;p.observedGame=source;
    p.originalSpecies=1;p.observedSpecies=1;p.initialLevel=20;p.observedLevel=20;
    p.originalSnapshotVerified=p.samePokemonVerified=true;
    p.everyLevelUpDecisionVerified=p.wholeClaimedHistoryVerified=true;
    p.initialMoves={33,45,73,22};p.observedMoves=p.initialMoves;
    a.recordedDeparture=a.recordedArrival=a.observedMoves=p.observedMoves;
    return a;
}
constexpr J::Input platinum(){
    auto a=base(H::Game::Ruby,G::Platinum);a.observedLevel=21;a.count=4;
    a.steps[0]={G::Platinum,21,A::OfferDecline,230,0,false,0,0,true,true};
    a.steps[1]={G::Platinum,21,A::Machine,412,0,false,53,123,true,true};
    a.steps[2]={G::Platinum,21,A::Tutor,235,1,false,0,0,true,true};
    a.steps[3]={G::Platinum,21,A::Reminder,230,2,false,0,0,true,true};
    a.observedMoves={412,235,230,22};return a;
}
static_assert(R::evaluate(platinum()).status==S::Consistent);
static_assert([]{auto a=platinum();a.steps[0].move=75;return R::evaluate(a).status==S::Contradicted;}());
static_assert([]{auto a=platinum();a.steps[1].machineNumber=1;return R::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=platinum();a.steps[1].machineNumber=90;return R::evaluate(a).status==S::Contradicted;}());
static_assert([]{auto a=platinum();a.steps[1].itemReceipt=0;return R::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=platinum();a.steps[1].game=G::HeartGold;return R::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=platinum();a.recordedArrival={33,45,73,76};return R::evaluate(a).status==S::Contradicted;}());
static_assert([]{auto a=base(H::Game::Ruby,G::Platinum);a.observedLevel=21;return R::evaluate(a).status==S::Contradicted;}());
static_assert([]{auto a=base(H::Game::Ruby,G::HeartGold);a.observedLevel=21;return R::evaluate(a).status==S::Unresolved;}());
int main(){
    for(const auto source:gba)for(const auto destination:nds){
        auto a=base(source,destination);
        assert(R::evaluate(a).status==S::Consistent);
        a.immediateArrivalSnapshotVerified=false;
        assert(R::evaluate(a).status==S::Unresolved);
    }
    assert(R::evaluate(platinum()).status==S::Consistent);
    std::cout<<"L3 original Gen III to Platinum native joint: PASS\n";
}
