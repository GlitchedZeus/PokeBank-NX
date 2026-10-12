#pragma once
// L1 Stage 6: READ-ONLY candidate-specific acquisition/lineage reconciliation.
// An original wild, egg, gift, NPC trade, event, static or GameCube candidate
// is NOT interchangeable with a different source merely because the current
// species/PID/IVs match. Every custody edge is independently attested.
// Unsupported future histories and incomplete chain evidence remain Unknown.
// Local contradiction of every SUPPLIED route never establishes global Invalid.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen14CrossOriginReconciliation {
inline constexpr std::size_t kMaxRoutes=64;
inline constexpr std::size_t kMaxCustodyEdges=8;
enum class Acquisition : uint8_t {
 Unknown,Wild,Static,GadgetFishing,Safari,GameCubeShadow,
 Egg,Gift,NpcTrade,EventDistribution,Roamer,Headbutt,
 HoneyTree,Radar,PokeWalker
};
enum class Edge : uint8_t {
 Unknown,Evolution,TradeEvolution,IntraGenerationTrade,
 TimeCapsule,GameCubeToGba,PalPark,OutsideModel
};
enum class Evidence : uint8_t {
 Unknown,SourceTemplateOnly,OriginalRngStageOnly,
 ConditionalOriginalNative,LocalSourceContradiction,BoundedSearchUnknown
};
enum class Status : uint8_t {
 UnknownRoot,UnknownOriginalSource,UnknownHistoricalWitness,
 UnknownPresentSnapshot,UnknownCustodyLink,UnknownTransitionRule,
 UnknownOutsideGenerationScope,UnknownCapacity,UnknownAlternativeHistory,
 UnknownSearchBound,LocalOriginalSourceContradiction,
 LocalTransitionContradiction,LocalPresentMismatch,
 ConditionalSourceAndCustodyOnly,ConditionalOriginalStageAndCustodyOnly
};
struct CustodyEdge {
 Edge type=Edge::Unknown;
 uint64_t fromSave=0,toSave=0;
 uint16_t fromSpecies=0,toSpecies=0;
 uint8_t fromGeneration=0,toGeneration=0;
 bool originalTransactionIndependentlyAttested=false;
 bool sourceAndDestinationSameSpecimenIndependentlyAttested=false;
 bool exactEvolutionOrTransferRuleIndependentlyVerified=false;
};
struct Route {
 uint64_t rootSpecimen=0,routeId=0,originalSave=0,presentSave=0;
 uint16_t capturedSpecies=0,presentSpecies=0;
 uint8_t captureGeneration=0,presentGeneration=0,edgeCount=0;
 Acquisition acquisition=Acquisition::Unknown;
 Evidence evidence=Evidence::Unknown;
 bool originalSaveAndSpecimenIndependentlyAttested=false;
 bool originalAcquisitionSceneIndependentlyAttested=false;
 bool capturedSpeciesAndGenerationIndependentlyAttested=false;
 bool currentSpecimenIdentityAndSnapshotIndependentlyAttested=false;
 std::array<CustodyEdge,kMaxCustodyEdges> edges{};
};
struct Result {
 Status status=Status::UnknownRoot;
 uint8_t examinedEdges=0;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr bool recognizedAcquisition(Acquisition x) noexcept {
 return x>=Acquisition::Wild&&x<=Acquisition::PokeWalker;
}
constexpr bool recognizedEvidence(Evidence x) noexcept {
 return x>=Evidence::Unknown&&x<=Evidence::BoundedSearchUnknown;
}
constexpr bool recognizedEdge(Edge e) noexcept {
 return e>=Edge::Evolution&&e<=Edge::OutsideModel;
}
constexpr bool speciesInGeneration(uint16_t species,uint8_t generation) noexcept {
 return species>0&&((generation==1&&species<=151)||
                    (generation==2&&species<=251)||
                    (generation==3&&species<=386)||
                    (generation==4&&species<=493));
}
constexpr bool familyMayOriginate(Acquisition family,uint8_t generation) noexcept {
 if(!recognizedAcquisition(family)||generation<1||generation>4)return false;
 switch(family) {
 case Acquisition::GameCubeShadow:return generation==3;
 case Acquisition::Headbutt:return generation==2||generation==4;
 case Acquisition::HoneyTree:case Acquisition::Radar:
 case Acquisition::PokeWalker:return generation==4;
 default:return true;
 }
}
constexpr bool recognizedTransaction(Edge edge,uint8_t from,uint8_t to) noexcept {
 switch(edge){
 case Edge::Evolution:return from==to;
 case Edge::TradeEvolution:case Edge::IntraGenerationTrade:return from==to;
 case Edge::TimeCapsule:return (from==1&&to==2)||(from==2&&to==1);
 case Edge::GameCubeToGba:return from==3&&to==3;
 case Edge::PalPark:return from==3&&to==4;
 default:return false;
 }
}
constexpr bool speciesTransitionCoherent(const CustodyEdge& e) noexcept {
 switch(e.type) {
 case Edge::Evolution:case Edge::TradeEvolution:
   return e.fromSpecies!=e.toSpecies;
 case Edge::IntraGenerationTrade:case Edge::TimeCapsule:
 case Edge::GameCubeToGba:case Edge::PalPark:
   return e.fromSpecies==e.toSpecies &&
     (e.type!=Edge::TimeCapsule||e.fromSpecies<=151);
 default:return false;
 }
}
constexpr Result evaluate(const Route& c) noexcept {
 if(!c.rootSpecimen||!c.originalSave||
    !c.originalSaveAndSpecimenIndependentlyAttested)return {};
 if(!c.routeId||!c.originalAcquisitionSceneIndependentlyAttested||
    !c.capturedSpeciesAndGenerationIndependentlyAttested||
    !familyMayOriginate(c.acquisition,c.captureGeneration)||
    !speciesInGeneration(c.capturedSpecies,c.captureGeneration)||
    !recognizedEvidence(c.evidence))
    return {Status::UnknownOriginalSource};
 if(c.evidence==Evidence::LocalSourceContradiction)
    return {Status::LocalOriginalSourceContradiction};
 if(c.evidence==Evidence::BoundedSearchUnknown)
    return {Status::UnknownSearchBound};
 if(c.evidence==Evidence::Unknown)return {Status::UnknownHistoricalWitness};
 if(!c.currentSpecimenIdentityAndSnapshotIndependentlyAttested||
    !c.presentSave||!speciesInGeneration(c.presentSpecies,c.presentGeneration))
    return {Status::UnknownPresentSnapshot};
 if(c.edgeCount>kMaxCustodyEdges)return {Status::UnknownCapacity};
 uint64_t save=c.originalSave;
 uint16_t species=c.capturedSpecies;
 uint8_t generation=c.captureGeneration;
 for(uint8_t i=0;i<c.edgeCount;++i) {
   const auto& e=c.edges[i];
   if(!recognizedEdge(e.type)||e.type==Edge::OutsideModel ||
      !speciesInGeneration(e.fromSpecies,e.fromGeneration)||
      !speciesInGeneration(e.toSpecies,e.toGeneration))
      return {Status::UnknownOutsideGenerationScope,i};
   if(!e.originalTransactionIndependentlyAttested||
      !e.sourceAndDestinationSameSpecimenIndependentlyAttested||
      !e.fromSave||!e.toSave)
      return {Status::UnknownCustodyLink,i};
   if(!e.exactEvolutionOrTransferRuleIndependentlyVerified)
      return {Status::UnknownTransitionRule,i};
   // An independently archived but DIFFERENT specimen or save was
   // spliced into this route; do not re-use it as a negative original
   // source proof or erase a separate egg/event/transfer hypothesis.
   if(e.fromSave!=save || e.fromSpecies!=species ||
      e.fromGeneration!=generation)
      return {Status::UnknownCustodyLink,i};
   if(!recognizedTransaction(e.type,e.fromGeneration,e.toGeneration) ||
      !speciesTransitionCoherent(e) ||
      ((e.type==Edge::Evolution)&&e.fromSave!=e.toSave) ||
      ((e.type==Edge::TradeEvolution||
        e.type==Edge::IntraGenerationTrade||
        e.type==Edge::TimeCapsule||
        e.type==Edge::GameCubeToGba||
        e.type==Edge::PalPark)&&e.fromSave==e.toSave))
      return {Status::LocalTransitionContradiction,static_cast<uint8_t>(i+1)};
   save=e.toSave;species=e.toSpecies;generation=e.toGeneration;
 }
 if(save!=c.presentSave||species!=c.presentSpecies||
    generation!=c.presentGeneration)
    return {Status::LocalPresentMismatch,c.edgeCount};
 return {c.evidence==Evidence::SourceTemplateOnly ?
         Status::ConditionalSourceAndCustodyOnly :
         Status::ConditionalOriginalStageAndCustodyOnly,c.edgeCount};
}
enum class UniverseStatus : uint8_t {
 UnknownRoot,UnknownCapacity,UnknownDuplicateRoute,UnknownAlternativeOrigins,
 OnlyProvidedRoutesLocallyContradicted,AtLeastOneConditionalRoute
};
struct Universe {
 UniverseStatus status=UniverseStatus::UnknownRoot;
 uint8_t conditional=0,contradicted=0,unknown=0;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr bool localContradiction(Status s) noexcept {
 return s==Status::LocalOriginalSourceContradiction ||
        s==Status::LocalTransitionContradiction ||
        s==Status::LocalPresentMismatch;
}
constexpr Universe reconcile(uint64_t root,bool rootIndependentlyAttested,
                             std::span<const Route> alternatives) noexcept {
 Universe result{};
 if(!root||!rootIndependentlyAttested)return result;
 if(alternatives.size()>kMaxRoutes){result.status=UniverseStatus::UnknownCapacity;return result;}
 for(std::size_t i=0;i<alternatives.size();++i) {
   if(!alternatives[i].routeId||alternatives[i].rootSpecimen!=root){
      result.status=UniverseStatus::UnknownDuplicateRoute;return result;
   }
   for(std::size_t j=0;j<i;++j)if(alternatives[i].routeId==alternatives[j].routeId){
      result.status=UniverseStatus::UnknownDuplicateRoute;return result;
   }
 }
 for(const auto& candidate:alternatives){
   const Status s=evaluate(candidate).status;
   if(s==Status::ConditionalSourceAndCustodyOnly||
      s==Status::ConditionalOriginalStageAndCustodyOnly)++result.conditional;
   else if(localContradiction(s))++result.contradicted;
   else ++result.unknown;
 }
 if(result.conditional)result.status=UniverseStatus::AtLeastOneConditionalRoute;
 else if(result.unknown||alternatives.empty())result.status=UniverseStatus::UnknownAlternativeOrigins;
 else result.status=UniverseStatus::OnlyProvidedRoutesLocallyContradicted;
 return result;
}
static_assert(kMaxRoutes==64&&kMaxCustodyEdges==8);
} // namespace Legality::Gen14CrossOriginReconciliation
