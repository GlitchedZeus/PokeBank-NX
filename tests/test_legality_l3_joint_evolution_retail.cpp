#include "Legality/Gen14CandidateMoveJointEvolutionRetail.h"
#include <array>
#include <cassert>
#include <iostream>
namespace H=Legality::Gen14CandidateMoveJointEvolutionHistory;
namespace R=Legality::Gen14CandidateMoveJointEvolutionRetail;
using G=H::Game; using A=H::Action; using M=H::Machine; using S=H::Status;

// The actual full 386-species per-version source tables are used, not a
// test-only substitute. The hypothetical original snapshot and user
// decisions are verified inputs, not conclusions inferred from native PK3.
constexpr H::Path sourceEvolution(G game) {
    H::Path p{};p.candidateId=9912;p.origin=game;p.observedGame=game;
    p.originalSpecies=5;p.observedSpecies=6;
    p.initialLevel=33;p.observedLevel=38;
    p.initialMoves={10,45,108,184};p.observedMoves={19,45,17,92};
    p.originalSnapshotVerified=p.samePokemonVerified=true;
    p.everyLevelUpDecisionVerified=p.wholeClaimedHistoryVerified=true;
    p.count=5;
    p.steps[0]={game,34,A::OfferedDecline,53,0,0,M::TM,0,G::Unknown,0,true,false,0};
    p.steps[1]={game,36,A::Evolve,0,0,6,M::TM,0,G::Unknown,0,true,true,0};
    p.steps[2]={game,36,A::OfferedLearn,17,2,0,M::TM,0,G::Unknown,0,true,false,0};
    p.steps[3]={game,37,A::TeachMachine,92,3,0,M::TM,6,game,6,true,true,881};
    p.steps[4]={game,38,A::TeachMachine,19,0,0,M::HM,2,game,6,true,true,0};
    return p;
}
constexpr std::array<G,5> games{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen};
static_assert([]{
    for(G game:games)
        if(R::evaluate(sourceEvolution(game)).status!=S::CandidateConsistent)
            return false;
    return true;
}());
static_assert([]{auto p=sourceEvolution(G::Ruby);
    p.steps[1].targetSpecies=94;
    return R::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=sourceEvolution(G::Ruby);
    p.steps[2].move=97;
    return R::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=sourceEvolution(G::FireRed);
    p.steps[4].receiptSpecies=5;
    return R::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=sourceEvolution(G::LeafGreen);
    p.steps[4].level=35;
    return R::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=sourceEvolution(G::Ruby);
    p.steps[3].receiptId=0;
    return R::evaluate(p).status==S::Unresolved;}());
// Same target moves do not imply that an original Ruby source stage can
// claim FireRed's level-13 Metal Claw offer without a verified game visit.
static_assert([]{auto p=sourceEvolution(G::Ruby);
    p.originalSpecies=4;p.observedSpecies=4;
    p.initialLevel=12;p.observedLevel=13;
    p.initialMoves={10,45,52,0};p.observedMoves={10,45,52,232};
    p.count=1;p.steps[0]={G::Ruby,13,A::OfferedLearn,232,3,0,M::TM,0,G::Unknown,0,true,false,0};
    return R::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=sourceEvolution(G::FireRed);
    p.originalSpecies=4;p.observedSpecies=4;
    p.initialLevel=12;p.observedLevel=13;
    p.initialMoves={10,45,52,0};p.observedMoves={10,45,52,232};
    p.count=1;p.steps[0]={G::FireRed,13,A::OfferedLearn,232,3,0,M::TM,0,G::Unknown,0,true,false,0};
    return R::evaluate(p).status==S::CandidateConsistent;}());
int main() {
    for(G game:games) {
        const auto result=R::evaluate(sourceEvolution(game));
        assert(result.status==S::CandidateConsistent);
        assert(result.offers==2 && result.evolutions==1 &&
               result.machines==2 && result.finalSpecies==6);
    }
    std::cout<<"L3 exact retail full-table five-game source-bound joint evolution: PASS\n";
}
