#include "Legality/Gen14CandidateMovePalParkNativeJoint.h"
#include "Legality/Gen4PlatinumNativeMethodData.h"
#include "Legality/Gen4PlatinumPinnedMachines.h"
#include <array>
#include <cassert>
#include <iostream>
namespace J=Legality::Gen14CandidateMovePalParkNativeJoint;
namespace H=Legality::Gen14CandidateMoveJointEvolutionHistory;
namespace L=Legality::Gen4PlatinumNativeMethodData;
namespace M=Legality::Gen4PlatinumPinnedMachines;
using G=J::Game;using A=J::Action;using S=J::Status;
struct FullOriginalBulbasaur {
    static constexpr std::array<H::Offer,11> offers{{{1,33},{4,45},{7,73},{10,22},{15,77},{15,79},{20,75},{25,230},{32,74},{39,235},{46,76}}};
    static constexpr bool supported(H::Game g,uint16_t sp){return H::isGen3(g)&&sp==1;}
    static constexpr uint16_t offerCount(H::Game g,uint16_t sp){return supported(g,sp)?11:0;}
    static constexpr H::Offer offerAt(H::Game g,uint16_t sp,uint16_t i){return i<offerCount(g,sp)?offers[i]:H::Offer{};}
    static constexpr bool supportsMachine(H::Game,uint16_t,H::Machine,uint8_t,uint16_t){return false;}
};
struct OriginalPlatinum {
    static constexpr bool supported(G g,uint16_t sp){return g==G::Platinum && L::offerCount(sp)!=0;}
    static constexpr uint16_t offerCount(G g,uint16_t sp){return supported(g,sp)?L::offerCount(sp):0;}
    static constexpr J::Offer offerAt(G g,uint16_t sp,uint16_t i){const auto p=i<offerCount(g,sp)?L::packedOffer(sp,i):0;return {static_cast<uint8_t>(p>>9),static_cast<uint16_t>(p&511u)};}
    static constexpr bool machineKnown(G g,bool hm,uint8_t n){return g==G::Platinum && M::known(hm,n);}
    static constexpr bool machineCompatible(G g,uint16_t sp,bool hm,uint8_t n,uint16_t m){return supported(g,sp)&&M::compatible(sp,hm,n,m);}
    static constexpr bool tutorCompatible(G g,uint16_t sp,uint16_t m){return supported(g,sp)&&L::tutorCompatible(sp,m);}
};
constexpr J::Input make(H::Game source,G destination){
    J::Input x{};x.candidateId=1778;x.species=1;x.arrivalGame=x.observedGame=destination;
    x.arrivalLevel=x.observedLevel=20;
    x.samePokemonThroughTransferVerified=true;
    x.preTransferSnapshotVerified=true;x.immediateArrivalSnapshotVerified=true;
    x.nativeActionsCompleteVerified=true;
    auto& p=x.departure;p.candidateId=x.candidateId;p.origin=p.observedGame=source;
    p.originalSpecies=p.observedSpecies=1;p.initialLevel=p.observedLevel=20;
    p.initialMoves={33,45,73,22};p.observedMoves=p.initialMoves;
    p.originalSnapshotVerified=p.samePokemonVerified=true;
    p.everyLevelUpDecisionVerified=p.wholeClaimedHistoryVerified=true;
    x.recordedDeparture=x.recordedArrival=x.observedMoves=p.observedMoves;
    return x;
}
constexpr J::Input trained(){
    auto x=make(H::Game::Ruby,G::Platinum);x.observedLevel=21;x.count=4;
    x.steps[0]={G::Platinum,21,A::OfferDecline,230,0,false,0,0,true,true};
    x.steps[1]={G::Platinum,21,A::Machine,412,0,false,53,177,true,true};
    x.steps[2]={G::Platinum,21,A::Tutor,235,1,false,0,0,true,true};
    x.steps[3]={G::Platinum,21,A::Reminder,230,2,false,0,0,true,true};
    x.observedMoves={412,235,230,22};return x;
}
static_assert(J::evaluate<FullOriginalBulbasaur,OriginalPlatinum>(trained()).status==S::Consistent);
static_assert([]{auto x=trained();x.steps[1].machineNumber=90;return J::evaluate<FullOriginalBulbasaur,OriginalPlatinum>(x).status==S::Contradicted;}());
static_assert([]{auto x=trained();x.steps[1].machineNumber=1;return J::evaluate<FullOriginalBulbasaur,OriginalPlatinum>(x).status==S::Unresolved;}());
static_assert([]{auto x=trained();x.steps[2].move=75;return J::evaluate<FullOriginalBulbasaur,OriginalPlatinum>(x).status==S::Contradicted;}());
static_assert([]{auto x=trained();x.steps[0].move=75;return J::evaluate<FullOriginalBulbasaur,OriginalPlatinum>(x).status==S::Contradicted;}());
static_assert([]{auto x=trained();x.steps[1].itemReceipt=0;return J::evaluate<FullOriginalBulbasaur,OriginalPlatinum>(x).status==S::Unresolved;}());
int main(){
    constexpr std::array<H::Game,5> old{H::Game::Ruby,H::Game::Sapphire,H::Game::Emerald,H::Game::FireRed,H::Game::LeafGreen};
    constexpr std::array<G,5> dest{G::Diamond,G::Pearl,G::Platinum,G::HeartGold,G::SoulSilver};
    for(auto a:old)for(auto b:dest)assert((J::evaluate<FullOriginalBulbasaur,OriginalPlatinum>(make(a,b)).status==S::Consistent));
    assert((J::evaluate<FullOriginalBulbasaur,OriginalPlatinum>(trained()).status==S::Consistent));
    std::cout<<"L3 exact Platinum native offers, TM53, tutor, reminder and 25 transfers: PASS\n";
}
