#pragma once
// L1 Stage 6 typed adapters for source-specific original encounter receipts.
// Classification does not confer original specimen or custody provenance:
// same original game/save, original capture identity, and independent transfer
// or evolution events remain separately required by the lineage reconciler.
#include "Legality/Gen14CrossOriginReconciliation.h"
#include "Legality/Gen14OriginalEvidenceBridge.h"
#include "Legality/Gen14EncounterCandidateUniverse.h"
#include "Legality/Gen4OriginalEvidenceFamilyAdapter.h"
#include "Legality/Gen3GameCubeOriginalTransferHistory.h"
#include "Legality/Gen2OriginalNpcTradeStage.h"
namespace Legality::Gen14CrossOriginEvidenceAdapter {
namespace C=Gen14CrossOriginReconciliation;
namespace B=Gen14OriginalEvidenceBridge;
namespace U=Gen14EncounterCandidateUniverse;
namespace G4=Gen4OriginalEvidenceFamilyAdapter;
namespace GC=Gen3GameCubeOriginalTransferHistory;
namespace NPC=Gen2OriginalNpcTradeStage;
constexpr C::Evidence map(B::Class evidence) noexcept {
 switch(evidence) {
 case B::Class::Unknown:return C::Evidence::Unknown;
 case B::Class::CandidateLocalContradiction:
     return C::Evidence::LocalSourceContradiction;
 case B::Class::SourceOnly:return C::Evidence::SourceTemplateOnly;
 case B::Class::RngStageOnly:return C::Evidence::OriginalRngStageOnly;
 case B::Class::ConditionalNative:return C::Evidence::ConditionalOriginalNative;
 }
 return C::Evidence::Unknown;
}
constexpr C::Evidence map(U::EvidenceKind evidence) noexcept {
 switch(evidence) {
 case U::EvidenceKind::UnknownOriginalHistory:return C::Evidence::Unknown;
 case U::EvidenceKind::CandidateLocalContradiction:
     return C::Evidence::LocalSourceContradiction;
 case U::EvidenceKind::OriginalTemplateCompatible:
     return C::Evidence::SourceTemplateOnly;
 case U::EvidenceKind::ConditionalOriginalHistory:
     return C::Evidence::ConditionalOriginalNative;
 }
 return C::Evidence::Unknown;
}
constexpr C::Evidence map(G4::Core::Stage stage) noexcept {
 switch(stage) {
 case G4::Core::Stage::Unknown:return C::Evidence::Unknown;
 case G4::Core::Stage::SourceTemplate:return C::Evidence::SourceTemplateOnly;
 case G4::Core::Stage::OriginalRngMatched:
     return C::Evidence::OriginalRngStageOnly;
 case G4::Core::Stage::CandidateContradiction:
     return C::Evidence::LocalSourceContradiction;
 case G4::Core::Stage::BoundedExhausted:
     return C::Evidence::BoundedSearchUnknown;
 }
 return C::Evidence::Unknown;
}
constexpr C::Evidence map(GC::Status status) noexcept {
 switch(status) {
 case GC::Status::ConditionalGameCubeTransferOnly:
     return C::Evidence::ConditionalOriginalNative;
 case GC::Status::CandidateOriginalSourceContradicted:
 case GC::Status::CandidateNativeIdentityContradicted:
 case GC::Status::CandidateNativeIvsContradicted:
 case GC::Status::CandidateOriginalTrainerShinyContradicted:
     return C::Evidence::LocalSourceContradiction;
 case GC::Status::UnknownBoundedTeamSearch:
     return C::Evidence::BoundedSearchUnknown;
 default:return C::Evidence::Unknown;
 }
}
constexpr C::Evidence map(NPC::Status status) noexcept {
 switch(status) {
 case NPC::Status::ConditionalOriginalTradeTemplateOnly:
     return C::Evidence::SourceTemplateOnly;
 case NPC::Status::CandidateOriginalSpeciesContradicted:
 case NPC::Status::CandidateMinimumLevelContradicted:
 case NPC::Status::CandidateFixedTrainerContradicted:
 case NPC::Status::CandidateFixedDVsContradicted:
 case NPC::Status::CandidateCaughtDataContradicted:
     return C::Evidence::LocalSourceContradiction;
 default:return C::Evidence::Unknown;
 }
}
// Every original-source stage status the evidence bridge classifies (for example
// Gen1OriginalWildFishingStage::Status) maps through that classification, so a
// stage without a dedicated overload above still reaches the lineage reconciler.
template<class OriginalSourceStatus>
    requires requires(OriginalSourceStatus status) { B::classify(status); }
constexpr C::Evidence map(OriginalSourceStatus status) noexcept {
 return map(B::classify(status));
}
template<class OriginalSourceStatus>
constexpr C::Route withOriginalSourceEvidence(C::Route route,
                                               OriginalSourceStatus status) noexcept {
 route.evidence=map(status);
 return route;
}
static_assert(map(B::Class::ConditionalNative)==C::Evidence::ConditionalOriginalNative);
static_assert(map(U::EvidenceKind::OriginalTemplateCompatible)==
              C::Evidence::SourceTemplateOnly);
static_assert(map(G4::Core::Stage::BoundedExhausted)==C::Evidence::BoundedSearchUnknown);
static_assert(map(GC::Status::UnknownBoundedTeamSearch)==C::Evidence::BoundedSearchUnknown);
static_assert(map(NPC::Status::UnknownLaterEvolutionOrTimeCapsule)==
              C::Evidence::Unknown);
} // namespace Legality::Gen14CrossOriginEvidenceAdapter
