#include "Legality/Gen1RbyOriginalJointHistory.h"
#include <cassert>
#include <iostream>
namespace H=Legality::Gen1RbyOriginalJointHistory;
using G=H::Game;using A=H::Action;using M=H::Method;using S=H::Status;
constexpr H::Path base(G g){H::Path p{};p.candidate=123;p.individual=456;p.origin=p.observedGame=g;p.species=p.observedSpecies=25;p.firstLevel=5;p.lastLevel=5;p.firstMoves={84,45,0,0};p.lastMoves=p.firstMoves;p.sameIndividualVerified=p.firstSnapshotVerified=p.lastSnapshotVerified=p.completeChoicesVerified=true;return p;}
constexpr H::Step offer(G g,unsigned level,unsigned move,unsigned slot){return {g,(std::uint8_t)level,A::OfferLearn,(std::uint16_t)move,(std::uint8_t)slot,M::None,0,0,0,true,true};}
constexpr H::Step machine(G g,unsigned level,unsigned move,unsigned slot,M method,unsigned number,unsigned receipt){return {g,(std::uint8_t)level,A::Machine,(std::uint16_t)move,(std::uint8_t)slot,method,(std::uint8_t)number,receipt,0,true,true};}
constexpr H::Path yellow(){auto p=base(G::Yellow);p.lastLevel=15;p.stepCount=5;p.steps[0]=offer(G::Yellow,6,39,2);p.steps[1]=offer(G::Yellow,8,86,3);p.steps[2]=offer(G::Yellow,11,98,0);p.steps[3]=machine(G::Yellow,12,85,1,M::TM,24,11);p.steps[4]=offer(G::Yellow,15,104,2);p.lastMoves={98,85,104,86};return p;}
constexpr H::Path red(){auto p=base(G::Red);p.lastLevel=16;p.stepCount=2;p.steps[0]=offer(G::Red,9,86,2);p.steps[1]=offer(G::Red,16,98,3);p.lastMoves={84,45,86,98};return p;}
static_assert(H::evaluate(yellow()).status==S::Consistent);
static_assert(H::evaluate(red()).status==S::Consistent);
static_assert([]{auto p=red();p.origin=p.observedGame=G::Blue;for(auto& s:p.steps)if(s.sourceVerified)s.game=G::Blue;return H::evaluate(p).status==S::Consistent;}());
static_assert([]{auto p=yellow();p.steps[0].move=86;return H::evaluate(p).status==S::Contradicted;}());
static_assert([]{auto p=yellow();p.steps[3].receipt=0;return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=yellow();p.steps[3].machine=95;return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=yellow();p.steps[4].actionVerified=false;return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=red();p.origin=p.observedGame=G::Yellow;for(auto& s:p.steps)if(s.sourceVerified)s.game=G::Yellow;return H::evaluate(p).status==S::Contradicted;}());
constexpr H::Path trade(){auto p=red();p.lastLevel=17;p.stepCount=3;p.steps[2]={G::Yellow,17,A::Trade,0,0,M::None,0,0,0,true,true};p.observedGame=G::Yellow;return p;}
static_assert(H::evaluate(trade()).status==S::Consistent);
static_assert([]{auto p=trade();p.lastLevel=26;return H::evaluate(p).status==S::Contradicted;}());
constexpr H::Path raichu(){auto p=red();p.lastLevel=27;p.stepCount=3;p.steps[2]={G::Red,17,A::Evolve,0,0,M::None,0,0,26,true,true};p.observedSpecies=26;return p;}
static_assert(H::evaluate(raichu()).status==S::Consistent);
static_assert([]{auto p=raichu();p.steps[2].sourceVerified=false;return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=red();p.firstSnapshotVerified=false;return H::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=yellow();p.steps[3].move=87;return H::evaluate(p).status==S::Contradicted;}());
int main(){unsigned checks=0;for(unsigned i=0;i<8000;++i){auto y=yellow();assert(H::evaluate(y).status==S::Consistent);++checks;auto n=y;n.steps[3].move=87;assert(H::evaluate(n).status==S::Contradicted);++checks;n=y;n.steps[4].sourceVerified=false;assert(H::evaluate(n).status==S::Unknown);++checks;auto r=red();assert(H::evaluate(r).status==S::Consistent);++checks;n=r;n.lastLevel=17;n.steps[1].level=17;if(H::evaluate(n).status!=S::Contradicted){std::cerr<<"wrong level status "<<int(H::evaluate(n).status)<<"\n";}assert(H::evaluate(n).status==S::Contradicted);++checks;n=raichu();assert(H::evaluate(n).status==S::Consistent);++checks;}std::cout<<"L3 Gen I RBY/Y source-bound four-slot replay "<<checks<<" assertions PASS\n";}
