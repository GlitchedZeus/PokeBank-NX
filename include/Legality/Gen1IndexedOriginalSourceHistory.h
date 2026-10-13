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
    UnknownOriginalCatchRate, UnknownOriginalSpecies, UnknownOriginalLevel,
    UnknownEncounterKind, UnknownCurrentSpecies,
    UnknownLaterGenerationEvolution, ForeignGameRowNotEvidence,
    CandidateOriginalSpeciesContradicted, CandidateOriginalLevelContradicted,
    CandidateEvolutionContradicted, CandidateOriginalCatchRateContradicted,
    ConditionalOriginalSource,
    UnknownOriginalMap, UnknownOriginalRequest, UnknownOriginalTradeReceipt,
    UnknownUnreachableNpcTrade, CandidateOriginalMapContradicted,
    CandidateOriginalRequestContradicted, UnknownOriginalFossilReceipt,
    ConditionalPretStaticSceneOnly,
    ConditionalPretNpcTradeOfferOnly,
    CandidateOriginalFossilMismatch, UnknownOriginalSubmittedLevel,
    UnknownOriginalReceivedLevel, CandidateOriginalTradeLevelMismatch
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
    bool originalSpeciesIndependentlyAttested=false;
    bool originalLevelIndependentlyAttested=false;
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
    // Original retail source graph handles Eevee branches and every
    // two-stage family without the numeric-neighbor false-positive risk.
    return Gen1Encounter::canDescendFrom(current,source);
}

