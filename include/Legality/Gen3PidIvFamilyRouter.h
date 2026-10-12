#pragma once
// L1 / E11: source-family dispatch without treating a failed family match
// as global impossibility. Distinct GBA, GC/XD, Channel and event RNGs are
// never blindly tried as normal handheld wild methods.
#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen3CxdPidIvCorrelation.h"
#include "Legality/Gen3BacdPidIvCorrelation.h"
#include "Legality/Gen3ChannelPidIvCorrelation.h"
#include <array>
#include <cstdint>
namespace Legality::Gen3PidIvFamilyRouter {
enum class Game : uint8_t {
    Unknown,Ruby,Sapphire,Emerald,FireRed,LeafGreen,Colosseum,XD
};
enum class Family : uint8_t {
    Unknown,HandheldWild,TruncatedRoamer,GameCubeShadow,ChannelGift,BacdEvent
};
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalRngFamily, UnknownOriginalTrainer,
    MalformedStoredIvs, UnsupportedOriginalMethod, SearchBoundExhausted,
    CandidateUnresolved, ConditionalRngCorrelation
};
struct Candidate {
    Game originalGame=Game::Unknown;
    Family family=Family::Unknown;
    uint32_t pid=0;
    std::array<uint8_t,6> ivs{};
    uint16_t tid=0,sid=0;
    uint8_t channelOriginGame=0,channelOtGender=0;
    bool familyIndependentlyAttested=false;
    bool originalTrainerIndependentlyAttested=false;
    bool channelStageIndependentlyAttested=false;
    bool ordinaryMethod3Unown=false;
};
struct Result {
    Status status=Status::UnknownOriginalRngFamily;
    uint32_t seed=0;
    Gen3PidIv::Method handheldMethod=Gen3PidIv::Method::None;
    constexpr bool matched()const noexcept{
        return status==Status::ConditionalRngCorrelation;
    }
};
constexpr bool gba(Game g) noexcept {
    return g==Game::Ruby||g==Game::Sapphire||g==Game::Emerald||
           g==Game::FireRed||g==Game::LeafGreen;
}
constexpr Result evaluate(const Candidate& c) noexcept {
    if(c.originalGame==Game::Unknown)return {Status::UnknownOriginalGame};
    if(!c.familyIndependentlyAttested || c.family==Family::Unknown)
        return {Status::UnknownOriginalRngFamily};
    if(!Gen3PidIv::validStoredIVs(c.ivs))
        return {Status::MalformedStoredIvs};
    switch(c.family) {
    case Family::HandheldWild: {
        if(!gba(c.originalGame))return {Status::UnsupportedOriginalMethod};
        const auto r=Gen3PidIv::analyze(c.pid,c.ivs,c.ordinaryMethod3Unown);
        return r.matched() ?
            Result{Status::ConditionalRngCorrelation,r.originSeed,r.method} :
            Result{Status::CandidateUnresolved};
    }
    case Family::TruncatedRoamer: {
        if(c.originalGame!=Game::Ruby&&c.originalGame!=Game::Sapphire&&
           c.originalGame!=Game::FireRed&&c.originalGame!=Game::LeafGreen)
            return {Status::UnsupportedOriginalMethod};
        const auto r=Gen3PidIv::analyzeRoamer(c.pid,c.ivs);
        return r.matched() ?
            Result{Status::ConditionalRngCorrelation,r.originSeed,r.method} :
            Result{Status::CandidateUnresolved};
    }
    case Family::GameCubeShadow: {
        if(c.originalGame!=Game::Colosseum&&c.originalGame!=Game::XD)
            return {Status::UnsupportedOriginalMethod};
        if(!c.originalTrainerIndependentlyAttested)
            return {Status::UnknownOriginalTrainer};
        const auto r=Gen3CxdPidIv::analyzeWithTrainer(
            c.pid,c.ivs,c.tid,c.sid);
        if(r.matched)return {Status::ConditionalRngCorrelation,r.originSeed};
        return {r.searchLimited?Status::SearchBoundExhausted:
                Status::CandidateUnresolved};
    }
    case Family::ChannelGift: {
        // The native Channel gift remains an R/S recipient; the separately
        // persisted Channel origin bit must identify the SAME exact game.
        if(c.originalGame!=Game::Ruby&&c.originalGame!=Game::Sapphire)
            return {Status::UnsupportedOriginalMethod};
        const uint8_t gameBit=c.originalGame==Game::Ruby?2u:1u;
        if(c.channelStageIndependentlyAttested &&
           c.channelOriginGame!=gameBit)
            return {Status::UnsupportedOriginalMethod};
        if(!c.originalTrainerIndependentlyAttested ||
           !c.channelStageIndependentlyAttested)
            return {Status::UnknownOriginalTrainer};
        const auto r=Gen3ChannelPidIv::analyze(
            c.pid,c.ivs,c.sid,c.channelOriginGame,c.channelOtGender);
        return {r.matched?Status::ConditionalRngCorrelation:
                Status::CandidateUnresolved,r.originSeed};
    }
    case Family::BacdEvent: {
        if(!gba(c.originalGame))return {Status::UnsupportedOriginalMethod};
        if(!c.originalTrainerIndependentlyAttested)
            return {Status::UnknownOriginalTrainer};
        const auto r=Gen3BacdPidIv::analyzeWithTrainer(
            c.pid,c.ivs,c.tid,c.sid);
        return {r.matched()?Status::ConditionalRngCorrelation:
                Status::CandidateUnresolved,r.originSeed};
    }
    case Family::Unknown:break;
    }
    return {Status::UnknownOriginalRngFamily};
}
} // namespace Legality::Gen3PidIvFamilyRouter
