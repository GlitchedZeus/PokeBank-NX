#pragma once
// L1 Stage 4: native PK3 <-> original Colosseum/XD source/lock/
// purification/recipient evidence. A detached transferred PK3's origin 15
// belongs to BOTH games. A matching frame or source template is conditional
// only and cannot establish global Legal or global Invalid.
// All save bytes read-only; archive/scene/team receipts come from independent
// original evidence, not guessed from the current PK3.
#include "Legality/Gen3GameCubeOriginalNativeStage.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen3GameCubeOriginalTransferHistory {
namespace N=Gen3GameCubeOriginalNativeStage;
enum class SourceCheck : uint8_t {
    Unknown, CandidateContradicted, BoundedSearchExhausted, SourceMatched
};
enum class Status : uint8_t {
    UnknownOriginalRoot, UnknownScene, UnknownOriginalSource,
    UnknownBoundedTeamSearch, UnknownOriginalTrainer, UnknownPurification,
    UnknownOriginalRecipient, UnknownNativeSnapshot,
    UnknownMalformedNativeIvs, UnknownIndependentNativeIvs,
    UnknownOriginalSourceReplay, CandidateNativeIdentityContradicted,
    CandidateNativeIvsContradicted, CandidateOriginalTrainerShinyContradicted,
    CandidateOriginalSourceContradicted, ConditionalGameCubeTransferOnly
};
struct Candidate {
    N::Candidate native{};
    // These three identities must be separately retained by the original
    // GameCube scene and any original GBA transfer/recipient transcript.
    uint64_t specimenIdentity=0,originalGameCubeSaveIdentity=0;
    uint64_t originalPhysicalSourceRoute=0,recipientSaveIdentity=0;
    bool originalRootIndependentlyAttested=false;
    bool originalSceneAndRouteIndependentlyAttested=false;
    bool originalSourceEvaluatorIndependentlyReplayed=false;
    SourceCheck sourceCheck=SourceCheck::Unknown;
    // Historical capture-stage PID, species and OT are checked by N::evaluate.
    // This separate original IV stage is not an arbitrary present PK3 guess.
    std::array<uint8_t,6> originalCapturedIvs{};
    bool originalIvsIndependentlyAttested=false;
    // For XD shadow anti-shiny, TID/SID cannot be reverse-fitted from PID.
    uint16_t originalTrainerTid=0,originalTrainerSid=0;
    bool originalTrainerIndependentlyAttested=false;
    bool originalShadowCpuTeamHistoryIndependentlyAttested=false;
    bool originalPurificationIndependentlyAttested=false;
    bool originalRecipientTransferIndependentlyAttested=false;
    // Bonuses are Colosseum distributions into native Ruby/Sapphire games.
    // The independently preserved actual recipient save pins this byte;
    // other native Gen III origin bytes cannot be used as substitutes.
    uint8_t attestedBonusRecipientNativeGame=0;
};
struct Result {
    Status status=Status::UnknownOriginalRoot;
    bool recipientOriginIsGba=false,globalInvalidSupported=false;
    bool globallyLegalSupported=false;
    N::Status nativeStatus=N::Status::UnknownOriginalGame;
};
constexpr bool validStoredIvs(const std::array<uint8_t,6>& ivs) noexcept {
    for(uint8_t iv:ivs)if(iv>31)return false;
    return true;
}
constexpr bool shinyUnderOriginalTrainer(uint32_t pid,uint16_t tid,uint16_t sid) noexcept {
    return (((pid>>16)^(pid&0xffffu)^tid^sid)&0xffffu)<8u;
}
constexpr bool recognizedCheck(SourceCheck value) noexcept {
    return value==SourceCheck::Unknown ||
           value==SourceCheck::CandidateContradicted ||
           value==SourceCheck::BoundedSearchExhausted ||
           value==SourceCheck::SourceMatched;
}
inline Result evaluate(const Candidate& c) noexcept {
    const auto route=c.native.originalRoute;
    const auto game=c.native.originalGame;
    if(!c.originalRootIndependentlyAttested || !c.specimenIdentity ||
       !c.originalGameCubeSaveIdentity || !c.originalPhysicalSourceRoute)
        return {};
    if(!N::recognizedGame(game) || !N::recognizedRoute(route) ||
       !c.originalSceneAndRouteIndependentlyAttested)
        return {Status::UnknownScene};
    if(!c.originalSourceEvaluatorIndependentlyReplayed ||
       !recognizedCheck(c.sourceCheck) ||
       c.sourceCheck==SourceCheck::Unknown)
        return {Status::UnknownOriginalSource};
    if(c.sourceCheck==SourceCheck::BoundedSearchExhausted)
        return {Status::UnknownBoundedTeamSearch};
    if(c.sourceCheck==SourceCheck::CandidateContradicted)
        return {Status::CandidateOriginalSourceContradicted};
    const bool shadow=route==N::Route::ShadowPurified;
    const bool bonus=route==N::Route::BonusDiscReward;
    if(shadow) {
        if(!c.originalTrainerIndependentlyAttested ||
           !c.originalShadowCpuTeamHistoryIndependentlyAttested)
            return {Status::UnknownOriginalTrainer};
        if(!c.originalPurificationIndependentlyAttested ||
           !c.native.originalPurificationIndependentlyAttested)
            return {Status::UnknownPurification};
    }
    if(!c.originalRecipientTransferIndependentlyAttested ||
       !c.recipientSaveIdentity ||
       (bonus && (c.attestedBonusRecipientNativeGame!=1 &&
                  c.attestedBonusRecipientNativeGame!=2)))
        return {Status::UnknownOriginalRecipient};
    if(!c.originalIvsIndependentlyAttested)
        return {Status::UnknownIndependentNativeIvs};
    if(!validStoredIvs(c.originalCapturedIvs))
        return {Status::UnknownMalformedNativeIvs};
    if(!c.native.native.checksumAndDecryptionVerified)
        return {Status::UnknownNativeSnapshot};
    const auto nr=N::evaluate(c.native);
    Result out{};
    out.recipientOriginIsGba=bonus;
    out.nativeStatus=nr.status;
    if(nr.status==N::Status::UnknownMalformedNativeIvs)
        out.status=Status::UnknownMalformedNativeIvs;
    else if(nr.status==N::Status::CandidateNativeIdentityContradicted ||
            nr.status==N::Status::CandidateNativeOriginContradicted)
        out.status=Status::CandidateNativeIdentityContradicted;
    else if(nr.status!=N::Status::ConditionalGameCubeOriginHistory)
        out.status=Status::UnknownNativeSnapshot;
    else if(c.originalCapturedIvs!=c.native.native.ivs)
        out.status=Status::CandidateNativeIvsContradicted;
    else if(bonus &&
            (game!=N::Game::Colosseum ||
             c.native.native.originGame!=c.attestedBonusRecipientNativeGame))
        out.status=Status::UnknownOriginalRecipient;
    else if(shadow && game==N::Game::XD &&
            shinyUnderOriginalTrainer(c.native.originalPersonality,
                                      c.originalTrainerTid,
                                      c.originalTrainerSid))
        out.status=Status::CandidateOriginalTrainerShinyContradicted;
    else
        out.status=Status::ConditionalGameCubeTransferOnly;
    return out;
}

