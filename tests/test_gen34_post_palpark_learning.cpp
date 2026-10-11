#include "Legality/Gen34PostPalParkLearning.h"
#include <array>
#include <cassert>
#include <iostream>
namespace L=Legality::Gen34PostPalParkLearning;
namespace P=Legality::Gen34PalParkSnapshotTransition;
void runCases(){
    L::Input in{};
    in.crossing={"emerald_gba","platinum_nds",1,true,true,true,
                 {33,45,73,22},{33,45,73,22}};
    in.samePokemonAndGameContinuityVerified=true;
    in.fullPostArrivalSequenceVerified=true;
    in.arrivalLevel=30;in.observedLevel=50;
    in.stepCount=2;
    // Hypothetical separately verified Gen IV teaching after Pal Park,
    // not a claim that a particular saved Bulbasaur received these.
    in.steps[0]={40,1,15,L::Method::HiddenMachine,0,true};
    in.steps[1]={50,2,92,L::Method::TechnicalMachine,1,true};
    in.observedMoves={15,92,73,22};
    auto consistent=L::evaluate(in);
    assert(consistent.status==L::Status::CandidateConsistent);
    assert(consistent.appliedMoves==2);
    assert((consistent.rebuilt==std::array<uint16_t,4>{15,92,73,22}));
    auto noPre=in;noPre.crossing.independentlyVerifiedPreTransfer=false;
    assert(L::evaluate(noPre).status==L::Status::UnknownTransfer);
    auto missingHistory=in;missingHistory.fullPostArrivalSequenceVerified=false;
    assert(L::evaluate(missingHistory).status==L::Status::UnknownTrainingHistory);
    auto ungroundedMethod=in;ungroundedMethod.steps[0].independentlyVerifiedMethod=false;
    assert(L::evaluate(ungroundedMethod).status==L::Status::UnknownMethodSource);
    auto duplicate=in;duplicate.steps[1].move=15;
    assert(L::evaluate(duplicate).status==L::Status::CandidateContradicted);
    auto shuffled=in;shuffled.steps[1].stage=1;
    assert(L::evaluate(shuffled).status==L::Status::MalformedHistory);
    auto early=in;early.steps[1].level=39;
    assert(L::evaluate(early).status==L::Status::MalformedHistory);
    auto tooNew=in;tooNew.steps[1].move=468;
    assert(L::evaluate(tooNew).status==L::Status::MalformedHistory);
    auto wrongEnd=in;wrongEnd.observedMoves={15,22,73,92};
    assert(L::evaluate(wrongEnd).status==L::Status::CandidateContradicted);
    auto impossibleHM=in;impossibleHM.crossing.outboundGbaMoves[0]=15;
    assert(L::evaluate(impossibleHM).status==L::Status::CandidateContradicted);
    auto tooMany=in;tooMany.stepCount=17;
    assert(L::evaluate(tooMany).status==L::Status::MalformedHistory);
    auto wrongGame=in;wrongGame.crossing.destinationGame="black_nds";
    assert(L::evaluate(wrongGame).status==L::Status::Unsupported);
    auto noSourceContinuity=in;noSourceContinuity.samePokemonAndGameContinuityVerified=false;
    assert(L::evaluate(noSourceContinuity).status==L::Status::UnknownTrainingHistory);
    auto alreadyKnown=in;alreadyKnown.steps[0].move=33;
    assert(L::evaluate(alreadyKnown).status==L::Status::CandidateContradicted);
    auto holes=in;holes.crossing.outboundGbaMoves={33,0,73,22};
    assert(L::evaluate(holes).status==L::Status::MalformedHistory);
}
int main(){runCases();std::cout<<"Gen III->IV Pal Park later HM/TM acquire chronological stages: PASS\n";}
