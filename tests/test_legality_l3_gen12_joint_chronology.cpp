#include "Legality/Gen12CandidateMoveJointChronology.h"
#include <cassert>
#include <iostream>
namespace H=Legality::Gen12CandidateMoveJointChronology;
using G=H::Game;using A=H::Action;using S=H::Status;
constexpr H::Path bulba(G g,uint8_t end) {
    H::Path p{};p.candidateId=913;p.species=1;p.origin=g;p.observedGame=g;
    p.initialLevel=5;p.observedLevel=end;
    p.initialMoves={33,45,0,0};p.observedMoves=p.initialMoves;
    p.originalSnapshotVerified=true;p.samePokemonAndSpeciesVerified=true;
    p.fullVisibleHistoryVerified=true;
    return p;
}
constexpr H::Path redOrder() {
    auto p=bulba(G::Red,13);p.stepCount=3;
    p.steps[0]={G::Red,7,A::OfferLearn,73,2,0,true};
    p.steps[1]={G::Red,8,A::TeachTM,38,3,10,true};
    p.steps[2]={G::Red,13,A::OfferLearn,22,0,0,true};
    p.observedMoves={22,45,73,38};return p;
}
static_assert(H::evaluate(redOrder()).status==S::CandidateConsistent);
static_assert(H::evaluate(redOrder()).offers==2);
static_assert(H::evaluate(redOrder()).machines==1);
static_assert([]{auto p=redOrder();p.steps[0]=p.steps[1];p.stepCount=1;
    p.observedLevel=8;p.observedMoves={33,45,38,0};
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=redOrder();p.steps[1].tmNumber=6;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=redOrder();p.steps[1].actionSourceVerified=false;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=redOrder();p.steps[1].tmNumber=40;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=redOrder();p.samePokemonAndSpeciesVerified=false;
    return H::evaluate(p).status==S::Unknown;}());
