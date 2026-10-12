#include "Legality/Gen14CandidateMoveChronology.h"
#include <cassert>
#include <iostream>
namespace L=Legality::Gen14CandidateMoveChronology;
using Game=L::Game;using A=L::Action;using M=L::Method;using S=L::Status;

constexpr L::Path rubyBulbasaur() {
    L::Path p{};
    p.candidateId=701;p.species=1;p.origin=Game::Ruby;
    p.initialLevel=5;p.observedLevel=10;
    p.initialMoves={33,45,0,0};
    p.observedMoves={33,45,73,22};
    p.initialSnapshotVerified=true;
    p.samePokemonAndSpeciesVerified=true;
    p.fullChronologyVerified=true;
    p.stepCount=2;
    // The retail Ruby Bulbasaur table offers Leech Seed (73) at 7 and
    // Vine Whip (22) at 10. These are synthetic *candidate* snapshots.
    p.steps[0]={Game::Ruby,7,A::Learn,M::LevelUp,73,2,true,Game::Ruby};
    p.steps[1]={Game::Ruby,10,A::Learn,M::LevelUp,22,3,true,Game::Ruby};
    return p;
}
constexpr auto ruby=rubyBulbasaur();
static_assert(L::evaluate(ruby).status==S::CandidateConsistent);
static_assert(L::evaluate(ruby).applied==2);

constexpr L::Path gbaFive(Game g) {
    auto p=ruby;p.origin=g;
    p.steps[0].game=g;p.steps[0].methodSource=g;
    p.steps[1].game=g;p.steps[1].methodSource=g;
    return p;
}
// Pinned Ruby, Emerald and FRLG original level-up learnsets independently
// corroborate this particular lv7/lv10 Bulbasaur sequence. Both cartridge
// identities within each shared RS or FRLG source family are preserved.
static_assert(L::evaluate(gbaFive(Game::Sapphire)).status==S::CandidateConsistent);
static_assert(L::evaluate(gbaFive(Game::Emerald)).status==S::CandidateConsistent);
static_assert(L::evaluate(gbaFive(Game::FireRed)).status==S::CandidateConsistent);
static_assert(L::evaluate(gbaFive(Game::LeafGreen)).status==S::CandidateConsistent);

// Synthetic verified snapshots at lv40: Bulbasaur has no mandatory
// level-up offers between 40 and 45 in the pinned GBA tables.
constexpr L::Path fullSlots() {
    auto p=ruby;p.initialLevel=40;p.observedLevel=45;
    p.initialMoves={33,45,73,22};
    p.observedMoves={33,45,92,15};p.stepCount=2;
    p.steps[0]={Game::Ruby,41,A::Learn,M::TM,92,2,true,Game::Ruby};
    p.steps[1]={Game::Ruby,43,A::Learn,M::HM,15,3,true,Game::Ruby};
    return p;
}
static_assert(L::evaluate(fullSlots()).status==S::CandidateConsistent);
static_assert([]{auto p=fullSlots();p.steps[0].methodSource=Game::Emerald;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=fullSlots();p.steps[0].evidenceVerified=false;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=fullSlots();p.steps[0].slot=1;
    p.initialMoves={33,45,0,0};return L::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=fullSlots();p.steps[1].move=92;
    return L::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=fullSlots();p.steps[1].game=Game::Emerald;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=fullSlots();p.steps[1].level=40;
    return L::evaluate(p).status==S::Unresolved;}());

constexpr L::Path replayTrade() {
    auto p=fullSlots();p.observedMoves={33,45,92,237};p.stepCount=3;
    p.steps[1]={Game::Emerald,42,A::VisitGame,M::LevelUp,0,0,true,Game::Unknown};
    p.steps[2]={Game::Emerald,44,A::Learn,M::TM,237,3,true,Game::Emerald};
    return p;
}
static_assert(L::evaluate(replayTrade()).status==S::CandidateConsistent);
static_assert([]{auto p=replayTrade();p.steps[1].evidenceVerified=false;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=replayTrade();p.steps[2].method=M::Tutor;
    p.steps[2].evidenceVerified=false;
    return L::evaluate(p).status==S::Unresolved;}());

constexpr L::Path palpark() {
    auto p=fullSlots();p.observedMoves={33,45,92,57};p.stepCount=3;
    p.steps[1]={Game::Platinum,42,A::PalPark,M::LevelUp,0,0,true,Game::Unknown};
    p.steps[2]={Game::Platinum,44,A::Learn,M::HM,57,3,true,Game::Platinum};
    return p;
}
static_assert(L::evaluate(palpark()).status==S::CandidateConsistent);
static_assert(L::evaluate(palpark()).transferred);
static_assert([]{auto p=palpark();p.initialMoves={33,57,73,22};
    return L::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=palpark();p.steps[2].game=Game::Ruby;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=palpark();p.steps[1].game=Game::Emerald;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=palpark();p.steps[1].evidenceVerified=false;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=palpark();p.steps[2].methodSource=Game::Diamond;
    return L::evaluate(p).status==S::Unresolved;}());

constexpr L::Path relearn() {
    auto p=fullSlots();p.observedMoves={33,45,22,92};p.stepCount=2;
    p.steps[0]={Game::Ruby,41,A::DeleteMove,M::LevelUp,0,2,true,Game::Unknown};
    p.steps[1]={Game::Ruby,43,A::Learn,M::Reminder,92,3,true,Game::Ruby};
    return p;
}
static_assert(L::evaluate(relearn()).status==S::CandidateConsistent);
static_assert(L::evaluate(relearn()).deleted==1);
static_assert([]{auto p=relearn();p.steps[0].slot=3;
    return L::evaluate(p).status==S::CandidateConsistent;}() == false);
static_assert([]{auto p=relearn();p.steps[1].evidenceVerified=false;
    return L::evaluate(p).status==S::Unresolved;}());

// Bulbasaur's source-attested next offer after level 40 is SolarBeam at
// level 46. At four moves, declining the offer is a legitimate decision.
constexpr L::Path lateOffer() {
    auto p=fullSlots();p.observedLevel=46;
    p.observedMoves=p.initialMoves;p.stepCount=1;
    p.steps[0]={Game::Ruby,46,A::Decline,M::LevelUp,76,0,true,Game::Ruby};
    return p;
}
static_assert(L::evaluate(lateOffer()).status==S::CandidateConsistent);
static_assert([]{auto p=lateOffer();p.initialMoves={33,45,0,0};
    p.observedMoves=p.initialMoves;
    return L::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=lateOffer();p.steps[0].action=A::AlreadyKnown;
    return L::evaluate(p).status==S::CandidateContradicted;}());

constexpr L::Paths branches=[] {
    L::Paths p{};p.count=2;p.allCandidateHistoriesEnumerated=true;
    p.alternatives[0]=palpark();p.alternatives[0].initialMoves={33,57,73,22};
    p.alternatives[1]=palpark();return p;
}();
static_assert(L::evaluate(branches).status==S::CandidateConsistent);
static_assert([]{auto p=branches;p.alternatives[1].steps[1].evidenceVerified=false;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=branches;p.alternatives[1]=p.alternatives[0];
    return L::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=branches;p.alternatives[1]=p.alternatives[0];
    p.allCandidateHistoriesEnumerated=false;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=branches;p.alternatives[1].candidateId=999;
    return L::evaluate(p).status==S::Unresolved;}());
int main() {
    assert(L::evaluate(replayTrade()).visits==1);
    assert(L::evaluate(palpark()).visits==1);
    std::cout<<"L3 candidate GBA / Gen IV four-slot chronology: PASS\n";
}
