#pragma once
// L4 Stage 3: immutable validated PK4 -> independent Gen IV source history.
// Delegates actual decryption/checksum to the existing production-safe
// Pokemon4ReadOnly; never trusts caller-supplied final PK4 fields.
#include "Legality/Gen4MysteryGiftContainerEvidence.h"
#include "Legality/Gen4MysteryGiftHistoryEvidence.h"
#include "Legality/Gen4EventNativeFieldEvidence.h"
#include "Legality/Gen4EventRangerGiftHistory.h"
#include <span>

namespace Legality::Gen4MysteryGiftNativeEvidence {
namespace C=Gen4MysteryGiftContainerEvidence;
namespace H=Gen4MysteryGiftHistoryEvidence;
namespace R=Gen4EventRangerGiftHistory;
struct Result {
    C::Projection card{};
    H::Observed observed{};
    H::Result history{};
    R::Result ranger{};
    bool rangerEvaluated=false;
};
inline Result evaluateNative(const Pokemon::Pokemon4ReadOnly& pk4,
                             std::span<const std::byte> pcd,
                             std::span<const std::byte> pgt,
                             const C::Attestation& archive,
                             const H::Source& source,
                             const R::SourceEvidence& rangerSource={}) noexcept {
    Result out{};
    out.card=pgt.empty()?C::inspect(pcd):C::inspectPair(pcd,pgt);
    const auto snapshot=Gen4EventNativeFieldEvidence::project(pk4);
    if(!snapshot.available) {
        out.history.status=H::Status::UnavailableNative;return out;
    }
    out.observed={true,snapshot.identity.species,pk4.tid(),pk4.sid(),
                  pk4.language(),pk4.originVersion(),snapshot.identity.ball,
                  pk4.isEgg(),pk4.fatefulEncounter(),
                  snapshot.observedMoves,snapshot.nativeRibbonWords,
                  snapshot.rawOT,snapshot.rawNickname};
    H::Source guarded=source;
    // There is no generic "original Ranger proof" bit accepted directly
    // from a caller. Require the existing native PID/IV + recipient/hatch
    // chain to return conditional before even permitting that history gate.
    guarded.rangerGiftProofIndependentlyVerified=false;
    if(out.card.delivery==C::Delivery::RangerManaphyEgg) {
        out.rangerEvaluated=true;
        out.ranger=R::evaluateNative(pk4,rangerSource);
        guarded.rangerGiftProofIndependentlyVerified=
            out.ranger.status==R::Status::ConditionalGiftCandidate;
    }
    out.history=H::evaluate(out.observed,out.card,archive,guarded);
    return out;
}
} // namespace Legality::Gen4MysteryGiftNativeEvidence