// Red, Blue and Yellow share this complete Bulbasaur retail table.
static_assert([]{auto p=redOrder();p.origin=G::Blue;p.observedGame=G::Blue;
    for(auto& s:p.steps)if(s.actionSourceVerified)s.game=G::Blue;
    return H::evaluate(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=redOrder();p.origin=G::Yellow;p.observedGame=G::Yellow;
    for(auto& s:p.steps)if(s.actionSourceVerified)s.game=G::Yellow;
    return H::evaluate(p).status==S::CandidateConsistent;}());
constexpr H::Path crystalDoubleOffer() {
    auto p=bulba(G::Crystal,15);p.stepCount=4;
    p.steps[0]={G::Crystal,7,A::OfferLearn,73,2,0,true};
    p.steps[1]={G::Crystal,10,A::OfferLearn,22,3,0,true};
    p.steps[2]={G::Crystal,15,A::OfferLearn,77,0,0,true};
    p.steps[3]={G::Crystal,15,A::OfferDecline,79,0,0,true};
    p.observedMoves={77,45,73,22};return p;
}
static_assert(H::evaluate(crystalDoubleOffer()).status==S::CandidateConsistent);
static_assert(H::evaluate(crystalDoubleOffer()).offers==4);
static_assert([]{auto p=crystalDoubleOffer();p.stepCount=3;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=crystalDoubleOffer();p.steps[3].action=A::OfferLearn;
    p.steps[3].move=79;
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=crystalDoubleOffer();p.origin=G::Gold;p.observedGame=G::Gold;
    for(auto& s:p.steps)if(s.actionSourceVerified)s.game=G::Gold;
    return H::evaluate(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=crystalDoubleOffer();p.origin=G::Silver;p.observedGame=G::Silver;
    for(auto& s:p.steps)if(s.actionSourceVerified)s.game=G::Silver;
    return H::evaluate(p).status==S::CandidateConsistent;}());
// A Gen II-only move cannot be retained through a verified Gen I entry,
// but may be independently reacquired on the Gen II return leg.
constexpr H::Path capsule() {
    auto p=bulba(G::Crystal,11);p.stepCount=4;
    p.retailTimeCapsulePolicyVerified=true;p.speciesAtGen1Entry=1;
    p.steps[0]={G::Crystal,7,A::OfferLearn,73,2,0,true};
    p.steps[1]={G::Red,8,A::ToGen1,0,0,0,true};
    p.steps[2]={G::Crystal,10,A::ReturnToGen2,0,0,0,true};
    p.steps[3]={G::Crystal,11,A::TeachTM,237,3,10,true};
    p.observedMoves={33,45,73,237};return p;
}
static_assert(H::evaluate(capsule()).status==S::CandidateConsistent);
static_assert(H::evaluate(capsule()).visitedGen1 && H::evaluate(capsule()).returnedToGen2);
static_assert([]{auto p=capsule();p.stepCount=5;
    p.steps[1]={G::Crystal,8,A::TeachTM,237,3,10,true};
    p.steps[2]={G::Red,8,A::ToGen1,0,0,0,true};
    p.steps[3]={G::Crystal,10,A::ReturnToGen2,0,0,0,true};
    p.steps[4]={G::Crystal,11,A::TeachTM,92,0,6,true};
    p.observedMoves={92,45,73,237};
    return H::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=capsule();p.retailTimeCapsulePolicyVerified=false;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=capsule();p.speciesAtGen1Entry=2;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=capsule();p.steps[2].game=G::Gold;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=capsule();p.steps[3].tmNumber=8;
    return H::evaluate(p).status==S::Unknown;}());
// A source-attested original Gen I -> Gen II transfer is distinct from
// same-generation trades and a Gen II -> Gen I -> Gen II round trip.
constexpr H::Path forward() {
    auto p=bulba(G::Red,10);p.stepCount=3;
    p.retailTimeCapsulePolicyVerified=true;p.observedGame=G::Crystal;
    p.steps[0]={G::Red,7,A::OfferLearn,73,2,0,true};
    p.steps[1]={G::Crystal,8,A::ToGen2,0,0,0,true};
    p.steps[2]={G::Crystal,10,A::OfferLearn,22,3,0,true};
    p.observedMoves={33,45,73,22};return p;
}
static_assert(H::evaluate(forward()).status==S::CandidateConsistent);
static_assert([]{auto p=forward();p.observedGame=G::Red;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=forward();p.retailTimeCapsulePolicyVerified=false;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=forward();p.steps[1].action=A::VisitGame;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=bulba(G::Crystal,10);p.stepCount=2;
    p.steps[0]={G::Crystal,7,A::OfferLearn,73,2,0,true};
    p.steps[1]={G::Crystal,10,A::OfferLearn,22,3,0,true};
    p.observedMoves={33,45,73,22};
    return H::evaluate(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=bulba(G::Crystal,10);p.species=155;
    p.initialMoves={33,43,0,0};p.observedMoves=p.initialMoves;
    return H::evaluate(p).status==S::CandidateContradicted;}()); // omitted lv6 offer
static_assert([]{auto p=bulba(G::Crystal,10);p.species=155;
    p.initialMoves={33,43,0,0};p.observedMoves={33,43,108,0};
    p.stepCount=1;p.steps[0]={G::Crystal,6,A::OfferLearn,108,2,0,true};
    return H::evaluate(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=capsule();p.originalSnapshotVerified=false;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=capsule();p.species=169;
    return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto a=capsule();a.steps[3].actionSourceVerified=false;
    H::Alternatives paths{};paths.count=2;paths.allGenuineRoutesEnumerated=true;
    paths.paths[0]=capsule();paths.paths[0].observedMoves={33,45,73,92};
    paths.paths[1]=a;
    return H::evaluate(paths)==S::Unknown;}());
static_assert([]{H::Alternatives a{};a.count=1;a.paths[0]=capsule();
    return H::evaluate(a)==S::CandidateConsistent;}());
int main() {
    assert(H::evaluate(redOrder()).status==S::CandidateConsistent);
    assert(H::evaluate(crystalDoubleOffer()).status==S::CandidateConsistent);
    assert(H::evaluate(capsule()).status==S::CandidateConsistent);
    std::cout << "L3 Gen I/II source-order and directional tradeback: PASS\n";
}