inline Result evaluate(const Candidate& c) noexcept {
    if(!c.originalGameIndependentlyAttested ||
       !Gen1Encounter::gameMaskForId(c.exactOriginalGame))
        return {};
    if(c.kind!=Kind::Static && c.kind!=Kind::InGameTrade)
        return {Status::UnknownEncounterKind};
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
    if(c.noTimeCapsuleHistoryIndependentlyAttested &&
       c.timeCapsuleHistoryIndependentlyAttested)
        return {Status::UnknownTradebackHistory,species,minimum};
    if((nativeMask&mask)==0)
        return {Status::ForeignGameRowNotEvidence,species,minimum};
    // Numeric defaults, corrupt source records and detached PK1 guesses are
    // not independently witnessed original acquisition species or levels.
    if(!c.originalSpeciesIndependentlyAttested ||
       c.originalSpecies==0 || c.originalSpecies>151)
        return {Status::UnknownOriginalSpecies,species,minimum};
    if(!c.originalLevelIndependentlyAttested ||
       c.originalLevel==0 || c.originalLevel>100)
        return {Status::UnknownOriginalLevel,species,minimum};
    if(c.originalSpecies!=species)
        return {Status::CandidateOriginalSpeciesContradicted,species,minimum};
    if(c.originalLevel<minimum)
        return {Status::CandidateOriginalLevelContradicted,species,minimum};
    // This helper only has the Gen I (1..151) evolution graph. An
    // acquisition in R/B/Y that later evolved through Gen II+ can be
    // legitimate (Scyther->Scizor, Porygon->Porygon2, Eevee->Espeon).
    // Never turn that unmodeled history into a candidate contradiction.
    if(c.presentSpecies==0)
        return {Status::UnknownCurrentSpecies,species,minimum};
    if(c.presentSpecies>151)
        return {Status::UnknownLaterGenerationEvolution,species,minimum};
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

// Pret ROM-native scene and trade row identities. Separate index namespaces
// from PKHeX kStaticEntries/kTradeEntries: prior indexes remain unchanged.
enum class PretFossilItem : uint8_t { Unverified, Dome, Helix, OldAmber };
// Original Cinnabar Lab source: DOME_FOSSIL->KABUTO,
// HELIX_FOSSIL->OMANYTE, OLD_AMBER->AERODACTYL.
// Player receipts, original items and history remain separate witnesses.
constexpr PretFossilItem sourceFossilForSpecies(uint16_t species) noexcept {
    switch(species) {
    case 140:return PretFossilItem::Dome;
    case 138:return PretFossilItem::Helix;
    case 142:return PretFossilItem::OldAmber;
    default:return PretFossilItem::Unverified;
    }
}
struct PretStaticCandidate {
    std::string_view originalGame{};
    std::size_t originalSceneIndex=0;
    std::string_view originalMap{};
    uint16_t originalSpecies=0;
    uint8_t originalLevel=0;
    bool originalGameIndependentlyAttested=false;
    bool originalSceneIndependentlyAttested=false;
    bool originalMapIndependentlyAttested=false;
    bool originalSpeciesIndependentlyAttested=false;
    bool originalLevelIndependentlyAttested=false;
    bool originalFossilIdentityIndependentlyAttested=false;
    PretFossilItem originalFossilItem=PretFossilItem::Unverified;
};
struct PretTradeCandidate {
    std::string_view originalGame{};
    uint8_t originalOfferIndex=0;
    uint16_t requestedSpecies=0, receivedSpecies=0;
    bool originalGameIndependentlyAttested=false;
    bool originalOfferIndexIndependentlyAttested=false;
    bool originalRequestedSpeciesIndependentlyAttested=false;
    bool originalReceivedSpeciesIndependentlyAttested=false;
    bool originalNpcTrainerIndependentlyAttested=false;
    bool originalTransactionIndependentlyAttested=false;
    uint8_t originalSubmittedLevel=0;
    uint8_t originalReceivedLevel=0;
    bool originalSubmittedLevelIndependentlyAttested=false;
    bool originalReceivedLevelIndependentlyAttested=false;
};
constexpr Result evaluatePretStatic(const PretStaticCandidate& c) noexcept {
    const uint8_t mask=Gen1Encounter::gameMaskForId(c.originalGame);
    if(!mask||!c.originalGameIndependentlyAttested)return {};
    if(!c.originalSceneIndependentlyAttested ||
       c.originalSceneIndex>=Gen1Encounter::kPretStaticSources.size())
       return {Status::UnknownOriginalRow};
    const auto& row=Gen1Encounter::kPretStaticSources[c.originalSceneIndex];
    if((row.gameMask&mask)==0)
       return {Status::ForeignGameRowNotEvidence,row.species,row.level};
    if(!c.originalMapIndependentlyAttested||c.originalMap.empty())
       return {Status::UnknownOriginalMap,row.species,row.level};
    if(!c.originalSpeciesIndependentlyAttested||
       c.originalSpecies==0||c.originalSpecies>151)
       return {Status::UnknownOriginalSpecies,row.species,row.level};
    if(!c.originalLevelIndependentlyAttested||
       !c.originalLevel||c.originalLevel>100)
       return {Status::UnknownOriginalLevel,row.species,row.level};
    if(c.originalMap!=row.originalMap)
       return {Status::CandidateOriginalMapContradicted,row.species,row.level};
    if(c.originalSpecies!=row.species)
       return {Status::CandidateOriginalSpeciesContradicted,row.species,row.level};
    if(c.originalLevel!=row.level)
       return {Status::CandidateOriginalLevelContradicted,row.species,row.level};
    if(row.originalMap=="CinnabarLabFossilRoom") {
       if(!c.originalFossilIdentityIndependentlyAttested ||
          c.originalFossilItem==PretFossilItem::Unverified)
         return {Status::UnknownOriginalFossilReceipt,row.species,row.level};
       if(c.originalFossilItem!=sourceFossilForSpecies(row.species))
         return {Status::CandidateOriginalFossilMismatch,row.species,row.level};
    }
    return {Status::ConditionalPretStaticSceneOnly,row.species,row.level};
}
constexpr Result evaluatePretNpcTrade(const PretTradeCandidate& c) noexcept {
    const uint8_t mask=Gen1Encounter::gameMaskForId(c.originalGame);
    if(!mask||!c.originalGameIndependentlyAttested)return {};
    if(!c.originalOfferIndexIndependentlyAttested)
       return {Status::UnknownOriginalRow};
    const auto* row=Gen1Encounter::originalPretTrade(mask,c.originalOfferIndex);
    if(!row)return {Status::UnknownOriginalRow};
    // Four ROM-stored offers are not reachable in the retail game.
    if(!row->reachable)
       return {Status::UnknownUnreachableNpcTrade,row->receivedSpecies};
    if(!c.originalRequestedSpeciesIndependentlyAttested||
       c.requestedSpecies==0||c.requestedSpecies>151)
       return {Status::UnknownOriginalRequest,row->receivedSpecies};
    if(!c.originalReceivedSpeciesIndependentlyAttested||
       c.receivedSpecies==0||c.receivedSpecies>151)
       return {Status::UnknownOriginalSpecies,row->receivedSpecies};
    if(c.requestedSpecies!=row->requestedSpecies)
       return {Status::CandidateOriginalRequestContradicted,row->receivedSpecies};
    if(c.receivedSpecies!=row->receivedSpecies)
       return {Status::CandidateOriginalSpeciesContradicted,row->receivedSpecies};
    if(!c.originalNpcTrainerIndependentlyAttested)
       return {Status::UnknownTradeOT,row->receivedSpecies};
    if(!c.originalTransactionIndependentlyAttested)
       return {Status::UnknownOriginalTradeReceipt,row->receivedSpecies};
    // Original pret engine/events/in_game_trades.asm copies the donated
    // party monster's level to wCurEnemyLevel and then uses it to create
    // the received NPC species. The offer table contains no fixed level.
    if(!c.originalSubmittedLevelIndependentlyAttested ||
       !c.originalSubmittedLevel || c.originalSubmittedLevel>100)
       return {Status::UnknownOriginalSubmittedLevel,row->receivedSpecies};
    if(!c.originalReceivedLevelIndependentlyAttested ||
       !c.originalReceivedLevel || c.originalReceivedLevel>100)
       return {Status::UnknownOriginalReceivedLevel,row->receivedSpecies};
    if(c.originalSubmittedLevel!=c.originalReceivedLevel)
       return {Status::CandidateOriginalTradeLevelMismatch,row->receivedSpecies};
    // Matching the ROM offer cannot verify original DV/OT or subsequent
    // tradebacks, catch-rate mutations or later evolutions.
    return {Status::ConditionalPretNpcTradeOfferOnly,row->receivedSpecies};
}
} // namespace Legality::Gen1IndexedOriginalSourceHistory
