#pragma once
// L1 E14/E16: exact original Diamond/Pearl/Platinum/HeartGold/SoulSilver
// indexed static/gift/roamer source. A detached current PK4, or one row,
// cannot prove an original history nor rule out other event/egg/trade origins.
// The original source method is independently attested: forced shiny/Walker/
// Cute Charm paths must not be silently processed as normal wild Method 1.
#include "Legality/Gen4StaticEncounter.h"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
namespace Legality::Gen4StaticIndexedOriginalHistory {
enum class PidOrigin : uint8_t {
    Unknown, OrdinaryMethod1, CuteCharm, ChainShiny, Pokewalker
};
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalRow, UnknownCaptureStage,
    UnknownOriginalEggStage, UnknownOriginalRoamingRoute,
    UnknownOriginalBall, UnknownOriginalPidOrigin, ForeignGameRowNotEvidence,
    CandidateIdentityContradicted, CandidateCaptureStageContradicted,
    CandidateEggStageContradicted, CandidateBallContradicted,
    CandidateTemplateConstraintsContradicted, CandidatePidFamilyContradicted,
    ConditionalOriginalStaticHistory
};
struct Candidate {
    std::string_view exactOriginalGame{};
    std::size_t originalRowIndex=0;
    uint16_t originalSpecies=0,originalLocation=0,originalEggLocation=0;
    uint8_t originalLevel=0,originalForm=0,originalBall=0xFFu;
    uint8_t originalGender=3,originalNature=25;
    bool originalShiny=false,originalFateful=false,originalIsEgg=false;
    PidOrigin historicalPidOrigin=PidOrigin::Unknown;
    bool originalGameIndependentlyAttested=false;
    bool originalRowIndependentlyAttested=false;
    bool originalCaptureFieldsIndependentlyAttested=false;
    bool originalEggStageIndependentlyAttested=false;
    bool originalRoamingRouteIndependentlyAttested=false;
    bool originalBallIndependentlyAttested=false;
    bool originalPidFamilyIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalGame;
    Gen4Static::PidCategory pinnedCategory=Gen4Static::PidCategory::None;
    bool originalGiftEgg=false;
    bool originalRoamer=false;
};
constexpr bool categoryAccepts(Gen4Static::PidCategory cat,PidOrigin origin) noexcept {
    using C=Gen4Static::PidCategory;
    switch(cat) {
        case C::Method1OrCuteCharm:
            return origin==PidOrigin::OrdinaryMethod1 || origin==PidOrigin::CuteCharm;
        case C::ChainShiny: return origin==PidOrigin::ChainShiny;
        case C::Pokewalker: return origin==PidOrigin::Pokewalker;
        case C::None: return false;
    }
    return false;
}
inline Result evaluate(const Candidate& c) noexcept {
    namespace S=Gen4Static;
    const auto game=Gen4Wild::gameForId(c.exactOriginalGame);
    if(!c.originalGameIndependentlyAttested ||
       game==Gen4Wild::Game::Invalid)return {};
    if(!c.originalRowIndependentlyAttested ||
       c.originalRowIndex>=std::size(S::kPackedGen4StaticEncounters))
        return {Status::UnknownOriginalRow};
    const auto row=S::kPackedGen4StaticEncounters[c.originalRowIndex];
    if(S::game(row)!=game)return {Status::ForeignGameRowNotEvidence};
    const auto category=S::pidCategoryForRow(row);
    const bool egg=S::eggLocation(row)!=0;
    const bool roam=S::roaming(row);
    if(!c.originalCaptureFieldsIndependentlyAttested)
        return {Status::UnknownCaptureStage,category,egg,roam};
    if(c.originalSpecies!=S::species(row) ||
       c.originalForm!=S::form(row))
        return {Status::CandidateIdentityContradicted,category,egg,roam};
    if(!c.originalEggStageIndependentlyAttested)
        return {Status::UnknownOriginalEggStage,category,egg,roam};
    if(c.originalIsEgg!=egg ||
       c.originalEggLocation!=S::eggLocation(row))
        return {Status::CandidateEggStageContradicted,category,egg,roam};
    if(egg){
        // An original unhatched gift keeps met-level 0, even when row
        // carries the template's post-hatch minimum level.
        if(c.originalLevel!=0)
            return {Status::CandidateCaptureStageContradicted,category,egg,roam};
    }else if(c.originalLevel!=S::level(row)){
        return {Status::CandidateCaptureStageContradicted,category,egg,roam};
    }
    if(roam){
        if(!c.originalRoamingRouteIndependentlyAttested)
            return {Status::UnknownOriginalRoamingRoute,category,egg,roam};
        if(!S::roamerLocationAllowed(S::location(row),c.originalLocation))
            return {Status::CandidateCaptureStageContradicted,category,egg,roam};
    }else if(c.originalLocation!=S::location(row))
        return {Status::CandidateCaptureStageContradicted,category,egg,roam};
    if(S::fixedBall(row)!=0){
        if(!c.originalBallIndependentlyAttested)
            return {Status::UnknownOriginalBall,category,egg,roam};
        if(c.originalBall!=S::fixedBall(row))
            return {Status::CandidateBallContradicted,category,egg,roam};
    }
    if(!S::constraintsMatch(row,c.originalGender,c.originalNature,
                            c.originalShiny,c.originalFateful))
        return {Status::CandidateTemplateConstraintsContradicted,category,egg,roam};
    if(!c.originalPidFamilyIndependentlyAttested ||
       c.historicalPidOrigin==PidOrigin::Unknown)
        return {Status::UnknownOriginalPidOrigin,category,egg,roam};
    if(!categoryAccepts(category,c.historicalPidOrigin))
        return {Status::CandidatePidFamilyContradicted,category,egg,roam};
    return {Status::ConditionalOriginalStaticHistory,category,egg,roam};
}
} // namespace Legality::Gen4StaticIndexedOriginalHistory
