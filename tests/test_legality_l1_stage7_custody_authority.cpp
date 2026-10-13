// L1 Stage 7: final production authority boundary. A matching PID/IV or
// nature-only seed is not an authenticated original catch, and cannot erase
// a competing egg, event, trade, GameCube or evolution acquisition history.
#include "Legality/Gen14CrossOriginReconciliation.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen14CrossOriginReconciliation;
L::Route original(std::uint64_t id,L::Evidence evidence) {
 L::Route c{};
 c.rootSpecimen=0x11223344;c.routeId=id;c.originalSave=101;
 c.presentSave=101;c.capturedSpecies=25;c.presentSpecies=25;
 c.captureGeneration=3;c.presentGeneration=3;
 c.acquisition=L::Acquisition::Wild;c.evidence=evidence;
 c.originalSaveAndSpecimenIndependentlyAttested=true;
 c.originalAcquisitionSceneIndependentlyAttested=true;
 c.capturedSpeciesAndGenerationIndependentlyAttested=true;
 c.currentSpecimenIdentityAndSnapshotIndependentlyAttested=true;
 return c;
}
int main() {
 std::uint64_t checks=0;
 auto stage=original(1,L::Evidence::OriginalRngStageOnly);
 assert(L::evaluate(stage).status==L::Status::UnknownHistoricalWitness);
 ++checks;
 auto conditional=original(2,L::Evidence::ConditionalOriginalNative);
 assert(L::evaluate(conditional).status==L::Status::ConditionalOriginalStageAndCustodyOnly);
 ++checks;
 for(unsigned bits=0;bits<65536;++bits) {
   auto c=original(3,static_cast<L::Evidence>(bits&255u));
   c.acquisition=static_cast<L::Acquisition>(bits>>8);
   const auto result=L::evaluate(c);
   assert(!result.globalInvalidSupported&&!result.globallyLegalSupported);
   if(!L::recognizedEvidence(c.evidence) ||
      !L::familyMayOriginate(c.acquisition,c.captureGeneration))
     assert(result.status==L::Status::UnknownOriginalSource);
   else if(c.evidence==L::Evidence::OriginalRngStageOnly)
     assert(result.status==L::Status::UnknownHistoricalWitness);
   ++checks;
 }
 for(unsigned code=0;code<6561;++code) {
   std::array<L::Route,8> routes{};
   unsigned v=code;unsigned unknown=0,contradicted=0;
   for(unsigned i=0;i<routes.size();++i) {
     const auto type=v%3u;v/=3u;
     routes[i]=original(i+1,type==0?L::Evidence::OriginalRngStageOnly:
                           type==1?L::Evidence::Unknown:
                                   L::Evidence::LocalSourceContradiction);
     unknown+=(type!=2);
     contradicted+=(type==2);
     const auto examined=L::evaluate(routes[i]);
     assert(examined.status==(type==2?
          L::Status::LocalOriginalSourceContradiction:
          L::Status::UnknownHistoricalWitness));
     ++checks;
   }
   const auto summary=L::reconcile(0x11223344,true,routes);
   assert(summary.unknown==unknown&&summary.contradicted==contradicted);
   assert(summary.conditional==0);
   assert(summary.status==(unknown?L::UniverseStatus::UnknownAlternativeOrigins:
                           L::UniverseStatus::OnlyProvidedRoutesLocallyContradicted));
   assert(!summary.globalInvalidSupported&&!summary.globallyLegalSupported);
   checks+=3;
   routes[7].routeId=routes[6].routeId;
   assert(L::reconcile(0x11223344,true,routes).status==
          L::UniverseStatus::UnknownDuplicateRoute);
   ++checks;
 }
 auto transfer=original(11,L::Evidence::SourceTemplateOnly);
 transfer.acquisition=L::Acquisition::GameCubeShadow;
 transfer.presentSave=300;transfer.presentSpecies=25;transfer.presentGeneration=4;
 transfer.edgeCount=2;
 transfer.edges[0]={L::Edge::GameCubeToGba,101,200,25,25,3,3,true,true,true};
 transfer.edges[1]={L::Edge::PalPark,200,300,25,25,3,4,true,true,true};
 assert(L::evaluate(transfer).status==L::Status::ConditionalSourceAndCustodyOnly);
 ++checks;
 for(unsigned i=0;i<2;++i) {
   auto missing=transfer;
   missing.edges[i].originalTransactionIndependentlyAttested=false;
   assert(L::evaluate(missing).status==L::Status::UnknownCustodyLink);
   ++checks;
   missing=transfer;
   missing.edges[i].exactEvolutionOrTransferRuleIndependentlyVerified=false;
   assert(L::evaluate(missing).status==L::Status::UnknownTransitionRule);
   ++checks;
 }
 transfer.evidence=L::Evidence::OriginalRngStageOnly;
 assert(L::evaluate(transfer).status==L::Status::UnknownHistoricalWitness);
 ++checks;
 std::cout<<"L1 Stage7 original acquisition evidence authority "<<checks
          <<" PASS, all alternative negative results candidate-only\n";
}
