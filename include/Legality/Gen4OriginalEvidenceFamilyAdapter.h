#pragma once
// L1 Stage 5: exact source-family status adapters. A historically attested
// source row is only a template witness; nature/PID stage without native IVs
// is only an RNG-stage witness. Unknown, corrupt or exhausted histories are
// never silently promoted to a global verdict.
#include "Legality/Gen4OriginalAcquisitionReconciliation.h"
#include "Legality/Gen4SinnohSpecialOriginalStage.h"
#include "Legality/Gen4SinnohOriginalHoneyHistory.h"
#include "Legality/Gen4HgssOriginalTreeStage.h"
#include "Legality/Gen4WildSpecialStageEvidence.h"
#include "Legality/Gen4OriginalJkNatureStage.h"
#include "Legality/Gen4StaticIndexedOriginalHistory.h"
namespace Legality::Gen4OriginalEvidenceFamilyAdapter {
namespace Core=Gen4OriginalAcquisitionReconciliation;
namespace Sinnoh=Gen4SinnohSpecialOriginalStage;
namespace Honey=Gen4SinnohOriginalHoneyHistory;
namespace Tree=Gen4HgssOriginalTreeStage;
namespace Special=Gen4WildSpecialStageEvidence;
namespace Jk=Gen4OriginalJkNatureStage;
namespace Static=Gen4StaticIndexedOriginalHistory;
constexpr Core::Stage map(Sinnoh::Status s) noexcept {
 using S=Sinnoh::Status;
 switch(s) {
 case S::ConditionalOriginalWildSource:
 case S::ConditionalOriginalHoneySource:
 case S::ConditionalRadarSourceAwaitingPidReplay:return Core::Stage::SourceTemplate;
 case S::CandidateOriginalSpeciesOrFormContradicted:
 case S::CandidateOriginalLocationOrLevelContradicted:
 case S::CandidateOriginalMethodContradicted:
 case S::CandidateTreeSelectionContradicted:
 case S::CandidateRadarSourceContradicted:return Core::Stage::CandidateContradiction;
 default:return Core::Stage::Unknown;
 }
}
constexpr Core::Stage map(Honey::Status s) noexcept {
 using S=Honey::Status;
 switch(s) {
 case S::ConditionalOriginalHoneyHistory:return Core::Stage::SourceTemplate;
 case S::CandidateTimerBeforeAppearance:
 case S::CandidateTimerExpired:
 case S::CandidateMunchlaxTreeContradicted:
 case S::CandidateGroupContradicted:
 case S::CandidateSpeciesContradicted:return Core::Stage::CandidateContradiction;
 default:return Core::Stage::Unknown;
 }
}
constexpr Core::Stage map(Tree::Status s) noexcept {
 using S=Tree::Status;
 switch(s) {
 case S::ConditionalOriginalTreeGroup:return Core::Stage::SourceTemplate;
 case S::CandidateTreeGroupContradicted:
 case S::CandidateTreeEmpty:return Core::Stage::CandidateContradiction;
 default:return Core::Stage::Unknown;
 }
}
constexpr Core::Stage map(Special::Status s) noexcept {
 using S=Special::Status;
 switch(s) {
 case S::KnownConditionalSpecialEncounter:return Core::Stage::OriginalRngMatched;
 case S::CandidateRowContradicted:
 case S::CandidatePidIvContradicted:return Core::Stage::CandidateContradiction;
 case S::BoundedCandidateUnresolved:return Core::Stage::BoundedExhausted;
 default:return Core::Stage::Unknown;
 }
}
constexpr Core::Stage map(Jk::Status s) noexcept {
 using S=Jk::Status;
 switch(s) {
 case S::ConditionalOriginalJkNatureStage:return Core::Stage::OriginalRngMatched;
 case S::CandidateNatureStageContradicted:
 case S::CandidatePidContradicted:return Core::Stage::CandidateContradiction;
 case S::UnknownSearchWindow:return Core::Stage::BoundedExhausted;
 default:return Core::Stage::Unknown;
 }
}
constexpr Core::Stage map(Static::Status s) noexcept {
 using S=Static::Status;
 switch(s) {
 case S::ConditionalOriginalStaticHistory:
 case S::ConditionalOriginalGiftEggSourceAwaitingBreedingVerification:
     return Core::Stage::SourceTemplate;
 case S::CandidateIdentityContradicted:
 case S::CandidateCaptureStageContradicted:
 case S::CandidateEggStageContradicted:
 case S::CandidateBallContradicted:
 case S::CandidateTemplateConstraintsContradicted:
 case S::CandidatePidFamilyContradicted:
     return Core::Stage::CandidateContradiction;
 default:return Core::Stage::Unknown;
 }
}
// A mapping never asserts a source row, original clock, historical trainer,
// encounter method or snapshot was verified: callers must supply those
// independent booleans and the physical-original identities separately.
template<class SourceStatus>
constexpr Core::Candidate withStage(Core::Candidate original,
                                     SourceStatus status) noexcept {
 original.stage=map(status);
 return original;
}
static_assert(map(Sinnoh::Status::ConditionalRadarSourceAwaitingPidReplay)==
              Core::Stage::SourceTemplate);
static_assert(map(Honey::Status::UnknownHistoricalTimer)==Core::Stage::Unknown);
static_assert(map(Tree::Status::CandidateTreeEmpty)==Core::Stage::CandidateContradiction);
static_assert(map(Special::Status::UnknownMalformedNativeRecord)==Core::Stage::Unknown);
static_assert(map(Jk::Status::UnknownSearchWindow)==Core::Stage::BoundedExhausted);
static_assert(map(Static::Status::ConditionalOriginalGiftEggSourceAwaitingBreedingVerification)==
              Core::Stage::SourceTemplate);
} // namespace Legality::Gen4OriginalEvidenceFamilyAdapter
