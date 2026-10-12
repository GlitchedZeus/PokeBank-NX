#pragma once
// L1 stage 5: read-only informational classification of ORIGINAL-source
// candidate statuses from all twelve independent Stage 1..4 helpers.
// Unknown is the fail-closed default; a matching stage is never global Legal.
// No enum has a mapping to global Invalid, and no dispatcher infers provenance.
#include "Legality/Gen1OriginalWildFishingStage.h"
#include "Legality/Gen2OriginalFishingStage.h"
#include "Legality/Gen2OriginalTreeContestStage.h"
#include "Legality/Gen2OriginalSwarmStage.h"
#include "Legality/Gen2OriginalRoamerRouteStage.h"
#include "Legality/Gen3OriginalWildSourceStage.h"
#include "Legality/Gen3OriginalWildRngWitness.h"
#include "Legality/Gen3OriginalRoamerFrameWitness.h"
#include "Legality/Gen3GameCubeOriginalNativeStage.h"
#include "Legality/Gen4HgssOriginalTreeStage.h"
#include "Legality/Gen4SinnohOriginalHoneyHistory.h"
#include "Legality/Gen4OriginalJkNatureStage.h"
#include <cstdint>

namespace Legality::Gen14OriginalEvidenceBridge {
// StageOnly: one witness or a historical-world/template predicate matched.
// ConditionalNative: a candidate-specific original identity was independently
// supplied and the *bounded* original GameCube stage matched it. This is NOT
// full chain proof, as purification/gifts/other candidates remain.
enum class Class : uint8_t { Unknown, CandidateLocalContradiction, SourceOnly,
                              RngStageOnly, ConditionalNative };
namespace G1=Gen1OriginalWildFishingStage;
namespace F2=Gen2OriginalFishingStage;
namespace T2=Gen2OriginalTreeContestStage;
namespace S2=Gen2OriginalSwarmStage;
namespace R2=Gen2OriginalRoamerRouteStage;
namespace W3=Gen3OriginalWildSourceStage;
namespace RNG3=Gen3OriginalWildRngWitness;
namespace Roam3=Gen3OriginalRoamerFrameWitness;
namespace GC3=Gen3GameCubeOriginalNativeStage;
namespace Tree4=Gen4HgssOriginalTreeStage;
namespace Honey4=Gen4SinnohOriginalHoneyHistory;
namespace JK4=Gen4OriginalJkNatureStage;

constexpr Class classify(G1::Status s) noexcept {
  using S=G1::Status;
  switch(s){
    case S::SourceSlotCompatibleRateAndHistoryUnverified:return Class::SourceOnly;
    case S::CandidateOriginalMapContradicted:
    case S::CandidateOriginalMethodContradicted:
    case S::CandidateOriginalSpeciesContradicted:
    case S::CandidateOriginalLevelContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(F2::Status s) noexcept {
  using S=F2::Status;
  switch(s){
    case S::SourceSlotCompatibleWorldAndRngUnverified:return Class::SourceOnly;
    case S::CandidateRodContradicted:case S::CandidateDayNightContradicted:
    case S::CandidateSwarmContradicted:case S::CandidateSpeciesContradicted:
    case S::CandidateLevelContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(T2::Status s) noexcept {
  using S=T2::Status;
  switch(s){
    case S::SourceRowCompatibleHistoryUnverified:return Class::SourceOnly;
    case S::CandidateMapContradicted:case S::CandidateTreeSetContradicted:
    case S::CandidateRarityContradicted:case S::CandidateSpeciesContradicted:
    case S::CandidateLevelContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(S2::Status s) noexcept {
  using S=S2::Status;
  switch(s){
    case S::SourceSwarmRowCompatibleOriginalWorldUnverified:return Class::SourceOnly;
    case S::CandidateTimeContradicted:case S::CandidateMethodContradicted:
    case S::CandidateMapContradicted:case S::CandidateSpeciesContradicted:
    case S::CandidateLevelContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(R2::Status s) noexcept {
  using S=R2::Status;
  switch(s){
    case S::ConditionalSourceTransitionOnly:return Class::SourceOnly;
    case S::CandidateOriginalTransitionContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(W3::Status s) noexcept {
  using S=W3::Status;
  switch(s){
    case S::SourceTemplateCompatibleHistoryUnverified:return Class::SourceOnly;
    case S::CandidateGameContradicted:case S::CandidateMethodContradicted:
    case S::CandidateSpeciesContradicted:case S::CandidateLevelContradicted:
    case S::CandidateRodContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(RNG3::Status s) noexcept {
  using S=RNG3::Status;
  switch(s){
    case S::StageOnlyCompatibleHistoryUnverified:return Class::RngStageOnly;
    case S::CandidateSlotContradicted:case S::CandidateLevelContradicted:
    case S::CandidateNatureContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(Roam3::Status s) noexcept {
  using S=Roam3::Status;
  switch(s){
    case S::OriginalFrameCompatibleHistoryUnverified:return Class::RngStageOnly;
    case S::CandidateSpeciesContradicted:case S::CandidatePidContradicted:
    case S::CandidateIvsContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(GC3::Status s) noexcept {
  using S=GC3::Status;
  switch(s){
    case S::ConditionalGameCubeOriginHistory:return Class::ConditionalNative;
    case S::CandidateNativeOriginContradicted:
    case S::CandidateNativeIdentityContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(Tree4::Status s) noexcept {
  using S=Tree4::Status;
  switch(s){
    case S::ConditionalOriginalTreeGroup:return Class::SourceOnly;
    case S::CandidateTreeGroupContradicted:case S::CandidateTreeEmpty:
      return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(Honey4::Status s) noexcept {
  using S=Honey4::Status;
  switch(s){
    case S::ConditionalOriginalHoneyHistory:return Class::SourceOnly;
    case S::CandidateTimerBeforeAppearance:case S::CandidateTimerExpired:
    case S::CandidateMunchlaxTreeContradicted:case S::CandidateGroupContradicted:
    case S::CandidateSpeciesContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
constexpr Class classify(JK4::Status s) noexcept {
  using S=JK4::Status;
  switch(s){
    case S::ConditionalOriginalJkNatureStage:return Class::RngStageOnly;
    case S::CandidateNatureStageContradicted:
    case S::CandidatePidContradicted:return Class::CandidateLocalContradiction;
    default:return Class::Unknown;
  }
}
// Require an overload from this file, not an implicit enum/integer conversion.
template<class Status>
constexpr Class map(const Status status) noexcept {return classify(status);}
} // namespace Legality::Gen14OriginalEvidenceBridge
