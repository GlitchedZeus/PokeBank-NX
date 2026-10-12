// Exact original Ruby, Sapphire, Emerald, FireRed, LeafGreen production-table
// link. No fake original specimen, historical TM receipt or parent history is
// established by these synthetic source-method regressions.
#include "Legality/Gen3GbaJointMethodHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace R=Legality::Gen3GbaJointMethodHistory;
namespace H=Legality::Gen3GbaJointMethodHistoryCore;
using G=H::Game;using A=H::Action;using S=H::Status;
constexpr std::array<G,5> games{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen};
H::History emptyLevelStage(G game,std::uint16_t species) {
 H::History p{};p.candidateId=3003;p.game=game;p.species=species;
 p.initialLevel=100;p.observedLevel=100;p.initialMoves={33,0,0,0};
 p.observedMoves=p.initialMoves;p.initialSnapshotVerified=true;
 p.samePokemonAndGameVerified=true;p.allPlayerActionsVerified=true;
 return p;
}
int main() {
 std::uint64_t supported=0,possibleMachines=0,blockedMachines=0;
 std::uint64_t originalOffers=0,relearnerChecks=0,sourceChecks=0;
 for(const auto game:games)for(std::uint16_t species=1;species<=386;++species) {
  const auto data=R::originalEvidence(game,species);
  assert(data.pinned&&data.count&&data.count<=64);
  originalOffers+=data.count;++supported;
  const auto candidate=emptyLevelStage(game,species);
  assert(R::evaluate(candidate).status==S::CandidateConsistent);
  for(std::uint16_t i=0;i<data.count;++i) {
   const auto o=data.offers[i];
   assert(o.level>=1&&o.level<=100&&o.move>=1&&o.move<=354);
   if(i)assert(data.offers[i-1].level<=o.level);
   ++sourceChecks;
  }
  const auto c=H::Catalog{game,species,
      std::span<const H::Offer>(data.offers.data(),data.count),
      std::span<const std::uint16_t>(Legality::Gen3GbaOriginalMachineData::kMachineMove.data(),58),
      data.machines,true,true,true,true};
  assert(H::pinnedCatalog(c));
  for(unsigned level=1;level<=100;++level) {
   // All source levels, all species, all retail cartridges: no higher-level
   // offer is exposed via the original relearner menu.
   for(std::uint16_t i=0;i<data.count;++i) {
    const auto o=data.offers[i];
    const bool present=H::inRelearnerMenu(c,static_cast<std::uint8_t>(level),
                                          {33,0,0,0},o.move);
    if(o.move!=33&&o.level<=level)assert(present);
    if(o.move==33)assert(!present);
    ++relearnerChecks;
   }
  }
  for(unsigned number=1;number<=58;++number) {
   auto p=candidate;p.stepCount=1;
   const bool tm=number<=50;
   const std::uint8_t n=static_cast<std::uint8_t>(tm?number:number-50);
   const std::uint16_t move=Legality::Gen3GbaOriginalMachineData::kMachineMove[number-1];
   p.steps[0]={game,100,tm?A::TeachTM:A::TeachHM,move,1,n,true};
   p.observedMoves={33,move,0,0};
   const bool enabled=(data.machines&(std::uint64_t{1}<<(number-1)))!=0;
   assert(R::evaluate(p).status==(enabled?S::CandidateConsistent:S::CandidateContradicted));
   if(enabled)++possibleMachines;else ++blockedMachines;
   p.steps[0].originalActionReceiptVerified=false;
   assert(R::evaluate(p).status==S::Unknown);
  }
 }
 assert(supported==1930);
 assert(originalOffers==19865); // Ruby + Sapphire + Emerald + FR + LG.
 assert(possibleMachines==44635);
 assert(possibleMachines+blockedMachines==1930ULL*58);
 // Never elevate a hypothetical candidate into whole-Pokemon Invalid.
 auto conflicting=emptyLevelStage(G::Ruby,1);conflicting.initialLevel=5;
 conflicting.observedLevel=10;conflicting.stepCount=1;
 conflicting.steps[0]={G::Ruby,10,A::TeachTM,92,1,6,true};
 conflicting.observedMoves={33,92,0,0};
 assert(R::evaluate(conflicting).status==S::CandidateContradicted);
 conflicting.allPlayerActionsVerified=false;
 assert(R::evaluate(conflicting).status==S::Unknown);
 assert(R::originalEvidence(G::Unknown,1).pinned==false);
 assert(R::originalEvidence(G::Ruby,387).pinned==false);
 std::cout<<"L3 Gen III original exact-game joint provenance: "<<supported
          <<" source rows, "<<originalOffers<<" offers, "<<possibleMachines
          <<" method positives, "<<blockedMachines<<" candidate negatives, "
          <<sourceChecks<<" source checks, "<<relearnerChecks
          <<" source-level relearner comparisons PASS\n";
}
