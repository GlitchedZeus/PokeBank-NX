#include "Legality/Gen14CandidateMoveJointAcquisition.h"
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
// Distinct original retail source tables: at level 13 Charmander gets
// Smokescreen (108) in R/S/E but Metal Claw (232) in FR/LG. An independently
// attested level-up method must not be applied across those versions.
constexpr L::Path charmander(Game g,uint16_t move) {
    L::Path p{};p.candidateId=704;p.species=4;p.origin=g;
    p.initialLevel=12;p.observedLevel=13;
    p.initialMoves={10,45,52,0};p.observedMoves={10,45,52,move};
    p.initialSnapshotVerified=true;p.samePokemonAndSpeciesVerified=true;
    p.fullChronologyVerified=true;p.stepCount=1;
    p.steps[0]={g,13,A::Learn,M::LevelUp,move,3,true,g};
    return p;
}
static_assert(L::evaluate(charmander(Game::Ruby,108)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(Game::Sapphire,108)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(Game::Emerald,108)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(Game::FireRed,232)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(Game::LeafGreen,232)).status==S::CandidateConsistent);
static_assert(L::evaluate(charmander(Game::Ruby,232)).status==S::CandidateContradicted);
static_assert(L::evaluate(charmander(Game::FireRed,108)).status==S::CandidateContradicted);
static_assert([]{auto p=charmander(Game::FireRed,232);
    p.steps[0].evidenceVerified=false;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert(L::sourcePinnedLevelOffer(Game::Ruby,133,13,108)==
    L::PinnedOffer::Unmodeled); // Other species not falsely rejected.

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

// Cut (15), unlike Surf, is a Bulbasaur HM-compatible move in both
// pinned RS/FRLG TMHM tables; migration still requires deletion and
// independently evidenced Gen IV HM teaching after arrival.
constexpr L::Path palpark() {
    auto p=fullSlots();p.observedMoves={33,45,92,15};p.stepCount=3;
    p.steps[1]={Game::Platinum,42,A::PalPark,M::LevelUp,0,0,true,Game::Unknown};
    p.steps[2]={Game::Platinum,44,A::Learn,M::HM,15,3,true,Game::Platinum};
    return p;
}
static_assert(L::evaluate(palpark()).status==S::CandidateConsistent);
static_assert(L::evaluate(palpark()).transferred);
static_assert([]{auto p=palpark();p.initialMoves={33,15,73,22};
    return L::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=palpark();p.steps[2].game=Game::Ruby;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=palpark();p.steps[1].game=Game::Emerald;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=palpark();p.steps[1].evidenceVerified=false;
    return L::evaluate(p).status==S::Unresolved;}());
static_assert([]{auto p=palpark();p.steps[2].methodSource=Game::Diamond;
    return L::evaluate(p).status==S::Unresolved;}());

// Leech Seed (73) is an actual Ruby Bulbasaur level-7 move, unlike TM
// Toxic: the proposed Move Reminder method concerns a past level-up move.
constexpr L::Path relearn() {
    auto p=fullSlots();p.observedMoves={33,45,22,73};p.stepCount=2;
    p.steps[0]={Game::Ruby,41,A::DeleteMove,M::LevelUp,0,2,true,Game::Unknown};
    p.steps[1]={Game::Ruby,43,A::Learn,M::Reminder,73,3,true,Game::Ruby};
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
    p.alternatives[0]=palpark();p.alternatives[0].initialMoves={33,15,73,22};
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
// Integration gate: all target moves are individually GBA-source compatible,
// but their stated time/slot sequence may be impossible. This test does NOT
// attempt to exclude different evolution/trade/egg/event/transfer histories.
namespace J=Legality::Gen14CandidateMoveJointAcquisition;
namespace ME=Legality::Gen14CandidateMoveMachineEvidence;
constexpr J::Input jointBulbasaur() {
    J::Input in{};in.path=rubyBulbasaur();
    in.uninterruptedSameSpeciesGameStageVerified=true;
    in.allOfferedLevelUpDecisionsRecorded=true;
    return in;
}
static_assert(L::evaluate(jointBulbasaur().path).status==S::CandidateConsistent);
static_assert(J::evaluate(jointBulbasaur()).status==S::CandidateConsistent);
constexpr J::Input skippedMandatoryOffers() {
    auto in=jointBulbasaur();
    in.path.observedMoves={33,45,92,237};
    // Individually both Toxic and Hidden Power are Bulbasaur GBA TMs,
    // but their acquisition cannot replace the lv7/lv10 forced offers
    // without an observed decline/replacement decision.
    in.path.steps[0]={Game::Ruby,7,A::Learn,M::TM,92,2,true,Game::Ruby};
    in.path.steps[1]={Game::Ruby,10,A::Learn,M::TM,237,3,true,Game::Ruby};
    in.machineCount=2;
    in.machines[0]={0,{Game::Ruby,1,ME::Machine::TM,6,92,true,true}};
    in.machines[1]={1,{Game::Ruby,1,ME::Machine::TM,10,237,true,true}};
    return in;
}
static_assert(L::evaluate(skippedMandatoryOffers().path).status==S::CandidateConsistent);
static_assert(J::evaluate(skippedMandatoryOffers()).status==S::CandidateContradicted);
constexpr J::Input jointFourMoves() {
    auto in=jointBulbasaur();
    auto& p=in.path;p.initialLevel=40;p.observedLevel=46;
    p.initialMoves={33,45,73,22};
    p.observedMoves={92,237,15,76};
    p.stepCount=4;
    p.steps[0]={Game::Ruby,41,A::Learn,M::TM,92,0,true,Game::Ruby};
    p.steps[1]={Game::Ruby,43,A::Learn,M::TM,237,1,true,Game::Ruby};
    p.steps[2]={Game::Ruby,44,A::Learn,M::HM,15,2,true,Game::Ruby};
    p.steps[3]={Game::Ruby,46,A::Learn,M::LevelUp,76,3,true,Game::Ruby};
    in.machineCount=3;
    in.machines[0]={0,{Game::Ruby,1,ME::Machine::TM,6,92,true,true}};
    in.machines[1]={1,{Game::Ruby,1,ME::Machine::TM,10,237,true,true}};
    in.machines[2]={2,{Game::Ruby,1,ME::Machine::HM,1,15,true,true}};
    return in;
}
static_assert(J::evaluate(jointFourMoves()).status==S::CandidateConsistent);
static_assert(J::evaluate(jointFourMoves()).requiredOffers==1);
static_assert(J::evaluate(jointFourMoves()).verifiedMachines==3);
static_assert([]{auto a=jointFourMoves();
    a.path.steps[2].level=46;
    return L::evaluate(a.path).status==S::CandidateConsistent &&
           J::evaluate(a).status==S::CandidateContradicted;}());
// An independently attested item number mismatch contradicts only this
// route; an absent receipt makes this candidate Unknown, not Invalid.
static_assert([]{auto a=jointFourMoves();
    a.machines[0].numberedItem.machineNumber=10;
    return J::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=jointFourMoves();a.machineCount=2;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=jointFourMoves();a.machineCount=4;
    a.machines[3]=a.machines[2];
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=jointFourMoves();
    a.allOfferedLevelUpDecisionsRecorded=false;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=jointFourMoves();
    a.uninterruptedSameSpeciesGameStageVerified=false;
    return J::evaluate(a).status==S::Unresolved;}());
// A reminder or tutor route may be legal but needs its own method witness:
// do not incorrectly reject it just because this joint check is narrower.
static_assert([]{auto a=jointFourMoves();
    a.path.initialLevel=40;a.path.observedLevel=45;
    a.path.initialMoves={33,45,22,77};
    a.path.observedMoves={33,45,22,73};
    a.path.stepCount=1;
    a.path.steps[0]={Game::Ruby,43,A::Learn,M::Reminder,73,3,true,Game::Ruby};
    a.machineCount=0;
    return L::evaluate(a.path).status==S::CandidateConsistent &&
           J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=jointBulbasaur();
    a.path.origin=Game::Ruby;a.path.species=133;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=jointBulbasaur();
    a.path.samePokemonAndSpeciesVerified=false;
    return J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=jointBulbasaur();
    a.path.stepCount=3;
    a.path.steps[2]={Game::Emerald,10,A::VisitGame,M::LevelUp,0,0,true,Game::Unknown};
    return L::evaluate(a.path).status==S::CandidateConsistent &&
           J::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=jointBulbasaur();
    a.path.stepCount=3;
    a.path.steps[2]={Game::Diamond,10,A::PalPark,M::LevelUp,0,0,true,Game::Unknown};
    return L::evaluate(a.path).status==S::CandidateConsistent &&
           J::evaluate(a).status==S::Unresolved;}());
// The previous four-slot-only candidate can be "consistent" without any
// source-offer action; source order independently disproves this particular
// claimed complete history, not any unenumerated alternative.
static_assert([]{auto a=jointFourMoves();
    a.path.observedMoves=a.path.initialMoves;
    a.path.stepCount=0;a.machineCount=0;
    return L::evaluate(a.path).status==S::CandidateConsistent &&
           J::evaluate(a).status==S::CandidateContradicted;}());

// A hypothetical initial move from an independently evidenced egg/event
// stage may already be known when its source level-up offer occurs. The
// replay checks the response, not whether a parent/event existed.
static_assert([]{auto a=jointBulbasaur();
    a.path.initialMoves={33,45,73,0};
    a.path.observedMoves={33,45,73,22};
    a.path.steps[0]={Game::Ruby,7,A::AlreadyKnown,M::LevelUp,73,0,true,Game::Ruby};
    a.path.steps[1]={Game::Ruby,10,A::Learn,M::LevelUp,22,3,true,Game::Ruby};
    return J::evaluate(a).status==S::CandidateConsistent;}());
// Missing original egg/event snapshot is NOT permission to conclude the
// four moves globally incompatible.
static_assert([]{auto a=jointBulbasaur();
    a.path.initialSnapshotVerified=false;
    return J::evaluate(a).status==S::Unresolved;}());
// Original staged evolution is a distinct candidate route. It cannot be
// excluded by an uninterrupted same-species mechanism.
static_assert([]{auto a=jointBulbasaur();
    a.path.samePokemonAndSpeciesVerified=false;
    return J::evaluate(a).status==S::Unresolved;}());
// A source-complete contradictory path does not exhaust another plausible
// unverified parent/trade/reminder history with the same candidate identity.
static_assert([]{auto first=skippedMandatoryOffers();
    auto other=jointBulbasaur();
    other.path.initialSnapshotVerified=false;
    return J::evaluate(first).status==S::CandidateContradicted &&
           J::evaluate(other).status==S::Unresolved;}());

int main() {
    assert(J::evaluate(jointFourMoves()).status==S::CandidateConsistent);
    assert(J::evaluate(skippedMandatoryOffers()).status==S::CandidateContradicted);
    assert(L::evaluate(replayTrade()).visits==1);
    assert(L::evaluate(palpark()).visits==1);
    std::cout<<"L3 candidate GBA / Gen IV four-slot chronology: PASS\n";
}
