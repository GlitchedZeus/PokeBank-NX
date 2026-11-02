// Real pinned DP 493 species / HGSS 493 species / Platinum 12 species.
// This file MUST compile on the actual original full-data Git checkout.
#include "Legality/Gen4NativeOriginalHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace N=Legality::Gen4NativeOriginalHistory;
namespace H=Legality::Gen4NativeOriginalHistoryCore;
namespace D=Legality::Gen4NativeOriginalSourceData;
using G=H::Game;using A=H::Action;using S=H::Status;
constexpr H::History starter(G g){
 H::History x{};x.candidate=17420;x.species=1;x.origin=x.observedGame=g;
 x.initialLevel=5;x.observedLevel=9;
 x.initialMoves={33,45,0,0};x.observedMoves={33,45,73,22};
 x.originalSnapshotAttested=x.samePokemonAttested=x.everyIntermediateActionAttested=true;
 x.count=2;
 x.steps[0]={g,7,A::LevelLearn,73,2,0,0,true,true,false};
 x.steps[1]={g,9,A::LevelLearn,22,3,0,0,true,true,false};
 return x;
}
constexpr H::History plain(G g){
 auto x=starter(g);x.initialLevel=x.observedLevel=5;x.initialMoves=x.observedMoves={33,45,0,0};x.count=0;return x;
}
static_assert(N::evaluate(starter(G::Diamond)).status==S::CandidateConsistent);
static_assert(N::evaluate(starter(G::Pearl)).status==S::CandidateConsistent);
static_assert(N::evaluate(starter(G::Platinum)).status==S::CandidateConsistent);
static_assert([] { auto x=plain(G::HeartGold);x.count=1;
 x.steps[0]={G::HeartGold,5,A::Tutor,29,2,0,0,true,true,false};
 x.observedMoves={33,45,29,0};return N::evaluate(x).status==S::CandidateConsistent; }());
static_assert([] { auto x=plain(G::Platinum);x.count=1;
 x.steps[0]={G::Platinum,5,A::TM,412,2,53,9001,true,true,false};
 x.observedMoves={33,45,412,0};return N::evaluate(x).status==S::CandidateConsistent; }());
static_assert([] { auto x=plain(G::Diamond);x.count=1;
 x.steps[0]={G::Diamond,5,A::TM,92,2,6,9002,true,true,false};
 x.observedMoves={33,45,92,0};return N::evaluate(x).status==S::Unknown; }());
static_assert([] { auto x=plain(G::SoulSilver);x.count=1;
 x.steps[0]={G::SoulSilver,5,A::Tutor,29,2,0,0,true,true,false};
 x.observedLevel=6;x.observedMoves={33,45,29,0};
 return N::evaluate(x).status==S::Unknown; }());
static_assert(!N::Original::catalog(G::Diamond,354).orderedLevelSourceComplete);
static_assert(!N::Original::catalog(G::Platinum,386).tutorMoves.empty());
static_assert(!N::Original::catalog(G::HeartGold,386).orderedLevelSourceComplete);
int main(){
 std::uint64_t originalOffers=0,dpMachines=0,hgMachines=0,hgTutors=0,checks=0;
 unsigned dpReady=0,hgMethod=0,platReady=0;
 for(std::uint16_t sp=1;sp<=493;++sp){
  const auto dp=N::Original::catalog(G::Diamond,sp);
  const auto pearl=N::Original::catalog(G::Pearl,sp);
  const auto hg=N::Original::catalog(G::HeartGold,sp);
  const auto ss=N::Original::catalog(G::SoulSilver,sp);
  const auto pt=N::Original::catalog(G::Platinum,sp);
  assert(H::validSource(dp)&&H::validSource(pearl)&&H::validSource(hg)&&H::validSource(ss)&&H::validSource(pt));checks+=5;
  assert(dp.orderedLevelSourceComplete==pearl.orderedLevelSourceComplete);
  assert(dp.packedOffers.size()==pearl.packedOffers.size());
  assert(dp.machineLow==pearl.machineLow&&dp.machineHigh==pearl.machineHigh);
  assert(hg.machineLow==ss.machineLow&&hg.machineHigh==ss.machineHigh);
  assert(hg.tutorMask==ss.tutorMask);checks+=5;
  if(dp.orderedLevelSourceComplete)++dpReady;
  if(hg.tutorSourceComplete)++hgMethod;
  if(pt.orderedLevelSourceComplete)++platReady;
  originalOffers+=D::offerCount(sp);
  for(std::uint8_t tm=1;tm<=92;++tm){
   dpMachines+=H::machineCompatible(dp,A::TM,tm)?1:0;
   hgMachines+=H::machineCompatible(hg,A::TM,tm)?1:0;
  }
  for(std::uint8_t hm=1;hm<=8;++hm){
   dpMachines+=H::machineCompatible(dp,A::HM,hm)?1:0;
   hgMachines+=H::machineCompatible(hg,A::HM,hm)?1:0;
  }
  for(std::uint16_t i=0;i<D::kHeartGoldSoulSilverTutorMoves.size();++i)
   hgTutors+=((hg.tutorMask>>i)&1ULL)?1:0;
  if(sp==354){
   assert(!dp.orderedLevelSourceComplete);
   // Original DP Banette row contains an out-of-order level-up line:
   // preserve original data, do not sort it into a fictional timeline.
   assert(D::offerCount(sp)>0);
  }
 }
 assert(originalOffers==6527);
 assert(dpMachines==17575);
 assert(hgMachines==17632);
 assert(hgTutors==5165);
 assert(dpReady==492 && hgMethod==493 && platReady==12);
 for(G g:{G::Diamond,G::Pearl,G::Platinum}){
  auto x=starter(g);assert(N::evaluate(x).status==S::CandidateConsistent);++checks;
  x.steps[1].move=73;assert(N::evaluate(x).status==S::CandidateContradicted);++checks;
  x.steps[0].sourceMethodAttested=false;assert(N::evaluate(x).status==S::Unknown);++checks;
 }
 for(G g:{G::HeartGold,G::SoulSilver}){
   auto x=plain(g);x.count=1;x.steps[0]={g,5,A::Tutor,29,2,0,0,true,true,false};
   x.observedMoves={33,45,29,0};assert(N::evaluate(x).status==S::CandidateConsistent);++checks;
   x.observedLevel=6;assert(N::evaluate(x).status==S::Unknown);++checks;
 }
 std::cout<<"L3 original Gen IV source audit: DP="<<originalOffers
          <<" offers / "<<dpMachines<<" machines, HGSS="<<hgMachines
          <<" machines / "<<hgTutors<<" tutors, "<<checks<<" source checks PASS\n";
}
