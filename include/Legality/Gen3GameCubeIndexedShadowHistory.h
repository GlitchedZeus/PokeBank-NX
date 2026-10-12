#pragma once
// L1 E09/E10: exact original Colosseum-versus-XD shadow source and XDRNG
// witness. Both games store PK3 origin=15: that byte alone is not provenance.
// No missing original capture, trainer, Miror B. or CPU-team history implies
// global invalidity. SearchLimit always stays a bounded Unknown.
#include "Legality/Gen3ColoShadowEncounter.h"
#include "Legality/Gen3XdShadowEncounter.h"
#include "Legality/Gen3ColoShadowTeamLock.h"
#include "Legality/Gen3XdShadowTeamLock.h"
#include "Legality/Gen3CxdPidIvCorrelation.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace Legality::Gen3GameCubeIndexedShadowHistory {
enum class Game : uint8_t { Unknown, Colosseum, XD };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalSource, UnknownPhysicalRow,
    UnknownOriginalStage, UnknownOriginalTrainer, UnknownRebattleStage,
    ForeignRowNotEvidence, CandidateOriginalIdentityContradicted,
    CandidatePidIvContradicted, BoundedPidIvUnknown, BoundedTeamSearchUnknown,
    CandidateTeamHistoryContradicted, ConditionalShadowHistory
};
struct Candidate {
    Game originalGame=Game::Unknown;
    std::size_t originalRowIndex=0;
    uint16_t originalSpecies=0,originalLocation=0;
    uint8_t originalLevel=0,originalOriginVersion=0,originalBall=0;
    uint16_t originalTid=0,originalSid=0;
    uint32_t originalPid=0;
    std::array<uint8_t,6> originalIVs{};
    bool originalEgg=false,originalFateful=false,originalShiny=false;
    bool originalGameIndependentlyAttested=false;
    bool originalRowIndependentlyAttested=false;
    bool originalCaptureStageIndependentlyAttested=false;
    bool originalTrainerIndependentlyAttested=false;
    bool originalRebattleIndependentlyAttested=false;
    uint32_t teamSearchBudget=16384;
};
struct Result {
    Status status=Status::UnknownOriginalGame;
    uint8_t shadowIndex=0;
    uint32_t xdrngSeed=0;
    bool xdAntiShiny=false;
};
inline Result evaluate(const Candidate& c) {
    if(!c.originalGameIndependentlyAttested ||
       c.originalGame==Game::Unknown)return {};
    if(!c.originalRowIndependentlyAttested)
        return {Status::UnknownPhysicalRow};
    if(!c.originalCaptureStageIndependentlyAttested)
        return {Status::UnknownOriginalStage};
    if(!Gen3PidIv::validStoredIVs(c.originalIVs))
        return {Status::UnknownOriginalStage};

    if(c.originalGame==Game::Colosseum) {
        if(c.originalRowIndex>=Gen3ColoShadowEncounter::kEntries.size())
            return {Status::UnknownPhysicalRow};
        const auto row=Gen3ColoShadowEncounter::kEntries[c.originalRowIndex];
        if(c.originalSpecies!=row.species ||
           c.originalLevel!=row.level ||
           c.originalLocation!=row.location ||
           c.originalOriginVersion!=15 ||
           c.originalEgg || c.originalFateful)
            return {Status::CandidateOriginalIdentityContradicted};
        const auto rng=Gen3CxdPidIv::analyze(c.originalPid,c.originalIVs);
        if(!rng.matched)return {Status::CandidatePidIvContradicted,row.shadowIndex};
        const auto lock=Gen3ColoShadowTeamLock::validate(
            Gen3ColoShadowEncounter::teamSet(row),
            rng.originSeed,c.teamSearchBudget);
        if(lock==Gen3ColoShadowTeamLock::Result::SearchLimit)
            return {Status::BoundedTeamSearchUnknown,row.shadowIndex,rng.originSeed};
        if(lock!=Gen3ColoShadowTeamLock::Result::Matched)
            return {Status::CandidateTeamHistoryContradicted,row.shadowIndex,rng.originSeed};
        return {Status::ConditionalShadowHistory,row.shadowIndex,rng.originSeed};
    }
    if(c.originalGame!=Game::XD)return {};
    if(c.originalRowIndex>=Gen3XdShadowEncounter::kEntries.size())
        return {Status::UnknownPhysicalRow};
    const auto row=Gen3XdShadowEncounter::kEntries[c.originalRowIndex];
    const bool rebattle=c.originalLocation!=row.location &&
        Gen3XdShadowEncounter::isRebattleLocation(c.originalLocation);
    if(rebattle&&!c.originalRebattleIndependentlyAttested)
        return {Status::UnknownRebattleStage,row.index};
    if(c.originalSpecies!=row.species ||
       c.originalLevel!=row.level ||
       (c.originalLocation!=row.location&&!rebattle) ||
       (row.fixedBall!=0&&c.originalBall!=row.fixedBall) ||
       c.originalOriginVersion!=15 || c.originalEgg ||
       !c.originalFateful || c.originalShiny)
        return {Status::CandidateOriginalIdentityContradicted,row.index};
    if(!c.originalTrainerIndependentlyAttested)
        return {Status::UnknownOriginalTrainer,row.index};
    // XD's stored PK3 shiny flag is not an independent fact. Recheck
    // the actual PID against the original attested TID/SID before
    // permitting a source shadow route (XD shadows were anti-shiny).
    const uint32_t shinyXor=(c.originalPid>>16) ^
        (c.originalPid&0xFFFFu) ^ c.originalTid ^ c.originalSid;
    if(shinyXor<8u)
        return {Status::CandidateOriginalIdentityContradicted,row.index};
    const auto rng=Gen3CxdPidIv::analyzeWithTrainer(
        c.originalPid,c.originalIVs,c.originalTid,c.originalSid);
    if(!rng.matched)
        return {rng.searchLimited?Status::BoundedPidIvUnknown:
                Status::CandidatePidIvContradicted,row.index};
    const auto lock=Gen3XdShadowTeamLock::validateXd(
        row.index,rng.originSeed,c.originalTid,c.originalSid,
        c.teamSearchBudget);
    if(lock==Gen3XdShadowTeamLock::Result::SearchLimit)
        return {Status::BoundedTeamSearchUnknown,row.index,rng.originSeed,
                rng.antiShiny()};
    if(lock!=Gen3XdShadowTeamLock::Result::Matched)
        return {Status::CandidateTeamHistoryContradicted,row.index,
                rng.originSeed,rng.antiShiny()};
    return {Status::ConditionalShadowHistory,row.index,
            rng.originSeed,rng.antiShiny()};
}
} // namespace Legality::Gen3GameCubeIndexedShadowHistory
