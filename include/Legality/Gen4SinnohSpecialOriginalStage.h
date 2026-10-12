#pragma once
// L1 E12/E16: exact D/P/Pt Sinnoh wild original row, Honey Tree original
// trainer-dependent source and separately witnessed Radar activation.
// Positive source compatibility is NOT full PID/lead/chain RNG proof.
#include "Legality/Gen4WildEncounter.h"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
namespace Legality::Gen4SinnohSpecialOriginalStage {
namespace W=Gen4Wild;
enum class Path : uint8_t {
    Unknown, OrdinaryWild, HoneyTree, RadarWild, RadarChainShiny
};
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalRow, ForeignGameRowNotEvidence,
    UnknownOriginalCaptureStage, UnknownOriginalPath, UnknownTreeTrainer,
    UnknownRadarActivation, UnknownChainShinyStage,
    CandidateOriginalSpeciesOrFormContradicted,
    CandidateOriginalLocationOrLevelContradicted,
    CandidateOriginalMethodContradicted, CandidateTreeSelectionContradicted,
    CandidateRadarSourceContradicted,
    ConditionalOriginalWildSource, ConditionalOriginalHoneySource,
    ConditionalRadarSourceAwaitingPidReplay
};
struct Candidate {
    std::string_view exactOriginalGame{};
    std::size_t originalRowIndex=0;
    uint16_t originalSpecies=0,originalLocation=0;
    uint8_t originalLevel=0,originalForm=0,originalMethod=0;
    Path originalPath=Path::Unknown;
    uint32_t originalSaveTrainerId32=0;
    bool originalGameIndependentlyAttested=false;
    bool originalRowIndependentlyAttested=false;
    bool originalCaptureStageIndependentlyAttested=false;
    bool originalPathIndependentlyAttested=false;
    bool originalTrainerIdIndependentlyAttested=false;
    bool originalRadarActivationIndependentlyAttested=false;
    bool originalChainShinyStageIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalGame;
    uint8_t originalSlot=0xFF;
    bool radarCapable=false;
    bool trainerDependentTree=false;
};
inline Result evaluate(const Candidate& c) noexcept {
    const auto game=W::gameForId(c.exactOriginalGame);
    if(!c.originalGameIndependentlyAttested ||
       (game!=W::Game::Diamond && game!=W::Game::Pearl &&
        game!=W::Game::Platinum))return {};
    if(!c.originalRowIndependentlyAttested ||
       c.originalRowIndex>=std::size(W::kPackedGen4WildEncounters))
        return {Status::UnknownOriginalRow};
    const uint64_t row=W::kPackedGen4WildEncounters[c.originalRowIndex];
    if(W::game(row)!=game)
        return {Status::ForeignGameRowNotEvidence};
    const uint8_t slot=W::slot(row);
    const bool radar=W::radarCapable(row);
    const bool munchlax=W::species(row)==446u&&W::method(row)==9u;
    if(!c.originalCaptureStageIndependentlyAttested)
        return {Status::UnknownOriginalCaptureStage,slot,radar,munchlax};
    if(!c.originalPathIndependentlyAttested ||
       (c.originalPath!=Path::OrdinaryWild &&
        c.originalPath!=Path::HoneyTree &&
        c.originalPath!=Path::RadarWild &&
        c.originalPath!=Path::RadarChainShiny))
        return {Status::UnknownOriginalPath,slot,radar,munchlax};
    if(c.originalSpecies!=W::species(row) ||
       !W::formMatches(W::form(row),c.originalForm))
        return {Status::CandidateOriginalSpeciesOrFormContradicted,slot,radar,munchlax};
    if(c.originalLocation!=W::location(row) ||
       !W::levelMatches(row,c.originalLevel))
        return {Status::CandidateOriginalLocationOrLevelContradicted,slot,radar,munchlax};
    if(c.originalMethod!=W::method(row))
        return {Status::CandidateOriginalMethodContradicted,slot,radar,munchlax};
    if(c.originalPath==Path::HoneyTree){
        if(W::method(row)!=9u)
            return {Status::CandidateOriginalMethodContradicted,slot,radar,munchlax};
        if(munchlax){
            if(!c.originalTrainerIdIndependentlyAttested)
                return {Status::UnknownTreeTrainer,slot,radar,true};
            if(!W::isMunchlaxTreeLocation(
                    c.originalSaveTrainerId32,c.originalLocation))
                return {Status::CandidateTreeSelectionContradicted,slot,radar,true};
        }
        return {Status::ConditionalOriginalHoneySource,slot,radar,munchlax};
    }
    if(W::method(row)==9u)
        return {Status::CandidateOriginalMethodContradicted,slot,radar,munchlax};
    if(c.originalPath==Path::OrdinaryWild)
        return {Status::ConditionalOriginalWildSource,slot,radar,munchlax};
    if(!radar)
        return {Status::CandidateRadarSourceContradicted,slot,radar,munchlax};
    if(!c.originalRadarActivationIndependentlyAttested)
        return {Status::UnknownRadarActivation,slot,radar,munchlax};
    if(c.originalPath==Path::RadarChainShiny &&
       !c.originalChainShinyStageIndependentlyAttested)
        return {Status::UnknownChainShinyStage,slot,radar,munchlax};
    // A radar-capable source row, and even a separately attested chain
    // stage, do NOT replay the actual chain-shiny PID generation. Reserve
    // the stronger result for a dedicated original PID/IV analysis.
    return {Status::ConditionalRadarSourceAwaitingPidReplay,slot,radar,munchlax};
}
} // namespace Legality::Gen4SinnohSpecialOriginalStage
