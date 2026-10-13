#include "Legality/Gen56PkhexChronologyCore.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace C=Legality::Gen56PkhexChronologyCore;
using G=C::Game;using A=C::Action;using M=C::Method;using S=C::Status;
constexpr C::Snapshot snap(std::uint64_t id,G game,std::array<std::uint16_t,4> moves,
                           std::uint16_t species=1,std::uint8_t form=0,std::uint8_t lv=50){
 C::Snapshot s{};s.candidateId=70001;s.individualId=81001;s.snapshotId=id;
 s.game=game;s.species=species;s.form=form;s.level=lv;s.moves=moves;
 s.independentlyAuthenticated=true;return s;
}
constexpr C::Step action(C::Snapshot before,C::Snapshot after,A op,M method=M::None,
                         std::uint16_t move=0,std::uint8_t slot=0,std::uint8_t machine=0){
 C::Step s{};s.before=before;s.after=after;s.action=op;s.method=method;s.move=move;s.slot=slot;
 auto& p=s.proof;p.actionReceipt=after.snapshotId+100000;
 p.originalActionAuthenticated=p.sameIndividualAuthenticated=true;
 p.allIntermediateLevelDecisionsAuthenticated=true;
 p.exactSourceAuthenticated=p.speciesCompatibleAuthenticated=true;
 p.cartridgeOrServiceAuthenticated=p.transferRouteAuthenticated=true;
 p.destinationMarkerAuthenticated=p.fourSlotsPreservedAuthenticated=true;
 p.originalSourceRecord=1000000+after.snapshotId;
 p.exactGame=before.game;p.exactSpecies=before.species;p.exactForm=before.form;
 p.machineNumber=machine;return s;
}
constexpr C::Path full(){
 C::Path p{};p.stepCount=6;p.completeIntermediateSequenceAuthenticated=true;
 const std::array<std::uint16_t,4> start{33,45,73,22};
 const std::array<std::uint16_t,4> snarl{555,45,73,22};
 const std::array<std::uint16_t,4> afterHm{555,45,249,22};
 const auto s0=snap(1,G::HeartGold,start),s1=snap(2,G::Black,start);
 const auto s2=snap(3,G::Black2,start),s3=snap(4,G::Black2,snarl);
 const auto s4=snap(5,G::X,snarl),s5=snap(6,G::OmegaRuby,snarl);
 const auto s6=snap(7,G::OmegaRuby,afterHm);
 p.initial=s0;p.observed=s6;
 p.steps[0]=action(s0,s1,A::PokeTransfer4to5);
 p.steps[1]=action(s1,s2,A::VisitGame);
 p.steps[2]=action(s2,s3,A::Teach,M::TM,555,0,95);
 p.steps[3]=action(s3,s4,A::BankTransfer5to6);
 p.steps[4]=action(s4,s5,A::VisitGame);
 p.steps[5]=action(s5,s6,A::Teach,M::HM,249,2,6);
 return p;
}
static_assert(C::evaluate(full()).status==S::CandidateConsistent);
static_assert([]{auto p=full();p.steps[2].proof.exactGame=G::Black;return C::evaluate(p).status==S::Unknown;}());
static_assert([]{auto p=full();p.steps[2].proof.machineNumber=94;return C::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=full();p.steps[5].proof.machineNumber=7;return C::evaluate(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=full();p.steps[3].proof.transferRouteAuthenticated=false;return C::evaluate(p).status==S::Unknown;}());
static_assert(!C::evaluate(full()).globalInvalid);
int main(){
 std::uint64_t checks=0;
 for(unsigned i=0;i<6000;++i){
  auto p=full();
  assert(C::evaluate(p).status==S::CandidateConsistent);++checks;
  {auto x=p;x.steps[2].proof.machineNumber=94;
   assert(C::evaluate(x).status==S::CandidateContradicted);++checks;}
  {auto x=p;x.steps[2].proof.machineNumber=94;
   x.steps[5].proof.exactSourceAuthenticated=false;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[2].proof.speciesCompatibleAuthenticated=false;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[3].proof.fourSlotsPreservedAuthenticated=false;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[0].after.moves[0]=15;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[3].after.individualId++;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[4].after.snapshotId=x.steps[2].after.snapshotId;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[5].proof.actionReceipt=x.steps[0].proof.actionReceipt;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[1].after.level=49;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[3].after.moves[2]=99;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[5].after.moves[2]=73;
   x.observed=x.steps[5].after;
   assert(C::evaluate(x).status==S::CandidateContradicted);++checks;}
  {auto x=p;x.steps[5].after.moves={555,45,0,22};
   x.observed=x.steps[5].after;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[2].proof.originalSourceRecord=0;
   assert(C::evaluate(x).status==S::Unknown);++checks;}
  {auto x=p;x.steps[5].proof.transferRouteAuthenticated=false;
   assert(C::evaluate(x).status==S::CandidateConsistent);++checks;}
 }
 C::Alternatives alternatives{};alternatives.count=2;
 alternatives.paths[0]=full();alternatives.paths[0].steps[2].proof.machineNumber=94;
 alternatives.paths[1]=full();alternatives.paths[1].steps[5].proof.exactSourceAuthenticated=false;
 assert(C::evaluate(alternatives)==S::Unknown);++checks;
 alternatives.paths[1]=full();assert(C::evaluate(alternatives)==S::CandidateConsistent);++checks;
 alternatives.paths[1].steps[5].proof.machineNumber=7;
 alternatives.allCandidateHistoriesEnumerated=true;
 assert(C::evaluate(alternatives)==S::CandidateContradicted);++checks;
 alternatives.allCandidateHistoriesEnumerated=false;
 assert(C::evaluate(alternatives)==S::Unknown);++checks;
 std::cout<<"L3 Gen V/VI source-bound Poké Transfer + Bank chronology: "<<checks<<" scenario groups PASS\n";
}
