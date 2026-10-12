#include "Legality/Gen12CandidateMoveMultiStage.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen12CandidateMoveMultiStage;
using G=H::Game;using A=H::Action;using S=H::Status;
constexpr H::Path bulba(G game,std::uint8_t level=16) {
    H::Path p{};
    p.candidateId=413;p.initialSpecies=1;p.observedSpecies=1;
    p.originalGame=p.observedGame=game;
    p.initialLevel=5;p.observedLevel=level;
    p.initialMoves={33,45,0,0};p.observedMoves=p.initialMoves;
    p.initialSnapshotAttested=p.sameIndividualAttested=p.everyInterveningActionAttested=true;
    return p;
}
constexpr H::Path redEvolve() {
    auto p=bulba(G::Red,22);
    p.observedSpecies=2;
    p.stepCount=4;
    p.steps[0]={G::Red,1,7,A::Learn,73,2,0,true};
    p.steps[1]={G::Red,1,13,A::Learn,22,3,0,true};
    p.steps[2]={G::Red,2,16,A::Evolve,0,0,0,true};
    p.steps[3]={G::Red,2,22,A::Learn,77,0,0,true};
    p.observedMoves={77,45,73,22};
    return p;
}
static_assert(H::evaluate(redEvolve()).status==S::CandidateConsistent);
static_assert(H::evaluate(redEvolve()).evolutions==1);
static_assert([]{auto p=redEvolve();p.steps[2].move=73;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=redEvolve();p.steps[2].slot=2;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=redEvolve();p.steps[0].machineNumber=10;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=redEvolve();p.steps[2].level=15;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=redEvolve();p.steps[3].move=75;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=redEvolve();p.steps[3].originalMethodWitness=false;
    return H::evaluate(p).status==S::Unknown;}());
constexpr H::Path crystalEvolve() {
    auto p=bulba(G::Crystal,22);p.observedSpecies=2;p.stepCount=6;
    p.steps[0]={G::Crystal,1,7,A::Learn,73,2,0,true};
    p.steps[1]={G::Crystal,1,10,A::Learn,22,3,0,true};
    p.steps[2]={G::Crystal,1,15,A::Learn,77,0,0,true};
    p.steps[3]={G::Crystal,1,15,A::Decline,79,0,0,true};
    p.steps[4]={G::Crystal,2,16,A::Evolve,0,0,0,true};
    p.steps[5]={G::Crystal,2,22,A::Learn,75,1,0,true};
    p.observedMoves={77,75,73,22};return p;
}
static_assert(H::evaluate(crystalEvolve()).status==S::CandidateConsistent);
static_assert([]{auto p=crystalEvolve();p.steps[3].move=75;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=crystalEvolve();p.stepCount=5;
    return H::evaluate(p).status==S::CandidateContradicted;}());
constexpr H::Path directTransfer() {
    auto p=bulba(G::Red,10);
    p.observedGame=G::Crystal;p.timeCapsulePolicyAttested=true;
    p.sourceWasNotEggAttested=true;p.stepCount=3;
    p.steps[0]={G::Red,1,7,A::Learn,73,2,0,true};
    p.steps[1]={G::Crystal,1,8,A::ForwardToGen2,0,0,0,true};
    p.steps[2]={G::Crystal,1,10,A::Learn,22,3,0,true};
    p.observedMoves={33,45,73,22};return p;
}
static_assert(H::evaluate(directTransfer()).status==S::CandidateConsistent);
static_assert([]{auto p=directTransfer();p.timeCapsulePolicyAttested=false;
    return H::evaluate(p).status==S::Unknown;}());
constexpr H::Path timeCapsule() {
    auto p=bulba(G::Crystal,11);p.stepCount=4;
    p.timeCapsulePolicyAttested=p.sourceWasNotEggAttested=true;
    p.steps[0]={G::Crystal,1,7,A::Learn,73,2,0,true};
    p.steps[1]={G::Red,1,8,A::ToGen1,0,0,0,true};
    p.steps[2]={G::Crystal,1,10,A::ReturnToGen2,0,0,0,true};
    p.steps[3]={G::Crystal,1,11,A::TeachTM,237,3,10,true};
    p.observedMoves={33,45,73,237};return p;
}
static_assert(H::evaluate(timeCapsule()).status==S::CandidateConsistent);
static_assert([]{auto p=timeCapsule();p.steps[1].move=33;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=timeCapsule();p.steps[2].machineNumber=10;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=timeCapsule();p.steps[1]={G::Crystal,1,8,A::TeachTM,237,3,10,true};
    p.steps[2]={G::Red,1,9,A::ToGen1,0,0,0,true};
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=timeCapsule();p.steps[3].machineNumber=8;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=timeCapsule();p.steps[1].originalMethodWitness=false;
    return H::evaluate(p).status==S::Unknown;}());
