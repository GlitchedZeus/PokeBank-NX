#pragma once
// L4 Stage 5: optional verified per-lane projections feeding the common
// coordinator gate. These preserve candidate-local ambiguity and do not
// turn candidate matches into global legality or source authenticity.
#include "Legality/Gen14EventCoordinatorJoinEvidence.h"
#include "Legality/Gen3DistributionWitnessEvidence.h"
#include "Legality/Gen4MysteryGiftContainerEvidence.h"
#include "Legality/Gen4MysteryGiftHistoryEvidence.h"
#include <cstdint>

namespace Legality::Gen14EventCoordinatorRoutes {
namespace J=Gen14EventCoordinatorJoinEvidence;
namespace G3=Gen3DistributionWitnessEvidence;
namespace G4=Gen4MysteryGiftContainerEvidence;
namespace H4=Gen4MysteryGiftHistoryEvidence;
enum class LaneState : uint8_t { Unavailable, UnknownSource,
    UnsupportedNonGift, UnknownHistory, CandidateExcluded, ConditionalEvidence };
struct Gate {
    LaneState state=LaneState::Unavailable;
    uint32_t contradicted=0,unknown=0;
    bool authenticated=false;
    bool globallyInvalid=false;
};
constexpr Gate classifyGen3(const G3::Result& evidence) noexcept {
    Gate out{};
    if(!evidence.nativeAvailable)return out;
    out.state=LaneState::UnknownSource;
    if(evidence.source!=G3::SourceStatus::ConditionalSource)return out;
    if(evidence.mismatchMask) {
        out.state=LaneState::CandidateExcluded;
        out.contradicted=evidence.mismatchMask;
    } else if(evidence.unknownMask) {
        out.state=LaneState::UnknownHistory;
        out.unknown=evidence.unknownMask;
    } else out.state=LaneState::ConditionalEvidence;
    return out;
}
constexpr Gate classifyGen4(const H4::Result& evidence,
                            const G4::Projection& projection) noexcept {
    Gate out{};
    if(evidence.status==H4::Status::UnavailableNative)return out;
    out.state=LaneState::UnknownSource;
    if(projection.delivery!=G4::Delivery::DirectPokemon &&
       projection.delivery!=G4::Delivery::RangerManaphyEgg) {
        out.state=LaneState::UnsupportedNonGift;return out;
    }
    out.contradicted=evidence.contradicted;
    out.unknown=evidence.unknown;
    if(evidence.status==H4::Status::CandidateContradicted)
        out.state=LaneState::CandidateExcluded;
    else if(evidence.status==H4::Status::ConditionalCandidate)
        out.state=LaneState::ConditionalEvidence;
    else if(evidence.status==H4::Status::UnresolvedHistory ||
            evidence.status==H4::Status::UnknownRangerReceipt)
        out.state=LaneState::UnknownHistory;
    return out;
}
constexpr J::Outcome reconcile(const J::Result& joint,const Gate& lane) noexcept {
    // Never upgrade an unproven source via a second test's compatible row.
    if(joint.outcome!=J::Outcome::ConditionalCandidate)return joint.outcome;
    if(lane.state==LaneState::CandidateExcluded)return J::Outcome::CandidateFieldExcluded;
    if(lane.state!=LaneState::ConditionalEvidence)
        return J::Outcome::UnknownTransferHistory;
    return J::Outcome::ConditionalCandidate;
}
static_assert(reconcile({},{})==J::Outcome::NativeUnavailable);
} // namespace Legality::Gen14EventCoordinatorRoutes
