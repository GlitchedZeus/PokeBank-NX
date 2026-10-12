// L1 Stage 6 independent historical source->present lineage tests.
// Fixtures are synthetic *attestation controls*, not retail provenance.
// Assertions prove that unknown egg/gift/event/evolution/transfer candidates
// survive contradictions in OTHER routes and no result is global Invalid.
#include "Legality/Gen14CrossOriginReconciliation.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
namespace L=Legality::Gen14CrossOriginReconciliation;
using L::Acquisition;using L::Edge;using L::Evidence;
using L::Status;using L::UniverseStatus;
L::Route basic(Acquisition family=Acquisition::Wild,uint8_t gen=3,
               uint16_t species=25) {
 L::Route r{};
 r.rootSpecimen=0xAABBCCDD;r.routeId=9;r.originalSave=101;
 r.presentSave=101;r.capturedSpecies=species;r.presentSpecies=species;
 r.captureGeneration=gen;r.presentGeneration=gen;r.acquisition=family;
 r.evidence=Evidence::SourceTemplateOnly;
 r.originalSaveAndSpecimenIndependentlyAttested=true;
 r.originalAcquisitionSceneIndependentlyAttested=true;
 r.capturedSpeciesAndGenerationIndependentlyAttested=true;
 r.currentSpecimenIdentityAndSnapshotIndependentlyAttested=true;
 return r;
}
void add(L::Route& r,Edge kind,uint16_t species,uint8_t generation,
         uint64_t save) {
 assert(r.edgeCount<L::kMaxCustodyEdges);
 const uint8_t i=r.edgeCount++;
 auto& e=r.edges[i];
 if(i==0){e.fromSave=r.originalSave;e.fromSpecies=r.capturedSpecies;
          e.fromGeneration=r.captureGeneration;}
 else{
  const auto& prev=r.edges[i-1];
  e.fromSave=prev.toSave;e.fromSpecies=prev.toSpecies;
  e.fromGeneration=prev.toGeneration;
 }
 e.toSave=save;e.toSpecies=species;e.toGeneration=generation;e.type=kind;
 e.originalTransactionIndependentlyAttested=true;
 e.sourceAndDestinationSameSpecimenIndependentlyAttested=true;
 e.exactEvolutionOrTransferRuleIndependentlyVerified=true;
 r.presentSave=save;r.presentSpecies=species;r.presentGeneration=generation;
}
int main(){
 uint64_t checks=0;
 // All original acquisition families in their source generations; an egg,
 // event or NPC trade is a DISTINCT physical source, not an alias for wild.
 for(unsigned a=1;a<=static_cast<unsigned>(Acquisition::PokeWalker);++a) {
  for(unsigned gen=1;gen<=4;++gen){
   auto r=basic(static_cast<Acquisition>(a),static_cast<uint8_t>(gen));
   const bool possible=L::familyMayOriginate(r.acquisition,r.captureGeneration);
   assert(L::evaluate(r).status==
          (possible?Status::ConditionalSourceAndCustodyOnly:Status::UnknownOriginalSource));
   ++checks;
  }
 }
 // Authentic-shaped custody edges in Gen I-IV: Time Capsule is reversible,
 // Pal Park is one way, GameCube shadow needs GBA receiving custody.
 auto evolution=basic(Acquisition::Egg,2,172);
 add(evolution,Edge::Evolution,25,2,101);
 assert(L::evaluate(evolution).status==Status::ConditionalSourceAndCustodyOnly);++checks;
 auto tradeEvolution=basic(Acquisition::NpcTrade,1,64);
 add(tradeEvolution,Edge::TradeEvolution,65,1,102);
 assert(L::evaluate(tradeEvolution).status==Status::ConditionalSourceAndCustodyOnly);++checks;
 auto capsule=basic(Acquisition::EventDistribution,1,25);
 add(capsule,Edge::TimeCapsule,25,2,103);
 add(capsule,Edge::TimeCapsule,25,1,104);
 assert(L::evaluate(capsule).status==Status::ConditionalSourceAndCustodyOnly);++checks;
 auto gamecube=basic(Acquisition::GameCubeShadow,3,197);
 add(gamecube,Edge::GameCubeToGba,197,3,105);
 add(gamecube,Edge::PalPark,197,4,106);
 add(gamecube,Edge::IntraGenerationTrade,197,4,107);
 add(gamecube,Edge::Evolution,471,4,107);
 assert(L::evaluate(gamecube).status==Status::ConditionalSourceAndCustodyOnly);++checks;
 gamecube.evidence=Evidence::ConditionalOriginalNative;
 assert(L::evaluate(gamecube).status==
        Status::ConditionalOriginalStageAndCustodyOnly);++checks;
 // Every independently witnessed missing bit on EVERY edge remains Unknown.
 for(unsigned i=0;i<gamecube.edgeCount;++i) {
  auto missing=gamecube;missing.edges[i].originalTransactionIndependentlyAttested=false;
  assert(L::evaluate(missing).status==Status::UnknownCustodyLink);++checks;
  missing=gamecube;missing.edges[i].sourceAndDestinationSameSpecimenIndependentlyAttested=false;
  assert(L::evaluate(missing).status==Status::UnknownCustodyLink);++checks;
  missing=gamecube;missing.edges[i].exactEvolutionOrTransferRuleIndependentlyVerified=false;
  assert(L::evaluate(missing).status==Status::UnknownTransitionRule);++checks;
  missing=gamecube;missing.edges[i].fromSave=999;
  assert(L::evaluate(missing).status==Status::UnknownCustodyLink);++checks;
  missing=gamecube;missing.edges[i].fromSpecies=999;
  assert(L::evaluate(missing).status==Status::UnknownOutsideGenerationScope ||
         L::evaluate(missing).status==Status::UnknownCustodyLink);++checks;
 }
 auto fail=gamecube;fail.edges[1].type=Edge::TimeCapsule;
 assert(L::evaluate(fail).status==Status::LocalTransitionContradiction);++checks;
 fail=gamecube;fail.edges[0].type=Edge::Evolution;
 assert(L::evaluate(fail).status==Status::LocalTransitionContradiction);++checks;
 fail=gamecube;fail.edges[2].toSave=fail.edges[2].fromSave;
 assert(L::evaluate(fail).status==Status::LocalTransitionContradiction);++checks;
 fail=gamecube;fail.presentSpecies=25;
 assert(L::evaluate(fail).status==Status::LocalPresentMismatch);++checks;
 fail=gamecube;fail.edgeCount=9;
 assert(L::evaluate(fail).status==Status::UnknownCapacity);++checks;
 fail=gamecube;fail.edges[0].type=Edge::OutsideModel;
 assert(L::evaluate(fail).status==Status::UnknownOutsideGenerationScope);++checks;
 fail=gamecube;fail.edges[0].toGeneration=5;
 assert(L::evaluate(fail).status==Status::UnknownOutsideGenerationScope);++checks;
 fail=gamecube;fail.presentGeneration=5;
 assert(L::evaluate(fail).status==Status::UnknownPresentSnapshot);++checks;
 fail=gamecube;fail.evidence=Evidence::BoundedSearchUnknown;
 assert(L::evaluate(fail).status==Status::UnknownSearchBound);++checks;
 fail=gamecube;fail.evidence=Evidence::LocalSourceContradiction;
 assert(L::evaluate(fail).status==Status::LocalOriginalSourceContradiction);++checks;
 fail=gamecube;fail.evidence=static_cast<Evidence>(255);
 assert(L::evaluate(fail).status==Status::UnknownOriginalSource);++checks;
 fail=gamecube;fail.acquisition=static_cast<Acquisition>(255);
 assert(L::evaluate(fail).status==Status::UnknownOriginalSource);++checks;
 fail=gamecube;fail.edges[0].type=static_cast<Edge>(255);
 assert(L::evaluate(fail).status==Status::UnknownOutsideGenerationScope);++checks;
 // Across 7 rival source routes, all 5^7 evidence assignments must respect:
 // any compatible route wins, any unresolved alternate survives a local
 // negative, and even all contradicted remains candidate-only.
 constexpr std::array<Evidence,5> evidence{
    Evidence::Unknown,Evidence::SourceTemplateOnly,
    Evidence::ConditionalOriginalNative,
    Evidence::LocalSourceContradiction,Evidence::BoundedSearchUnknown
 };
 constexpr std::array<Acquisition,7> origin{
    Acquisition::Wild,Acquisition::Egg,Acquisition::Gift,
    Acquisition::NpcTrade,Acquisition::EventDistribution,
    Acquisition::GameCubeShadow,Acquisition::Static
 };
 for(unsigned code=0;code<78125;++code) {
  std::array<L::Route,7> paths{};
  unsigned x=code;unsigned matched=0,unknown=0,contradicted=0;
  for(unsigned i=0;i<paths.size();++i){
    const auto ev=evidence[x%5];x/=5;
    paths[i]=basic(origin[i],3,25);
    paths[i].routeId=i+1;paths[i].evidence=ev;
    matched+=ev==Evidence::SourceTemplateOnly ||
             ev==Evidence::ConditionalOriginalNative;
    contradicted+=ev==Evidence::LocalSourceContradiction;
    unknown+=ev==Evidence::Unknown||ev==Evidence::BoundedSearchUnknown;
  }
  auto r=L::reconcile(0xAABBCCDD,true,paths);
  assert(r.conditional==matched&&r.contradicted==contradicted&&r.unknown==unknown);
  assert(r.status==(matched?UniverseStatus::AtLeastOneConditionalRoute:
              unknown?UniverseStatus::UnknownAlternativeOrigins:
                      UniverseStatus::OnlyProvidedRoutesLocallyContradicted));
  assert(!r.globalInvalidSupported&&!r.globallyLegalSupported);
  checks+=3;
  // No forged extra paths or duplicate route IDs can close the universe.
  paths[6].routeId=paths[5].routeId;
  assert(L::reconcile(0xAABBCCDD,true,paths).status==
         UniverseStatus::UnknownDuplicateRoute);
  ++checks;
 }
 for(unsigned a=0;a<256;++a)for(unsigned e=0;e<256;++e){
   auto r=basic(static_cast<Acquisition>(a),3);
   r.evidence=static_cast<Evidence>(e);
   const auto outcome=L::evaluate(r);
   assert(!outcome.globalInvalidSupported&&!outcome.globallyLegalSupported);
   if(a>static_cast<unsigned>(Acquisition::PokeWalker) ||
      (e>static_cast<unsigned>(Evidence::BoundedSearchUnknown)))
       assert(outcome.status==Status::UnknownOriginalSource);
   ++checks;
 }
 std::array<L::Route,65> overflow{};
 for(unsigned i=0;i<overflow.size();++i){overflow[i]=basic();overflow[i].routeId=i+1;}
 assert(L::reconcile(0xAABBCCDD,true,overflow).status==UniverseStatus::UnknownCapacity);
 assert(L::reconcile(0,true,std::span<const L::Route>()).status==UniverseStatus::UnknownRoot);
 assert(L::reconcile(0xAABBCCDD,true,std::span<const L::Route>()).status==
        UniverseStatus::UnknownAlternativeOrigins);
 checks+=3;
 std::cout<<"L1 Stage6 cross-origin acquisition, evolution, reversible Time Capsule, "
          <<"GameCube to GBA to Pal Park, 5^7 alternatives and enum domains: "
          <<checks<<" verified scenario assertions PASS\n";
}