constexpr H::Path originalMachines() {
    auto p=bulba(G::Red,5);p.stepCount=2;
    p.steps[0]={G::Red,1,5,A::TeachTM,38,2,10,true};
    p.steps[1]={G::Red,1,5,A::TeachHM,15,3,1,true};
    p.observedMoves={33,45,38,15};return p;
}
static_assert(H::evaluate(originalMachines()).status==S::CandidateConsistent);
static_assert([]{auto p=originalMachines();p.steps[0].machineNumber=6;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=originalMachines();p.steps[0].machineNumber=11;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=originalMachines();p.steps[1].originalMethodWitness=false;
    return H::evaluate(p).status==S::Unknown;}());
constexpr H::Path unknownBreeding() {
    auto p=redEvolve();p.stepCount=5;
    p.steps[4]={G::Red,2,22,A::Breeding,0,0,0,true};
    return p;
}
static_assert(H::evaluate(unknownBreeding()).status==S::Unknown);
static_assert([]{auto p=redEvolve();p.stepCount=5;
    p.steps[4]={G::Red,2,22,A::Reminder,73,0,0,true};
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=redEvolve();p.stepCount=5;
    p.steps[4]={G::Red,2,22,A::Tutor,73,0,0,true};
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=redEvolve();p.steps[2].species=156;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=crystalEvolve();p.steps[4].species=3;
    return H::evaluate(p).status==S::Unknown;}());
