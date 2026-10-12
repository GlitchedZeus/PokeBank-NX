#include "Legality/Gen14CandidateMovePalParkJointStage.h"
#include <array>
#include <cassert>
#include <iostream>
namespace J=Legality::Gen14CandidateMovePalParkJointStage;
namespace C=Legality::Gen14CandidateMoveChronology;
namespace M=Legality::Gen14CandidateMoveMachineEvidence;
using G=C::Game; using A=C::Action; using T=C::Method; using S=J::Status;
constexpr std::array<G,5> gba{{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen}};
constexpr std::array<G,5> nds{{G::Diamond,G::Pearl,G::Platinum,G::HeartGold,G::SoulSilver}};
constexpr J::Input basic(G source,G dest) {
    J::Input a{};
    a.sameOriginalPokemonAcrossBothWitnessesVerified=true;
    auto& p=a.path;
    p.candidateId=2001;p.species=1;p.origin=source;
    p.initialLevel=20;p.observedLevel=20;
    p.initialMoves={33,45,0,0};p.observedMoves={33,45,92,0};
    p.initialSnapshotVerified=true;p.samePokemonAndSpeciesVerified=true;
    p.fullChronologyVerified=true;p.stepCount=2;
    p.steps[0]={source,20,A::Learn,T::TM,92,2,true,source};
    p.steps[1]={dest,20,A::PalPark,T::LevelUp,0,0,true,G::Unknown};
    a.crossing.sourceGame=J::gameName(source);
    a.crossing.destinationGame=J::gameName(dest);
    a.crossing.species=1;
    a.crossing.independentlyVerifiedPreTransfer=true;
    a.crossing.independentlyVerifiedImmediateArrival=true;
    a.crossing.unchangedSpeciesThroughArrivalVerified=true;
    a.crossing.outboundGbaMoves={33,45,92,0};
    a.crossing.immediateNdsMoves={33,45,92,0};
    a.gbaMachineCount=1;
    a.gbaMachines[0]={0,{source,1,M::Machine::TM,6,92,true,true}};
    return a;
}
static_assert(J::evaluate(basic(G::Ruby,G::Diamond)).status==S::CandidateConsistent);
static_assert(J::evaluate(basic(G::LeafGreen,G::SoulSilver)).status==S::CandidateConsistent);
static_assert(J::evaluate(basic(G::Emerald,G::Platinum)).verifiedGbaMachines==1);
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.sameOriginalPokemonAcrossBothWitnessesVerified=false;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.crossing.independentlyVerifiedImmediateArrival=false;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.crossing.outboundGbaMoves={33,45,0,0};
    a.crossing.immediateNdsMoves=a.crossing.outboundGbaMoves;
    return J::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.crossing.immediateNdsMoves={33,92,45,0};
    return J::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.gbaMachineCount=0;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.gbaMachines[0].exactItem.machineNumber=10;
    return J::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.gbaMachines[0].stepIndex=1;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.gbaMachineCount=2;
    a.gbaMachines[1]=a.gbaMachines[0];
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.path.steps[1].game=G::Platinum;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.crossing.sourceGame="emerald_gba";
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.path.species=4;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.path.stepCount=1;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.path.stepCount=3;
    a.path.steps[2]=a.path.steps[1];
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.path.steps[1].evidenceVerified=false;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=basic(G::Ruby,G::Diamond);a.path.steps[0].methodSource=G::Emerald;
    return J::evaluate(a).status==S::Unresolved;}());
constexpr J::Input withNativeHm(bool nativeAttested) {
    auto a=basic(G::Ruby,G::Platinum);
    a.path.stepCount=3;a.path.steps[2]={G::Platinum,20,A::Learn,T::HM,15,3,true,G::Platinum};
    a.path.observedMoves={33,45,92,15};
    if(nativeAttested){
        a.laterMethodCount=1;
        a.laterMethods[0]={2,G::Platinum,T::HM,15,20,71,true};
    }
    return a;
}
static_assert(J::evaluate(withNativeHm(false)).status==S::Unresolved);
static_assert([]{auto a=basic(G::Ruby,G::Platinum);
    a.path.stepCount=3;
    a.path.steps[2]={G::Platinum,20,A::AlreadyKnown,T::LevelUp,92,0,true,G::Platinum};
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=withNativeHm(true);
    a.path.stepCount=4;
    a.path.steps[3]={G::Platinum,20,A::Decline,T::LevelUp,76,0,true,G::Platinum};
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=withNativeHm(true);
    a.path.stepCount=4;
    a.path.steps[3]={G::Platinum,20,A::DeleteMove,T::LevelUp,0,3,true,G::Unknown};
    return J::evaluate(a).status==S::Unresolved;}());

