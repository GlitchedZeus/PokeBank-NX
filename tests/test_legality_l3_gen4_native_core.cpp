#include "Legality/Gen4NativeOriginalHistoryCore.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen4NativeOriginalHistoryCore;
using G=H::Game;using A=H::Action;using S=H::Status;
struct Source {
 static constexpr std::array<std::uint16_t,5> kDP{{545,1581,3657,4630,6733}}; // lv1 Tackle, lv3 Growl, lv7 Leech Seed, lv9 Vine Whip, lv13 PoisonPowder
 static constexpr std::array<std::uint16_t,3> kPt{{545,1581,3657}};
 static constexpr std::array<std::uint16_t,2> kHGSTutor{{29,189}};
 static constexpr std::array<std::uint16_t,100> kMachines=[] {
     std::array<std::uint16_t,100> x{};x[5]=92;x[52]=412;x[92]=15;return x;
 }();
 static constexpr H::Catalog catalog(G g,std::uint16_t species) noexcept {
   H::Catalog c{};if(species!=1||!H::supported(g))return c;
   c.game=g;c.species=species;c.machineCompatibilitySourceComplete=true;
   c.machineLow=(std::uint64_t{1}<<5)|(std::uint64_t{1}<<52);
   c.machineHigh=std::uint64_t{1}<<(92-64);
   c.numberedMachineMoves=std::span<const std::uint16_t>(kMachines);
   if(g==G::Diamond||g==G::Pearl){c.orderedLevelSourceComplete=true;c.originalRelearnerSourceComplete=true;c.packedOffers=kDP;}
   if(g==G::Platinum){c.orderedLevelSourceComplete=true;c.originalRelearnerSourceComplete=true;c.packedOffers=kPt;}
   if(g==G::HeartGold||g==G::SoulSilver){c.tutorSourceComplete=true;c.tutorMoves=kHGSTutor;c.tutorMask=0x3;}
   return c;
 }
};
constexpr H::History base(G game=G::Diamond){
 H::History p{};p.candidate=3949;p.species=1;p.origin=p.observedGame=game;
 p.initialLevel=5;p.observedLevel=5;p.initialMoves={33,45,0,0};p.observedMoves=p.initialMoves;
 p.originalSnapshotAttested=p.samePokemonAttested=p.everyIntermediateActionAttested=true;return p;
}
constexpr H::History dp(){auto p=base();p.observedLevel=9;p.count=2;
 p.steps[0]={G::Diamond,7,A::LevelLearn,73,2,0,0,true,true,false};
 p.steps[1]={G::Diamond,9,A::LevelLearn,22,3,0,0,true,true,false};
 p.observedMoves={33,45,73,22};return p;}
constexpr H::History hgTutor(){auto p=base(G::HeartGold);p.count=1;
 p.steps[0]={G::HeartGold,5,A::Tutor,29,2,0,0,true,true,false};
 p.observedMoves={33,45,29,0};return p;}
static_assert(H::evaluate<Source>(dp()).status==S::CandidateConsistent);
static_assert(H::evaluate<Source>(hgTutor()).status==S::CandidateConsistent);
static_assert([]{auto p=hgTutor();p.observedLevel=6;return H::evaluate<Source>(p).status==S::Unknown;}());
static_assert([]{auto p=dp();p.steps[0].move=22;return H::evaluate<Source>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=base();p.observedLevel=7;return H::evaluate<Source>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=dp();p.steps[1].originalActionAttested=false;return H::evaluate<Source>(p).status==S::Unknown;}());
static_assert([]{auto p=base();p.count=1;p.steps[0]={G::Diamond,5,A::TM,92,2,6,99,true,true,false};p.observedMoves={33,45,92,0};return H::evaluate<Source>(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=base();p.count=1;p.steps[0]={G::Diamond,5,A::TM,92,2,7,99,true,true,false};p.observedMoves={33,45,92,0};return H::evaluate<Source>(p).status==S::Unknown;}());
static_assert([]{auto p=base();p.count=1;p.initialMoves={45,22,0,0};p.steps[0]={G::Diamond,5,A::Relearn,33,2,0,0,true,true,false};p.observedMoves={45,22,33,0};return H::evaluate<Source>(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=base(G::Platinum);p.count=1;p.initialMoves={45,22,0,0};p.steps[0]={G::Platinum,5,A::Relearn,33,2,0,0,true,true,false};p.observedMoves={45,22,33,0};return H::evaluate<Source>(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=base();p.count=1;p.steps[0]={G::Pearl,5,A::VisitGame,0,0,0,0,true,true,true};p.observedGame=G::Pearl;return H::evaluate<Source>(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=base();p.form=1;return H::evaluate<Source>(p).status==S::Unknown;}());
static_assert(!H::evaluate<Source>(dp()).globalInvalid);
static_assert([]{auto p=dp();p.steps[0].move=22;p.count=3;
 p.steps[2]={G::Diamond,9,A::Tutor,29,3,0,0,true,true,false};
 return H::evaluate<Source>(p).status==S::Unknown;}());
static_assert([]{auto p=dp();p.steps[0].move=22;p.count=3;
 p.steps[2]={G::Diamond,9,A::TM,92,3,7,711,true,true,false};
 return H::evaluate<Source>(p).status==S::Unknown;}());
int main(){
 std::uint64_t checks=0;
 for(int i=0;i<16000;++i){
   auto p=dp();if(i%2){p.origin=G::Pearl;p.observedGame=G::Pearl;p.steps[0].game=G::Pearl;p.steps[1].game=G::Pearl;}
   assert(H::evaluate<Source>(p).status==S::CandidateConsistent);++checks;
   const int which=i%8;auto x=p;
   if(which==0)x.steps[0].move=22;
   if(which==1)x.steps[1].move=73;
   if(which==2)x.steps[1].level=8;
   if(which==3)x.steps[1].slot=1;
   if(which==4)x.observedMoves={33,45,22,73};
   if(which==5)x.count=1;
   if(which==6)x.initialMoves={33,0,45,0};
   if(which==7)x.steps[0].action=A::LevelDecline;
   assert(H::evaluate<Source>(x).status!=S::CandidateConsistent);++checks;
   auto unknown=p;unknown.steps[1].sourceMethodAttested=false;
   assert(H::evaluate<Source>(unknown).status==S::Unknown);++checks;
   auto h=hgTutor();assert(H::evaluate<Source>(h).status==S::CandidateConsistent);++checks;
   h.observedLevel=6;assert(H::evaluate<Source>(h).status==S::Unknown);++checks;
   auto native=base();native.count=1;native.steps[0]={G::Diamond,5,A::TM,92,2,6,42,true,true,false};native.observedMoves={33,45,92,0};
   assert(H::evaluate<Source>(native).status==S::CandidateConsistent);++checks;
   native.steps[0].receipt=0;assert(H::evaluate<Source>(native).status==S::Unknown);++checks;
 }
 H::Alternatives a{};a.count=2;a.histories[0]=dp();a.histories[1]=dp();a.histories[0].steps[0].move=22;
 a.histories[1].steps[1].originalActionAttested=false;
 assert(H::evaluate<Source>(a)==S::Unknown);++checks;
 a.histories[1]=dp();assert(H::evaluate<Source>(a)==S::CandidateConsistent);++checks;
 std::cout<<"L3 original Gen IV title-specific history core: "<<checks<<" scenario groups PASS\n";
}