constexpr H::Path gen2Deleter() {
    auto p=crystalEvolve();p.observedLevel=22;p.stepCount=7;
    p.steps[6]={G::Crystal,0,22,A::DeleteMove,0,2,0,true};
    p.observedMoves={77,75,22,0};return p;
}
static_assert(H::evaluate(gen2Deleter()).status==S::CandidateConsistent);
static_assert([]{auto p=gen2Deleter();p.steps[6].originalMethodWitness=false;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=gen2Deleter();p.steps[6].game=G::Red;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=redEvolve();p.steps[2].level=24;p.steps[3].level=22;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=crystalEvolve();p.steps[4].level=17;
    return H::evaluate(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=crystalEvolve();p.steps[4].level=23;
    return H::evaluate(p).status==S::Unknown;}());

constexpr H::Path charmelEvolve() {
    auto p=bulba(G::Yellow,24);
    p.initialSpecies=4;p.observedSpecies=5;p.initialLevel=8;
    p.initialMoves={10,45,0,0};p.stepCount=4;
    p.steps[0]={G::Yellow,4,9,A::Learn,52,2,0,true};
    p.steps[1]={G::Yellow,4,15,A::Learn,43,3,0,true};
    p.steps[2]={G::Yellow,5,16,A::Evolve,0,0,0,true};
    p.steps[3]={G::Yellow,5,24,A::Learn,99,0,0,true};
    p.observedMoves={99,45,52,43};return p;
}
static_assert(H::evaluate(charmelEvolve()).status==S::CandidateConsistent);
static_assert([]{auto p=charmelEvolve();p.steps[3].level=22;
    return H::evaluate(p).status==S::CandidateContradicted;}());
constexpr H::Path quilavaEvolve() {
    auto p=bulba(G::Crystal,21);
    p.initialSpecies=155;p.observedSpecies=156;
    p.initialMoves={33,43,0,0};p.stepCount=4;
    p.steps[0]={G::Crystal,155,6,A::Learn,108,2,0,true};
    p.steps[1]={G::Crystal,155,12,A::Learn,52,3,0,true};
    p.steps[2]={G::Crystal,156,14,A::Evolve,0,0,0,true};
    p.steps[3]={G::Crystal,156,21,A::Learn,98,0,0,true};
    p.observedMoves={98,43,108,52};return p;
}
static_assert(H::evaluate(quilavaEvolve()).status==S::CandidateConsistent);
static_assert([]{auto p=quilavaEvolve();p.steps[3].level=19;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=quilavaEvolve();p.steps[2].level=13;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=quilavaEvolve();p.steps[3].originalMethodWitness=false;
    return H::evaluate(p).status==S::Unknown;}());

int main() {
    std::uint32_t checks=0;
    for(auto g:{G::Red,G::Blue,G::Yellow}) {
        auto p=redEvolve();p.originalGame=p.observedGame=g;
        for(auto& s:p.steps)if(s.originalMethodWitness)s.game=g;
        assert(H::evaluate(p).status==S::CandidateConsistent);
        ++checks;
    }
    for(auto g:{G::Gold,G::Silver,G::Crystal}) {
        auto p=crystalEvolve();p.originalGame=p.observedGame=g;
        for(auto& s:p.steps)if(s.originalMethodWitness)s.game=g;
        assert(H::evaluate(p).status==S::CandidateConsistent);
        ++checks;
    }
    for(auto g:{G::Red,G::Blue,G::Yellow}) {
        auto p=charmelEvolve();p.originalGame=p.observedGame=g;
        for(auto& s:p.steps)if(s.originalMethodWitness)s.game=g;
        assert(H::evaluate(p).status==S::CandidateConsistent);
        ++checks;
    }
    for(auto g:{G::Gold,G::Silver,G::Crystal}) {
        auto p=quilavaEvolve();p.originalGame=p.observedGame=g;
        for(auto& s:p.steps)if(s.originalMethodWitness)s.game=g;
        assert(H::evaluate(p).status==S::CandidateConsistent);
        ++checks;
    }
    // Each original game has a fully represented source/level row for every
    // species marked supported. Missing species must never gain certainty.
    for(auto g:{G::Red,G::Blue,G::Yellow,G::Gold,G::Silver,G::Crystal}) {
        for(std::uint16_t species=1;species<=251;++species) {
            const auto n=H::count(g,species);
            assert((n!=0)==H::supported(g,species));
            std::uint8_t previous=0;
            for(std::uint8_t i=0;i<n;++i) {
                const auto o=H::offer(g,species,i);
                assert(o.level>0&&o.level<=100&&o.move>=1&&o.move<=251);
                assert(o.level>=previous);previous=o.level;
                ++checks;
            }
            ++checks;
        }
    }
    H::Alternatives alt{};alt.count=2;
    alt.paths[0]=redEvolve();alt.paths[0].steps[3].move=75;
    alt.paths[1]=crystalEvolve();alt.paths[1].candidateId=alt.paths[0].candidateId;
    assert(H::evaluate(alt)==S::CandidateConsistent);++checks;
    alt.paths[1].steps[5].originalMethodWitness=false;
    assert(H::evaluate(alt)==S::Unknown);++checks;
    alt.everyActualEncounterAndHistoryEnumerated=true;
    assert(H::evaluate(alt)==S::Unknown);++checks;
    alt.count=1;
    assert(H::evaluate(alt)==S::CandidateContradicted);++checks;
    // Deterministic bounded candidate mutation; global legality is never
    // decided from one candidate, and unknown history has priority.
    std::uint32_t seed=0x734c1129u;
    for(unsigned i=0;i<8192;++i) {
        seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
        auto p=(seed&1u)?redEvolve():crystalEvolve();
        const auto step=static_cast<std::uint8_t>((seed>>1)%p.stepCount);
        const auto mutation=static_cast<unsigned>((seed>>5)%5);
        if(mutation==0)p.steps[step].originalMethodWitness=false;
        else if(mutation==1)p.everyInterveningActionAttested=false;
        else if(mutation==2)p.steps[step].game=G::Unknown;
        else if(mutation==3)p.steps[step].level=0;
        else p.observedSpecies=999;
        const auto observed=H::evaluate(p).status;
        assert(observed!=S::CandidateConsistent);
        ++checks;
    }
    std::cout<<"L3 Gen I/II multistage native four-slot histories: "<<checks
             <<" scenario/assertion groups PASS (candidate-only, no global Invalid)\n";
}