static_assert(J::evaluate(withNativeHm(true)).status==S::CandidateConsistent);
static_assert(J::evaluate(withNativeHm(true)).verifiedNativeMethods==1);
static_assert([]{auto a=withNativeHm(true);a.laterMethods[0].independentlyVerified=false;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=withNativeHm(true);a.laterMethods[0].independentSourceRecord=0;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=withNativeHm(true);a.laterMethods[0].sourceGame=G::HeartGold;
    return J::evaluate(a).status==S::Unresolved;}());
constexpr J::Input beforeTransferHmDeleted() {
    auto a=basic(G::Ruby,G::HeartGold);
    a.path.stepCount=4;
    a.path.steps[1]={G::Ruby,20,A::Learn,T::HM,15,3,true,G::Ruby};
    a.path.steps[2]={G::Ruby,20,A::DeleteMove,T::LevelUp,0,3,true,G::Unknown};
    a.path.steps[3]={G::HeartGold,20,A::PalPark,T::LevelUp,0,0,true,G::Unknown};
    a.gbaMachineCount=2;
    a.gbaMachines[1]={1,{G::Ruby,1,M::Machine::HM,1,15,true,true}};
    return a;
}
static_assert(J::evaluate(beforeTransferHmDeleted()).status==S::CandidateConsistent);
static_assert(J::evaluate(beforeTransferHmDeleted()).verifiedGbaMachines==2);
static_assert([]{auto a=beforeTransferHmDeleted();a.path.steps[2].evidenceVerified=false;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=beforeTransferHmDeleted();a.path.steps[2].slot=2;
    return J::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=beforeTransferHmDeleted();a.path.steps[2].action=A::VisitGame;
    return J::evaluate(a).status==S::Unresolved;}());
constexpr J::Input hmAtDeparture() {
    auto a=basic(G::Ruby,G::Diamond);
    a.path.stepCount=3;
    a.path.steps[1]={G::Ruby,20,A::Learn,T::HM,15,3,true,G::Ruby};
    a.path.steps[2]={G::Diamond,20,A::PalPark,T::LevelUp,0,0,true,G::Unknown};
    a.crossing.outboundGbaMoves={33,45,92,15};
    a.crossing.immediateNdsMoves=a.crossing.outboundGbaMoves;
    a.path.observedMoves=a.crossing.immediateNdsMoves;
    a.gbaMachineCount=2;
    a.gbaMachines[1]={1,{G::Ruby,1,M::Machine::HM,1,15,true,true}};
    return a;
}
static_assert(J::evaluate(hmAtDeparture()).status==S::CandidateContradicted);
constexpr J::Input changedGbaGame(){
    auto a=basic(G::Ruby,G::SoulSilver);
    a.path.stepCount=3;
    a.path.steps[0]={G::Emerald,20,A::VisitGame,T::LevelUp,0,0,true,G::Unknown};
    a.path.steps[1]={G::Emerald,20,A::Learn,T::TM,92,2,true,G::Emerald};
    a.path.steps[2]={G::SoulSilver,20,A::PalPark,T::LevelUp,0,0,true,G::Unknown};
    a.crossing.sourceGame="emerald_gba";
    a.gbaMachines[0]={1,{G::Emerald,1,M::Machine::TM,6,92,true,true}};
    return a;
}
static_assert(J::evaluate(changedGbaGame()).status==S::CandidateConsistent);
static_assert([]{auto a=changedGbaGame();a.crossing.sourceGame="ruby_gba";
    return J::evaluate(a).status==S::Unresolved;}());
int main(){
    for(G source:gba)for(G destination:nds){
        auto a=basic(source,destination);
        const auto result=J::evaluate(a);
        assert(result.status==S::CandidateConsistent);
        assert(result.verifiedGbaMachines==1 && result.verifiedNativeMethods==0);
        assert(result.departureMoves==a.crossing.outboundGbaMoves);
        assert(result.arrivalMoves==a.crossing.immediateNdsMoves);
        a.gbaMachines[0].exactItem.itemIdentityVerified=false;
        assert(J::evaluate(a).status==S::Unresolved);
    }
    std::cout<<"L3 all 25 GBA-to-GenIV source/destination Pal Park joints: PASS\n";
}
