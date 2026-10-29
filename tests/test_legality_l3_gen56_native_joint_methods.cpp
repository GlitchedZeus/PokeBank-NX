#include "Legality/Gen56PkhexChronologyCore.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace C=Legality::Gen56PkhexChronologyCore;
using G=C::Game;using A=C::Action;using M=C::Method;using S=C::Status;
constexpr C::Snapshot s(std::uint64_t id,G g,std::uint16_t sp,std::uint8_t lv,
                        std::array<std::uint16_t,4> moves){
 C::Snapshot x{};x.candidateId=991;x.individualId=998;x.snapshotId=id;x.game=g;
 x.species=sp;x.level=lv;x.moves=moves;x.independentlyAuthenticated=true;return x;
}
constexpr C::Step step(C::Snapshot before,C::Snapshot after,A op,M method=M::None,
                       std::uint16_t move=0,std::uint8_t slot=0,std::uint8_t number=0){
 C::Step x{};x.before=before;x.after=after;x.action=op;x.method=method;
 x.move=move;x.slot=slot;x.proof.actionReceipt=700+after.snapshotId;
 x.proof.originalSourceRecord=800+after.snapshotId;
 x.proof.originalActionAuthenticated=x.proof.sameIndividualAuthenticated=true;
 x.proof.exactSourceAuthenticated=x.proof.speciesCompatibleAuthenticated=true;
 x.proof.cartridgeOrServiceAuthenticated=x.proof.allIntermediateLevelDecisionsAuthenticated=true;
 x.proof.transferRouteAuthenticated=x.proof.fourSlotsPreservedAuthenticated=true;
 x.proof.destinationMarkerAuthenticated=true;
 x.proof.exactGame=before.game;x.proof.exactSpecies=before.species;
 x.proof.exactForm=before.form;x.proof.machineNumber=number;return x;
}
// Explicit hypothetical, NOT a real certified retail Bulbasaur evolution:
// move offers and visits become positive only with independently witnessed
// original move-service/evolution decisions bound into the source records.
constexpr C::Path evolve(){
 C::Path p{};p.completeIntermediateSequenceAuthenticated=true;p.stepCount=5;
 const std::array<std::uint16_t,4> orig{33,45,73,22};
 const std::array<std::uint16_t,4> taught{33,45,73,92};
 const std::array<std::uint16_t,4> cut{33,45,73,0};
 auto b=s(10,G::Black2,1,15,orig);auto a=s(11,G::Black2,2,16,orig);
 auto c=s(12,G::Black2,2,16,taught);auto d=s(13,G::Black2,2,16,cut);
 auto e=s(14,G::White2,2,16,cut);auto f=s(15,G::White2,2,17,cut);
 p.initial=b;p.observed=f;
 p.steps[0]=step(b,a,A::Evolve);
 p.steps[1]=step(a,c,A::Teach,M::TM,92,3,6);
 p.steps[2]=step(c,d,A::DeleteMove,M::None,0,3);
 p.steps[3]=step(d,e,A::VisitGame);
 p.steps[4]=step(e,f,A::OfferAlreadyKnown,M::LevelUp,45);
 return p;
}
static_assert(C::evaluate(evolve()).status==S::CandidateConsistent);
static_assert([]{auto x=evolve();x.steps[1].proof.exactSpecies=1;return C::evaluate(x).status==S::Unknown;}());
static_assert([]{auto x=evolve();x.steps[4].proof.allIntermediateLevelDecisionsAuthenticated=false;return C::evaluate(x).status==S::Unknown;}());
static_assert([]{auto x=evolve();x.steps[1].proof.machineNumber=7;return C::evaluate(x).status==S::CandidateContradicted;}());
static_assert([]{auto x=evolve();x.steps[3].before.game=G::X;return C::evaluate(x).status==S::Unknown;}());
[[gnu::noinline]] C::Path runtimePath(){ volatile unsigned k=0; auto p=evolve(); if(k) p.initial.level=0; return p; }
int main(){
 std::uint64_t checks=0;
 for(int i=0;i<5000;++i){
  const auto p=runtimePath();
  assert(C::evaluate(p).status==S::CandidateConsistent);++checks;
  {auto x=p;x.steps[0].after.moves[0]=35;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[0].after.species=3;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[1].proof.machineNumber=7;
   assert(C::evaluate(x).status==S::CandidateContradicted);++checks;}
  {auto x=p;x.steps[1].proof.machineNumber=7;
   x.steps[4].proof.exactSourceAuthenticated=false;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[4].method=M::Tutor;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[4].move=11;
   assert(C::evaluate(x).status==S::CandidateContradicted);++checks;}
  {auto x=p;x.steps[2].slot=9;
   assert(C::evaluate(x).status==S::CandidateContradicted);++checks;}
  {auto x=p;x.steps[2].proof.cartridgeOrServiceAuthenticated=false;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[3].after.game=G::X;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[0].proof.originalSourceRecord=0;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.observed.moves={33,45,73,22};
   assert(C::evaluate(x).status==S::Unknown);++checks;}
 }
 // A detached Gen V source cannot prove the same Pokémon also visited Gen VI.
 {auto p=runtimePath();p.completeIntermediateSequenceAuthenticated=false;
  assert(C::evaluate(p).status==S::Unknown);++checks;}
 std::cout<<"L3 Gen V/VI exact source-stage evolution/teaching history: "<<checks<<" scenario groups PASS\n";
}
