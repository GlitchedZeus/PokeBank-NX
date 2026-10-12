#pragma once
// L1 Stage 4: glue actual original GameCube shadow/team-lock and original
// fixed/gift template evaluators to native PK3 transfer reconciliation.
// This layer is informational and strictly keyed to one original capture.
// A matching source table row is not a retail-verified provenance certificate.
#include "Legality/Gen3GameCubeOriginalTransferHistory.h"
#include "Legality/Gen3GameCubeIndexedShadowHistory.h"
#include "Legality/Gen3GameCubeOriginalFixedHistory.h"
#include <array>
#include <cstdint>
namespace Legality::Gen3GameCubeOriginalSourceAdapter {
namespace H=Gen3GameCubeOriginalTransferHistory;
namespace N=Gen3GameCubeOriginalNativeStage;
namespace Shadow=Gen3GameCubeIndexedShadowHistory;
namespace Fixed=Gen3GameCubeOriginalFixedHistory;
constexpr H::SourceCheck mapShadow(Shadow::Status s) noexcept {
    switch(s) {
        case Shadow::Status::ConditionalShadowHistory:
            return H::SourceCheck::SourceMatched;
        case Shadow::Status::CandidateOriginalIdentityContradicted:
        case Shadow::Status::CandidatePidIvContradicted:
        case Shadow::Status::CandidateTeamHistoryContradicted:
            return H::SourceCheck::CandidateContradicted;
        case Shadow::Status::BoundedPidIvUnknown:
        case Shadow::Status::BoundedTeamSearchUnknown:
            return H::SourceCheck::BoundedSearchExhausted;
        default:return H::SourceCheck::Unknown;
    }
}
constexpr H::SourceCheck mapFixed(Fixed::Status s) noexcept {
    switch(s) {
        case Fixed::Status::ConditionalOriginalFixedHistory:
            return H::SourceCheck::SourceMatched;
        case Fixed::Status::CandidateOriginalIdentityContradicted:
        case Fixed::Status::CandidateOriginalPidIvContradicted:
            return H::SourceCheck::CandidateContradicted;
        case Fixed::Status::BoundedOriginalRngUnresolved:
            return H::SourceCheck::BoundedSearchExhausted;
        default:return H::SourceCheck::Unknown;
    }
}
constexpr bool sameNativeShadowCapture(const H::Candidate& a,
                                        const Shadow::Candidate& b) noexcept {
    return a.native.originalRoute==N::Route::ShadowPurified &&
           static_cast<uint8_t>(a.native.originalGame)==
               static_cast<uint8_t>(b.originalGame) &&
           a.native.originalSpecies==b.originalSpecies &&
           a.native.originalPersonality==b.originalPid &&
           a.originalCapturedIvs==b.originalIVs &&
           a.native.native.originGame==b.originalOriginVersion &&
           (a.native.originalGame!=N::Game::XD ||
                (a.originalTrainerTid==b.originalTid &&
                 a.originalTrainerSid==b.originalSid));
}
constexpr bool sameNativeFixedCapture(const H::Candidate& a,
                                       const Fixed::Candidate& b) noexcept {
    const bool bonus=b.originalNative.originGame!=15u;
    const auto scene=bonus?N::Route::BonusDiscReward:N::Route::FixedGiftOrTrade;
    return a.native.originalRoute==scene &&
           static_cast<uint8_t>(a.native.originalGame)==
               static_cast<uint8_t>(b.originalGame) &&
           a.native.originalSpecies==b.originalNative.species &&
           a.native.originalPersonality==b.originalNative.pid &&
           a.originalCapturedIvs==b.originalNative.ivs &&
           a.native.native.originGame==b.originalNative.originGame &&
           a.native.originalTrainerId==
               (static_cast<uint32_t>(b.originalNative.sid)<<16 |
                static_cast<uint32_t>(b.originalNative.tid));
}
inline H::Result evaluateShadow(H::Candidate transfer,
                                const Shadow::Candidate& originalShadow) {
    if(!sameNativeShadowCapture(transfer,originalShadow))
        return {H::Status::UnknownOriginalSource};
    const auto source=Shadow::evaluate(originalShadow);
    transfer.originalSourceEvaluatorIndependentlyReplayed=true;
    transfer.sourceCheck=mapShadow(source.status);
    return H::evaluate(transfer);
}
inline H::Result evaluateFixed(H::Candidate transfer,
                               const Fixed::Candidate& originalFixed) {
    if(!sameNativeFixedCapture(transfer,originalFixed))
        return {H::Status::UnknownOriginalSource};
    const auto source=Fixed::evaluate(originalFixed);
    transfer.originalSourceEvaluatorIndependentlyReplayed=true;
    transfer.sourceCheck=mapFixed(source.status);
    return H::evaluate(transfer);
}
static_assert(mapShadow(Shadow::Status::BoundedTeamSearchUnknown)==
              H::SourceCheck::BoundedSearchExhausted);
static_assert(mapFixed(Fixed::Status::UnknownOriginalTrainerAndText)==
              H::SourceCheck::Unknown);
} // namespace Legality::Gen3GameCubeOriginalSourceAdapter
