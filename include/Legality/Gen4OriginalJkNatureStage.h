#pragma once
// L1 Stage 4 original Method J/K nature/PID forward evidence.
// Mirrored and independently replayed from accepted Gen4NatureReversalReplay
// and Gen4WildRngCorrelation: J nature=(rand16/0xA3E), K=(rand16%25).
// This confirms ONLY a witnessed stage, not a species encounter source,
// shiny Radar chain, full IV origin, capture-time activation or legal PK4.
#include <cstdint>
namespace Legality::Gen4OriginalJkNatureStage {
enum class Game : uint8_t { Unknown, Diamond, Pearl, Platinum, HeartGold, SoulSilver };
enum class Lead : uint8_t { Unknown, NoLead, SynchronizeSuccess };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalStage, UnknownOriginalLead,
    UnknownOriginalNature, UnknownOriginalPid,
    UnknownSearchWindow, CandidateNatureStageContradicted,
    CandidatePidContradicted, ConditionalOriginalJkNatureStage
};
struct Candidate {
    Game originalGame=Game::Unknown;
    Lead historicalLead=Lead::Unknown;
    uint32_t witnessedPreNatureState=0,originalPid=0;
    uint8_t witnessedTargetNature=0;
    uint16_t maxPidPairs=128;
    bool originalGameIndependentlyAttested=false;
    bool originalNatureStageIndependentlyAttested=false;
    bool originalLeadIndependentlyAttested=false;
    bool originalPidSnapshotIndependentlyAttested=false;
};
struct Result { Status status=Status::UnknownOriginalGame; uint32_t matchedPid=0; uint16_t rejectedPairs=0; };
constexpr uint32_t next(uint32_t seed) noexcept { return seed*0x41C64E6Du+0x6073u; }
constexpr bool methodK(Game g) noexcept { return g==Game::HeartGold||g==Game::SoulSilver; }
constexpr bool methodJ(Game g) noexcept { return g==Game::Diamond||g==Game::Pearl||g==Game::Platinum; }
constexpr uint8_t rollNature(Game game,uint16_t rand16) noexcept {
    return static_cast<uint8_t>(methodK(game)?rand16%25u:rand16/0xA3Eu);
}
constexpr bool synchronizeSuccess(Game game,uint16_t rand16) noexcept {
    return methodK(game)?((rand16&1u)==0u):((rand16>>15)==0u);
}
constexpr Result evaluate(const Candidate& c) noexcept {
    if(!c.originalGameIndependentlyAttested || (!methodJ(c.originalGame)&&!methodK(c.originalGame)))return {};
    if(!c.originalNatureStageIndependentlyAttested)return {Status::UnknownOriginalStage};
    if(!c.originalLeadIndependentlyAttested || c.historicalLead==Lead::Unknown)
        return {Status::UnknownOriginalLead};
    if(c.witnessedTargetNature>=25)return {Status::UnknownOriginalNature};
    if(!c.originalPidSnapshotIndependentlyAttested)return {Status::UnknownOriginalPid};
    if(c.maxPidPairs==0)return {Status::UnknownSearchWindow};
    uint32_t state=next(c.witnessedPreNatureState);
    const uint16_t rand16=static_cast<uint16_t>(state>>16);
    if(c.historicalLead==Lead::NoLead){
        if(rollNature(c.originalGame,rand16)!=c.witnessedTargetNature)
            return {Status::CandidateNatureStageContradicted};
    }else if(!synchronizeSuccess(c.originalGame,rand16)) {
        // A failed Synchronize activation enters the random-nature path;
        // this helper does not assert its unrecorded extra seed history.
        return {Status::UnknownOriginalLead};
    }
    const uint32_t maxPairs=c.maxPidPairs<4096u?c.maxPidPairs:4096u;
    for(uint32_t i=0;i<maxPairs;++i){
        state=next(state);const uint32_t lo=state>>16;
        state=next(state);const uint32_t pid=(state&0xFFFF0000u)|lo;
        if(pid%25u!=c.witnessedTargetNature)continue;
        return {pid==c.originalPid?Status::ConditionalOriginalJkNatureStage:
                Status::CandidatePidContradicted,pid,static_cast<uint16_t>(i)};
    }
    return {Status::UnknownSearchWindow};
}
} // namespace Legality::Gen4OriginalJkNatureStage