inline constexpr std::size_t kMaximumRoutes=16;
enum class UniverseStatus : uint8_t {
    UnknownRoot, UnknownCapacity, UnknownDuplicateRoute,
    UnknownAlternativeHistory, SuppliedRoutesLocallyContradictedOnly,
    OneConditionalTransferHistory
};
struct Alternative {uint64_t routeIdentity=0;Candidate original{};};
struct UniverseResult {
    UniverseStatus status=UniverseStatus::UnknownRoot;
    uint8_t conditional=0,contradicted=0,unknown=0;
    bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr bool locallyContradicted(Status status) noexcept {
    return status==Status::CandidateOriginalSourceContradicted ||
           status==Status::CandidateNativeIdentityContradicted ||
           status==Status::CandidateNativeIvsContradicted ||
           status==Status::CandidateOriginalTrainerShinyContradicted;
}
inline UniverseResult evaluateUniverse(uint64_t specimenRoot,
    bool originalRootAttested,std::span<const Alternative> options) noexcept {
    UniverseResult result{};
    if(!specimenRoot || !originalRootAttested)return result;
    if(options.size()>kMaximumRoutes) {
        result.status=UniverseStatus::UnknownCapacity;return result;
    }
    for(std::size_t i=0;i<options.size();++i) {
        if(!options[i].routeIdentity ||
           options[i].original.specimenIdentity!=specimenRoot) {
            result.status=UniverseStatus::UnknownDuplicateRoute;return result;
        }
        for(std::size_t j=0;j<i;++j) {
            if(options[j].routeIdentity==options[i].routeIdentity) {
                result.status=UniverseStatus::UnknownDuplicateRoute;return result;
            }
        }
    }
    for(const auto& option:options) {
        const auto check=evaluate(option.original);
        if(check.status==Status::ConditionalGameCubeTransferOnly)++result.conditional;
        else if(locallyContradicted(check.status))++result.contradicted;
        else ++result.unknown;
    }
    if(result.conditional)result.status=UniverseStatus::OneConditionalTransferHistory;
    else if(result.unknown || options.empty())
        result.status=UniverseStatus::UnknownAlternativeHistory;
    else result.status=UniverseStatus::SuppliedRoutesLocallyContradictedOnly;
    return result;
}
} // namespace Legality::Gen3GameCubeOriginalTransferHistory
