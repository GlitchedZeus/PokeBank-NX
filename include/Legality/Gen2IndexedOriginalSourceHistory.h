#pragma once
// L1 E04/E05: exact Gold/Silver/Crystal original wild/static indexed source.
// A Crystal caught-data word can corroborate a source stage but a Gold/Silver
// missing word cannot falsify it. Headbutt tree eligibility, contest session,
// swarm/rod, roamer route and an egg's source all require historical witnesses.
#include "Legality/Gen2WildEncounter.h"
#include "Legality/Gen2StaticEncounter.h"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>

namespace Legality::Gen2IndexedOriginalSourceHistory {

enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalRow, UnknownCaptureStage,
    UnknownOriginalMethod, UnknownOriginalTree, UnknownOriginalContest,
    UnknownOriginalRoamer, UnknownOriginalEggState, UnknownOriginalShiny,
    ForeignGameRowNotEvidence, CandidateSpeciesContradicted,
    CandidateLocationContradicted, CandidateLevelContradicted,
    CandidateTimeContradicted, CandidateEggContradicted,
    CandidateShinyContradicted, ConditionalOriginalSource
};

struct Candidate {
    std::string_view exactOriginalGame{};
    std::size_t originalRowIndex=0;
    uint16_t originalSpecies=0;
    uint8_t originalLocation=0;
    uint8_t originalLevel=0;
    uint8_t originalTime=0; // Original Crystal 2-bit time group; not current clock
    uint8_t originalMethod=0;
    bool originalIsEgg=false;
    bool originalShiny=false;
    bool originalGameIndependentlyAttested=false;
    bool originalRowIndependentlyAttested=false;
    bool originalCaptureStageIndependentlyAttested=false;
    bool originalMethodIndependentlyAttested=false;
    bool originalTreeEligibilityIndependentlyAttested=false;
    bool originalContestSessionIndependentlyAttested=false;
    bool originalRoamerRouteIndependentlyAttested=false;
    bool originalEggStateIndependentlyAttested=false;
    bool originalShinyIndependentlyAttested=false;
    // Only when original Crystal caught-data was independently preserved.
    bool originalCrystalCaughtDataIndependentlyAttested=false;
    uint16_t originalCrystalCaughtData=0;
};

struct Result {
    Status status=Status::UnknownOriginalGame;
    uint8_t sourceMethod=0;
    uint8_t slot=0;
};

inline Result wild(const Candidate& c) noexcept {
    namespace W=Gen2Wild;
    const auto game=W::gameForId(c.exactOriginalGame);
    if(!c.originalGameIndependentlyAttested || game==W::Game::Invalid)
        return {};
    if(!c.originalRowIndependentlyAttested ||
       c.originalRowIndex>=std::size(W::kPackedGen2WildEncounters))
        return {Status::UnknownOriginalRow};
    const auto row=W::kPackedGen2WildEncounters[c.originalRowIndex];
    if(W::game(row)!=game)
        return {Status::ForeignGameRowNotEvidence};
    if(!c.originalCaptureStageIndependentlyAttested)
        return {Status::UnknownCaptureStage};
    if(!c.originalMethodIndependentlyAttested)
        return {Status::UnknownOriginalMethod};
    const auto method=W::type(row);
    if(method==6u || method==7u) {
        if(!c.originalTreeEligibilityIndependentlyAttested)
            return {Status::UnknownOriginalTree,method,W::slot(row)};
    }
    if(method==8u && !c.originalContestSessionIndependentlyAttested)
        return {Status::UnknownOriginalContest,method,W::slot(row)};
    if(c.originalSpecies!=W::species(row))
        return {Status::CandidateSpeciesContradicted,method,W::slot(row)};
    if(c.originalLocation!=W::location(row))
        return {Status::CandidateLocationContradicted,method,W::slot(row)};
    if(c.originalLevel<W::minLevel(row) || c.originalLevel>W::maxLevel(row))
        return {Status::CandidateLevelContradicted,method,W::slot(row)};
    if(c.originalMethod!=method)
        return {Status::CandidateTimeContradicted,method,W::slot(row)};
    if(!W::timeAllows(W::timeMask(row),c.originalTime))
        return {Status::CandidateTimeContradicted,method,W::slot(row)};
    if(game==W::Game::Crystal &&
       c.originalCrystalCaughtDataIndependentlyAttested) {
        const uint16_t d=c.originalCrystalCaughtData;
        if(((d>>14)&3u)!=c.originalTime ||
           ((d>>8)&0x3Fu)!=c.originalLevel ||
           (d&0x7Fu)!=c.originalLocation)
            return {Status::CandidateTimeContradicted,method,W::slot(row)};
    }
    return {Status::ConditionalOriginalSource,method,W::slot(row)};
}

inline Result staticOrGift(const Candidate& c) noexcept {
    namespace S=Gen2Static;
    const auto game=S::gameForId(c.exactOriginalGame);
    if(!c.originalGameIndependentlyAttested || game==S::Game::Invalid)
        return {};
    if(!c.originalRowIndependentlyAttested ||
       c.originalRowIndex>=std::size(S::kPackedGen2StaticEncounters))
        return {Status::UnknownOriginalRow};
    const auto row=S::kPackedGen2StaticEncounters[c.originalRowIndex];
    if(S::game(row)!=game)
        return {Status::ForeignGameRowNotEvidence};
    if(!c.originalCaptureStageIndependentlyAttested)
        return {Status::UnknownCaptureStage};
    if(c.originalSpecies!=S::species(row))
        return {Status::CandidateSpeciesContradicted};
    if(c.originalLevel!=S::level(row))
        return {Status::CandidateLevelContradicted};
    if(!c.originalEggStateIndependentlyAttested)
        return {Status::UnknownOriginalEggState};
    if(c.originalIsEgg!=S::isEgg(row))
        return {Status::CandidateEggContradicted};
    if(S::isEgg(row)) {
        // Gift-egg's hatch location is not its fixed acquisition source.
        if(c.originalLocation!=S::location(row))
            return {Status::CandidateLocationContradicted};
    } else if(S::roaming(row)) {
        if(!c.originalRoamerRouteIndependentlyAttested)
            return {Status::UnknownOriginalRoamer};
        if(!S::roamerLocationAllowed(c.originalLocation))
            return {Status::CandidateLocationContradicted};
    } else if(c.originalLocation!=S::location(row)) {
        return {Status::CandidateLocationContradicted};
    }
    if(S::shinyPolicy(row)!=0) {
        if(!c.originalShinyIndependentlyAttested)
            return {Status::UnknownOriginalShiny};
        if((S::shinyPolicy(row)==1 && !c.originalShiny) ||
           (S::shinyPolicy(row)==2 && c.originalShiny))
            return {Status::CandidateShinyContradicted};
    }
    if(game==S::Game::Crystal &&
       c.originalCrystalCaughtDataIndependentlyAttested &&
       !S::isEgg(row)) {
        const uint16_t d=c.originalCrystalCaughtData;
        if(S::caughtMetLevel(d)!=c.originalLevel ||
           S::caughtMetLocation(d)!=c.originalLocation)
            return {Status::CandidateLocationContradicted};
    }
    return {Status::ConditionalOriginalSource};
}
} // namespace Legality::Gen2IndexedOriginalSourceHistory
