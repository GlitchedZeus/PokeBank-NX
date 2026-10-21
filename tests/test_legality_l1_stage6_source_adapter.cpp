// L1 Stage 6: existing source evaluators cannot forge full lineage when
// a gift egg, NPC trade, event, GameCube transfer and wild alternative compete.
#include "Legality/Gen14CrossOriginEvidenceAdapter.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen14CrossOriginEvidenceAdapter;
namespace C=Legality::Gen14CrossOriginReconciliation;
namespace B=Legality::Gen14OriginalEvidenceBridge;
namespace U=Legality::Gen14EncounterCandidateUniverse;
namespace G4=Legality::Gen4OriginalEvidenceFamilyAdapter;
namespace GC=Legality::Gen3GameCubeOriginalTransferHistory;
namespace NPC=Legality::Gen2OriginalNpcTradeStage;
C::Route route(C::Acquisition type,std::uint64_t id,std::uint8_t gen=3,
               std::uint16_t species=25) {
 C::Route r{};
 r.rootSpecimen=0x30000001;r.routeId=id;r.originalSave=id+900;
 r.presentSave=r.originalSave;r.capturedSpecies=r.presentSpecies=species;
 r.captureGeneration=r.presentGeneration=gen;r.acquisition=type;
 r.originalSaveAndSpecimenIndependentlyAttested=true;
 r.originalAcquisitionSceneIndependentlyAttested=true;
 r.capturedSpeciesAndGenerationIndependentlyAttested=true;
 r.currentSpecimenIdentityAndSnapshotIndependentlyAttested=true;
 return r;
}
int main() {
 using E=C::Evidence;
 static_assert(A::map(B::Class::SourceOnly)==E::SourceTemplateOnly);
 static_assert(A::map(B::Class::RngStageOnly)==E::OriginalRngStageOnly);
 static_assert(A::map(B::Class::ConditionalNative)==E::ConditionalOriginalNative);
 static_assert(A::map(B::Class::CandidateLocalContradiction)==E::LocalSourceContradiction);
 static_assert(A::map(U::EvidenceKind::ConditionalOriginalHistory)==E::ConditionalOriginalNative);
 static_assert(A::map(G4::Core::Stage::BoundedExhausted)==E::BoundedSearchUnknown);
 static_assert(A::map(GC::Status::UnknownBoundedTeamSearch)==E::BoundedSearchUnknown);
 static_assert(A::map(NPC::Status::ConditionalOriginalTradeTemplateOnly)==
               E::SourceTemplateOnly);
 unsigned verified=0;
 std::array<C::Route,7> alternatives{{
  A::withOriginalSourceEvidence(route(C::Acquisition::Wild,1,1),
     B::G1::Status::CandidateOriginalSpeciesContradicted),
  A::withOriginalSourceEvidence(route(C::Acquisition::NpcTrade,2,2),
     NPC::Status::ConditionalOriginalTradeTemplateOnly),
  A::withOriginalSourceEvidence(route(C::Acquisition::Egg,3,4),
     G4::map(G4::Static::Status::ConditionalOriginalGiftEggSourceAwaitingBreedingVerification)),
  A::withOriginalSourceEvidence(route(C::Acquisition::Gift,4,3),
     U::EvidenceKind::UnknownOriginalHistory),
  A::withOriginalSourceEvidence(route(C::Acquisition::EventDistribution,5,3),
     B::Class::Unknown),
  A::withOriginalSourceEvidence(route(C::Acquisition::GameCubeShadow,6,3),
     GC::Status::ConditionalGameCubeTransferOnly),
  A::withOriginalSourceEvidence(route(C::Acquisition::Radar,7,4),
     G4::map(G4::Sinnoh::Status::UnknownRadarActivation))
 }};
 // This is a route-level source check only, never breeding PID/hatch proof.
 // Use the genuine Gen IV static source adapter to avoid fabricating an
 // original Native condition out of a gift egg template.
 alternatives[2].evidence=A::map(G4::map(
   G4::Static::Status::ConditionalOriginalGiftEggSourceAwaitingBreedingVerification));
 for(const auto& r:alternatives)
   assert(!C::evaluate(r).globalInvalidSupported);
 auto mix=C::reconcile(0x30000001,true,alternatives);
 assert(mix.conditional==3&&mix.contradicted==1&&mix.unknown==3);
 assert(mix.status==C::UniverseStatus::AtLeastOneConditionalRoute);
 assert(!mix.globalInvalidSupported&&!mix.globallyLegalSupported);
 verified+=5;
 // A false-negative claim in a wild source must never erase the independently
 // sourced gift egg/event, and a missing egg hatching/evolution must be Unknown.
 auto egg=alternatives[2];
 egg.presentSpecies=26;
 assert(C::evaluate(egg).status==C::Status::LocalPresentMismatch);
 egg.currentSpecimenIdentityAndSnapshotIndependentlyAttested=false;
 assert(C::evaluate(egg).status==C::Status::UnknownPresentSnapshot);
 verified+=2;
 // A bounded GameCube CPU-team search is not a failed native encounter.
 alternatives[5].evidence=A::map(GC::Status::UnknownBoundedTeamSearch);
 mix=C::reconcile(0x30000001,true,alternatives);
 assert(mix.conditional==2&&mix.unknown==4&&mix.contradicted==1);
 verified+=2;
 // Even all seven independently supplied routes contradicted cannot return
 // global Invalid, because an unmodeled Time Capsule/event/egg may exist.
 for(auto& r:alternatives)r.evidence=E::LocalSourceContradiction;
 mix=C::reconcile(0x30000001,true,alternatives);
 assert(mix.status==C::UniverseStatus::OnlyProvidedRoutesLocallyContradicted);
 assert(mix.contradicted==7&&mix.conditional==0);
 assert(!mix.globalInvalidSupported&&!mix.globallyLegalSupported);
 verified+=3;
 // 256-byte adversarial family and status corruption, including unknown
 // source status enum values, must fail closed for all five adapters.
 for(unsigned raw=128;raw<256;++raw) {
   assert(A::map(static_cast<B::Class>(raw))==E::Unknown);
   assert(A::map(static_cast<U::EvidenceKind>(raw))==E::Unknown);
   assert(A::map(static_cast<G4::Core::Stage>(raw))==E::Unknown);
   assert(A::map(static_cast<GC::Status>(raw))==E::Unknown);
   assert(A::map(static_cast<NPC::Status>(raw))==E::Unknown);
   verified+=5;
 }
 std::cout<<"L1 Stage6 typed cross-origin adapters: "<<verified
          <<" route and enum checks PASS (no global legality verdict)\n";
}
