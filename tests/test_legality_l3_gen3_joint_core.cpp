#include "Legality/Gen3GbaJointMethodHistoryCore.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>
namespace H=Legality::Gen3GbaJointMethodHistoryCore;
using G=H::Game;using A=H::Action;using S=H::Status;
constexpr std::array<H::Offer,11> bulbasaur{{
 {1,33},{4,45},{7,73},{10,22},{15,77},{15,79},{20,75},{25,230},
 {32,74},{39,235},{46,76}
}};
constexpr std::array<H::Offer,10> charmanderRSE{{
 {1,10},{1,45},{7,52},{13,108},{19,99},{25,184},
 {31,53},{37,163},{43,82},{49,83}
}};
constexpr std::array<H::Offer,10> charmanderFRLG{{
 {1,10},{1,45},{7,52},{13,232},{19,108},{25,184},
 {31,53},{37,163},{43,82},{49,83}
}};
constexpr auto machines=[]{
 std::array<std::uint16_t,58> a{};a.fill(1);a[5]=92;a[9]=237;a[50]=15;return a;
}();
constexpr H::Catalog catalog(G game=G::Ruby,std::uint16_t species=1) {
 return {game,species,species==1?std::span<const H::Offer>(bulbasaur):
       game==G::FireRed||game==G::LeafGreen?
       std::span<const H::Offer>(charmanderFRLG):
       std::span<const H::Offer>(charmanderRSE),machines,
       (std::uint64_t{1}<<5)|(std::uint64_t{1}<<9)|(std::uint64_t{1}<<50),
       true,true,true,true};
}
constexpr H::History starter(G game=G::Ruby,std::uint16_t species=1) {
 H::History p{};p.candidateId=7103;p.game=game;p.species=species;
 p.initialLevel=5;p.observedLevel=10;
 p.initialMoves=species==1?std::array<std::uint16_t,4>{33,45,0,0}:
                           std::array<std::uint16_t,4>{10,45,0,0};
 p.observedMoves=p.initialMoves;
 p.initialSnapshotVerified=true;p.samePokemonAndGameVerified=true;
 p.allPlayerActionsVerified=true;return p;
}
constexpr H::History validMixed() {
 auto p=starter();p.stepCount=4;p.observedLevel=11;
 p.steps[0]={G::Ruby,7,A::LevelLearn,73,2,0,true};
 p.steps[1]={G::Ruby,10,A::LevelLearn,22,3,0,true};
 p.steps[2]={G::Ruby,10,A::TeachTM,92,0,6,true};
 p.steps[3]={G::Ruby,11,A::Relearn,33,0,0,true};
 p.observedMoves={33,45,73,22};return p;
}
static_assert(H::evaluate(catalog(),validMixed()).status==S::CandidateConsistent);
static_assert(H::evaluate(catalog(),validMixed()).mandatoryOffers==2);
static_assert(H::evaluate(catalog(),validMixed()).machines==1);
static_assert(H::evaluate(catalog(),validMixed()).reminders==1);
static_assert([]{auto p=validMixed();p.steps[1].move=73;
 return H::evaluate(catalog(),p).status==S::CandidateContradicted;}());
static_assert([]{auto p=validMixed();p.steps[3].originalActionReceiptVerified=false;
 return H::evaluate(catalog(),p).status==S::Unknown;}());
static_assert([]{auto p=validMixed();p.steps[2].machineNumber=10;
 return H::evaluate(catalog(),p).status==S::CandidateContradicted;}());
static_assert([]{auto p=validMixed();p.steps[0].slot=3;
 return H::evaluate(catalog(),p).status==S::CandidateContradicted;}());
