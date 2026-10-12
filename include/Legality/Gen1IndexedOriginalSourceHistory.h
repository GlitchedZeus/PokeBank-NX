#pragma once
// L1 E02/E03: source-attested Red/Blue/Yellow STATIC or IN-GAME TRADE
// origins. Gen I wild origins are NOT in Gen1EncounterEvidenceData and must
// never be excluded by this subset. Tradeback mutates the catch-rate byte;
// container game, original game and trade/evolution history are distinct.
#include "Legality/Gen1EncounterEvidence.h"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>

namespace Legality::Gen1IndexedOriginalSourceHistory {

enum class Kind : uint8_t { Static, InGameTrade };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalSource, UnknownOriginalRow,
    UnknownEvolutionChain, UnknownTradeOT, UnknownTradebackHistory,
    UnknownOriginalCatchRate, ForeignGameRowNotEvidence,
    CandidateOriginalSpeciesContradicted, CandidateOriginalLevelContradicted,
    CandidateEvolutionContradicted, CandidateOriginalCatchRateContradicted,
    ConditionalOriginalSource
};

struct Candidate {
    Kind kind=Kind::Static;
    std::string_view exactOriginalGame{};
    std::size_t originalRowIndex=0;
    uint16_t originalSpecies=0;
    uint16_t presentSpecies=0;
    uint8_t originalLevel=0;
    uint8_t originalCatchRate=0;
    // Original game and source row are independently attested: not guessed
    // from a detached PK1's container version or its current species.
    bool originalGameIndependentlyAttested=false;
    bool originalSourceRowIndependentlyAttested=false;
    bool originalEvolutionChainIndependentlyAttested=false;
    bool originalTradeOTIndependentlyAttested=false;
    bool originalCatchRateIndependentlyAttested=false;
    // A Game Boy Time Capsule tradeback is a separate, witnessed history;
    // its held-item byte may replace the original PK1 catch-rate byte.
    bool noTimeCapsuleHistoryIndependentlyAttested=false;
    bool timeCapsuleHistoryIndependentlyAttested=false;
};

struct Result {
    Status status=Status::UnknownOriginalGame;
    uint16_t sourceSpecies=0;
    uint8_t minimumSourceLevel=0;
    bool potentialTradeback=false;
};

constexpr bool possibleOriginalEvolution(uint16_t source,uint16_t current) noexcept {
    if(source==current && source!=0 && source<=151)return true;
    if(source==0 || current==0 || current>151)return false;
    // Eevee's three evolutions are siblings, never one another's parents.
    if(current>=134 && current<=136)return source==133;
    return Gen1Encounter::canDescendFrom(current,source);
}

inline Result evaluate(const Candidate& c) noexcept {
    if(!c.originalGameIndependentlyAttested ||
       !Gen1Encounter::gameMaskForId(c.exactOriginalGame))
        return {};
    if(!c.originalSourceRowIndependentlyAttested)
        return {Status::UnknownOriginalRow};
    const uint8_t mask=Gen1Encounter::gameMaskForId(c.exactOriginalGame);
    uint16_t species=0;
    uint8_t minimum=0,nativeMask=0;
    if(c.kind==Kind::Static) {
        if(c.originalRowIndex>=Gen1Encounter::kStaticEntries.size())
            return {Status::UnknownOriginalRow};
        const auto& row=Gen1Encounter::kStaticEntries[c.originalRowIndex];
        species=row.species;
        minimum=row.minimumLevel;
        nativeMask=row.nativeGameMask;
    } else {
        if(c.originalRowIndex>=Gen1Encounter::kTradeEntries.size())
            return {Status::UnknownOriginalRow};
        const auto& row=Gen1Encounter::kTradeEntries[c.originalRowIndex];
        species=row.species;
        minimum=row.minimumLevelRBY;
        nativeMask=row.nativeGameMask;
        if(!c.originalTradeOTIndependentlyAttested)
            return {Status::UnknownTradeOT,species,minimum};
        // "Definitely never traded back" and "definitely traded back"
        // are mutually exclusive source histories. Conflicting witness
        // flags are not permission to pick a stricter source level.
        if(c.noTimeCapsuleHistoryIndependentlyAttested ==
           c.timeCapsuleHistoryIndependentlyAttested)
            return {Status::UnknownTradebackHistory,species,minimum};
        if(c.timeCapsuleHistoryIndependentlyAttested)
            minimum=row.minimumLevelAfterTradeback;
    }
    if((nativeMask&mask)==0)
        return {Status::ForeignGameRowNotEvidence,species,minimum};
    if(c.originalSpecies!=species)
        return {Status::CandidateOriginalSpeciesContradicted,species,minimum};
    if(c.originalLevel<minimum)
        return {Status::CandidateOriginalLevelContradicted,species,minimum};
    if(c.presentSpecies!=species && !c.originalEvolutionChainIndependentlyAttested)
        return {Status::UnknownEvolutionChain,species,minimum};
    if(!possibleOriginalEvolution(species,c.presentSpecies))
        return {Status::CandidateEvolutionContradicted,species,minimum};
    if(!c.originalCatchRateIndependentlyAttested)
        return {Status::UnknownOriginalCatchRate,species,minimum};
    // Check the original acquisition-time byte, NOT PK1's mutable current
    // catch-rate byte. Source-stage original values must be a separate witness.
    const uint8_t required=Gen1CatchRate::expectedRate(c.exactOriginalGame,species);
    if(required!=c.originalCatchRate)
        return {Status::CandidateOriginalCatchRateContradicted,species,minimum};
    return {Status::ConditionalOriginalSource,species,minimum,
            c.timeCapsuleHistoryIndependentlyAttested};
}

} // namespace Legality::Gen1IndexedOriginalSourceHistory