constexpr H::History charmander(G game,std::uint16_t offer) {
 auto p=starter(game,4);p.initialLevel=12;p.observedLevel=13;
 p.initialMoves={10,45,52,0};p.observedMoves={10,45,52,offer};
 p.steps[0]={game,13,A::LevelLearn,offer,3,0,true};p.stepCount=1;
 return p;
}
static_assert(H::evaluate(catalog(G::Ruby,4),charmander(G::Ruby,108)).status==S::CandidateConsistent);
static_assert(H::evaluate(catalog(G::FireRed,4),charmander(G::FireRed,232)).status==S::CandidateConsistent);
static_assert(H::evaluate(catalog(G::FireRed,4),charmander(G::FireRed,108)).status==S::CandidateContradicted);
int main() {
 std::uint32_t checks=0;
 // Every cartridge has an exact separate source identity; sources cannot be
 // laundered from a matching level/move row on a different game.
 for(G game:{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen}) {
  const auto data=catalog(game);
  auto good=validMixed();good.game=game;
  for(std::uint8_t i=0;i<good.stepCount;++i)good.steps[i].game=game;
  assert(H::evaluate(data,good).status==S::CandidateConsistent);++checks;
  for(unsigned i=0;i<good.stepCount;++i) {
   for(unsigned mutation=0;mutation<14;++mutation) {
    auto p=good;
    switch(mutation) {
    case 0:p.steps[i].originalActionReceiptVerified=false;break;
    case 1:p.steps[i].game=G::Unknown;break;
    case 2:p.steps[i].level=0;break;
    case 3:p.steps[i].action=A::UnknownService;break;
    case 4:p.steps[i].move=0;break;
    case 5:p.steps[i].move=355;break;
    case 6:p.steps[i].machineNumber=90;break;
    case 7:p.steps[i].slot=7;break;
    case 8:p.steps[i].level=100;break;
    case 9:p.steps[i].game=G::Unknown;break;
    case 10:p.steps[i].originalActionReceiptVerified=false;break;
    case 11:p.steps[i].move=999;break;
    case 12:p.steps[i].action=A::DeleteMove;break;
    default:p.steps[i].action=A::TeachHM;break;
    }
    assert(H::evaluate(data,p).status!=S::CandidateConsistent);++checks;
   }
  }
  for(std::uint16_t species=1;species<=386;++species) {
   auto candidate=good;candidate.species=species;
   assert(H::evaluate(data,candidate).status==(species==1?S::CandidateConsistent:S::Unknown));++checks;
   auto c=data;c.species=species;
   if(species!=1)assert(H::evaluate(c,good).status==S::Unknown);
   ++checks;
  }
  for(unsigned mutation=0;mutation<1024;++mutation) {
   auto c=data;auto p=good;
   if(mutation&1)c.fullMachineSourceVerified=false;
   if(mutation&2)c.fullLevelUpSourceVerified=false;
   if(mutation&4)c.originalSourceVerified=false;
   if(mutation&8)c.originalRelearnerFunctionVerified=false;
   if(mutation&16)p.samePokemonAndGameVerified=false;
   if(mutation&32)p.initialSnapshotVerified=false;
   if(mutation&64)p.allPlayerActionsVerified=false;
   if(mutation&128)p.candidateId=0;
   if(mutation&256)c.game=G::Unknown;
   if(mutation&512)p.game=G::Unknown;
   assert(H::evaluate(c,p).status==(mutation?S::Unknown:S::CandidateConsistent));++checks;
  }
 }
 auto options=H::Alternatives{};
 options.paths[0]=validMixed();options.count=1;
 options.paths[0].steps[0].move=22;
 assert(H::evaluate(catalog(),options)==S::Unknown);++checks;
 options.allActualHistoriesAuthenticatedAndEnumerated=true;
 assert(H::evaluate(catalog(),options)==S::CandidateContradicted);++checks;
 options.paths[1]=validMixed();options.count=2;
 assert(H::evaluate(catalog(),options)==S::CandidateConsistent);++checks;
 assert(checks==9268);
 std::cout<<"L3 Gen III source-exact joint method history: "<<checks<<" PASS\n";
}
