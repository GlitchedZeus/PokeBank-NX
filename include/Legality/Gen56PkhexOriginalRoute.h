// SPDX-License-Identifier: GPL-3.0-or-later
// L1 Phase A native bounded adapter to PKHeX.Core EncounterPossible5/6.
// Generator source: kwsch/PKHeX at 66ef5983a5d349efa2dd615f2926a1b1ee97306e.
// Original Generator5.cs/Generator6.cs SHA-256 match the owner's PKHeX ZIP.
// This is a SOURCE TEMPLATE adapter, not a replacement for PKHeX GetEncounters,
// PK5/PK6 parsers, binary wild tables, Event databases, breeding, or PID/IV.
// Every route requires an independently identified physical original save.
#pragma once
#include "Legality/Gen56PkhexSourceCatalog.generated.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen56PkhexOriginalRoute {
namespace P=Gen56PkhexSourceCatalog;
using Game=P::Game;using Table=P::Table;using Kind=P::SourceKind;
inline constexpr std::size_t kMaxMatches=64;
enum class Status : uint8_t {
 UnknownOriginalGame, UnknownOriginalSpecimen, UnknownOriginalRow,
 UnknownOriginalSpecies, UnknownOriginalLevel,
 UnknownOriginalLocation, UnknownOriginalEggContext,
 UnknownOriginalForm, UnknownSourceClassification,
 CandidateLocalSpeciesMismatch, CandidateLocalLevelMismatch,
 CandidateLocalLocationMismatch, CandidateLocalEggMismatch,
 CandidateLocalFormMismatch, CandidateLocalShinyMismatch,
 CandidateLocalFlawlessMismatch,
 CandidateLocalForeignGame, UnknownCatalogExhaustion,
 SourceTemplateOnly
};
struct SourceEvidence {
 uint64_t originalSaveId=0,originalSpecimenId=0;
 Game originalGame=static_cast<Game>(0);
 uint16_t originalSpecies=0, originalLevel=0;
 uint16_t originalMetLocation=0,originalEggLocation=0;
 uint8_t originalForm=0;
 bool originalSaveAndSpecimenIndependentlyAttested=false;
 bool originalGameIndependentlyAttested=false;
 bool originalSpeciesAndLevelIndependentlyAttested=false;
 bool originalMetLocationIndependentlyAttested=false;
 bool originalEggLocationIndependentlyAttested=false;
 bool originalFormIndependentlyAttested=false;
 bool originalShinyIndependentlyAttested=false;
 bool originalShiny=false;
 bool originalIVsIndependentlyAttested=false;
 std::array<uint8_t,6> originalIVs{};
};
struct Result {
 Status status=Status::UnknownOriginalGame;
 uint16_t sourceIndex=0;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr uint8_t mask(Game g) noexcept {
 switch(g){
 case Game::W:return 1;case Game::B:return 2;
 case Game::W2:return 4;case Game::B2:return 8;
 case Game::X:return 16;case Game::Y:return 32;
 case Game::AS:return 64;case Game::OR:return 128;
 }return 0;
}
constexpr bool validGame(Game g) noexcept {return mask(g)!=0;}
constexpr bool gen5(Game g) noexcept {return (mask(g)&15)!=0;}
constexpr bool gen6(Game g) noexcept {return (mask(g)&240)!=0;}
constexpr uint16_t maxSpecies(Game g) noexcept {return gen5(g)?649u:721u;}
constexpr Kind tableKind(Table table) noexcept {
 const auto id=static_cast<unsigned>(table);
 return id<P::kTables.size()?P::kTables[id].kind:Kind::StaticOrGift;
}
constexpr const P::Row* row(Table table,uint16_t originalIndex) noexcept {
 for(const auto& r:P::kRows)
   if(r.table==table && r.sourceIndex==originalIndex)return &r;
 return nullptr;
}
// Exact source array ORDER within the static/trade categories of
// EncounterPossible5/6. Event, wild, eggs, Dream World and Dream Radar
// have other source providers; their absence here is always Unknown.
constexpr std::array<Table,4> orderedTables(Game g,Kind kind) noexcept {
 using T=Table;
 if(kind==Kind::NpcTrade) {
 switch(g) {
 case Game::B: return {T::TradeGift_B,T::TradeGift_BW,T::Encounter_BW,T::Encounter_BW};
 case Game::W: return {T::TradeGift_W,T::TradeGift_BW,T::Encounter_BW,T::Encounter_BW};
 case Game::B2:return {T::TradeGift_B2,T::TradeGift_B2W2,T::Encounter_BW,T::Encounter_BW};
 case Game::W2:return {T::TradeGift_W2,T::TradeGift_B2W2,T::Encounter_BW,T::Encounter_BW};
 case Game::X:case Game::Y:
      return {T::TradeGift_XY,T::Encounter_BW,T::Encounter_BW,T::Encounter_BW};
 case Game::AS:case Game::OR:
      return {T::TradeGift_AO,T::Encounter_BW,T::Encounter_BW,T::Encounter_BW};
 } }
 else if(kind==Kind::StaticOrGift) {
 switch(g) {
 case Game::B:return {T::StaticB,T::Encounter_BW,T::Encounter_BW,T::Encounter_BW};
 case Game::W:return {T::StaticW,T::Encounter_BW,T::Encounter_BW,T::Encounter_BW};
 case Game::B2:return {T::StaticB2,T::Encounter_B2W2_Regular,T::Encounter_B2W2_N,T::Encounter_BW};
 case Game::W2:return {T::StaticW2,T::Encounter_B2W2_Regular,T::Encounter_B2W2_N,T::Encounter_BW};
 case Game::X:return {T::StaticX,T::Encounter_XY,T::Encounter_BW,T::Encounter_BW};
 case Game::Y:return {T::StaticY,T::Encounter_XY,T::Encounter_BW,T::Encounter_BW};
 case Game::AS:return {T::StaticA,T::Encounter_AO_Regular,T::Encounter_BW,T::Encounter_BW};
 case Game::OR:return {T::StaticO,T::Encounter_AO_Regular,T::Encounter_BW,T::Encounter_BW};
 } }
 return {T::Encounter_BW,T::Encounter_BW,T::Encounter_BW,T::Encounter_BW};
}
constexpr uint8_t tableCount(Game game,Kind kind) noexcept {
 if(!validGame(game))return 0;
 if(kind!=Kind::StaticOrGift&&kind!=Kind::NpcTrade)return 0;
 if(kind==Kind::NpcTrade)return gen5(game)?2:1;
 return (game==Game::B2||game==Game::W2)?3:2;
}
constexpr Result evaluate(const P::Row& r, const SourceEvidence& e) noexcept {
 if(!e.originalGameIndependentlyAttested||!validGame(e.originalGame))
    return {Status::UnknownOriginalGame,r.sourceIndex};
 if(!e.originalSaveAndSpecimenIndependentlyAttested||
     !e.originalSaveId||!e.originalSpecimenId)
    return {Status::UnknownOriginalSpecimen,r.sourceIndex};
 if((r.gameMask&mask(e.originalGame))==0)
    return {Status::CandidateLocalForeignGame,r.sourceIndex};
 if(!e.originalSpeciesAndLevelIndependentlyAttested||
     !e.originalSpecies||e.originalSpecies>maxSpecies(e.originalGame))
    return {Status::UnknownOriginalSpecies,r.sourceIndex};
 if(!e.originalLevel||e.originalLevel>100)
    return {Status::UnknownOriginalLevel,r.sourceIndex};
 if(r.species!=e.originalSpecies)
    return {Status::CandidateLocalSpeciesMismatch,r.sourceIndex};
 if(r.level!=e.originalLevel)
    return {Status::CandidateLocalLevelMismatch,r.sourceIndex};
 if(r.flags&1u){
    if(!e.originalMetLocationIndependentlyAttested)
       return {Status::UnknownOriginalLocation,r.sourceIndex};
    if(e.originalMetLocation!=r.location)
       return {Status::CandidateLocalLocationMismatch,r.sourceIndex};
 }
 if(r.flags&2u){
    if(!e.originalEggLocationIndependentlyAttested)
       return {Status::UnknownOriginalEggContext,r.sourceIndex};
    if(e.originalEggLocation!=r.eggLocation)
       return {Status::CandidateLocalEggMismatch,r.sourceIndex};
 }
 if(r.flags&4u){
    if(!e.originalFormIndependentlyAttested)
       return {Status::UnknownOriginalForm,r.sourceIndex};
    if(e.originalForm!=r.form)
       return {Status::CandidateLocalFormMismatch,r.sourceIndex};
 }
 if(r.shinyPolicy && e.originalShinyIndependentlyAttested &&
    e.originalShiny==(r.shinyPolicy==1u))
    return {Status::CandidateLocalShinyMismatch,r.sourceIndex};
 if(r.flawlessIVs && e.originalIVsIndependentlyAttested){
    uint8_t flawless=0;
    for(uint8_t iv:e.originalIVs) {
      if(iv>31u)return {Status::UnknownSourceClassification,r.sourceIndex};
      flawless+=iv==31u;
    }
    if(flawless<r.flawlessIVs)
       return {Status::CandidateLocalFlawlessMismatch,r.sourceIndex};
 }
 return {Status::SourceTemplateOnly,r.sourceIndex};
}
enum class ScanStatus : uint8_t {
 UnknownOriginalGame,UnknownOriginalSource,UnknownUnmodeledAlternatives,
 SourceCandidatesOnly,UnknownCapacity
};
struct Hit {
 Table table=Table::Encounter_BW;
 uint16_t sourceIndex=0;
 Status localStatus=Status::UnknownOriginalRow;
};
struct ScanResult {
 ScanStatus status=ScanStatus::UnknownOriginalGame;
 std::array<Hit,kMaxMatches> hits{};
 uint8_t matched=0,unresolved=0;
 bool exhaustiveOriginalHistories=false;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr bool unresolved(Status s) noexcept {
 return s>=Status::UnknownOriginalGame&&s<=Status::UnknownSourceClassification;
}
constexpr bool contradicted(Status s) noexcept {
 return s>=Status::CandidateLocalSpeciesMismatch&&s<=Status::CandidateLocalForeignGame;
}
constexpr ScanResult collect(const SourceEvidence& e,Kind requested) noexcept {
 ScanResult result{};
 if(!e.originalGameIndependentlyAttested||!validGame(e.originalGame))return result;
 if(!e.originalSaveAndSpecimenIndependentlyAttested||
    !e.originalSaveId||!e.originalSpecimenId||
    !e.originalSpeciesAndLevelIndependentlyAttested||
    !e.originalSpecies||e.originalSpecies>maxSpecies(e.originalGame)||
    !e.originalLevel||e.originalLevel>100){
    result.status=ScanStatus::UnknownOriginalSource;return result;
 }
 if(requested!=Kind::NpcTrade&&requested!=Kind::StaticOrGift){
    result.status=ScanStatus::UnknownOriginalSource;return result;
 }
 const auto order=orderedTables(e.originalGame,requested);
 const auto size=tableCount(e.originalGame,requested);
 for(uint8_t ti=0;ti<size;++ti)for(const auto& r:P::kRows){
    if(r.table!=order[ti]||!(r.gameMask&mask(e.originalGame)))continue;
    if(r.species!=e.originalSpecies)continue;
    const auto status=evaluate(r,e).status;
    if(contradicted(status))continue;
    if(result.matched==kMaxMatches){
       result.status=ScanStatus::UnknownCapacity;return result;
    }
    result.hits[result.matched++]={r.table,r.sourceIndex,status};
    result.unresolved+=unresolved(status);
 }
 // Even zero matched source rows does not exclude wild, eggs, events,
 // other original games, trade histories or later-generation transfers.
 result.status=result.matched?ScanStatus::SourceCandidatesOnly:
                              ScanStatus::UnknownUnmodeledAlternatives;
 return result;
}
static_assert(mask(Game::W)==1&&mask(Game::B)==2);
static_assert(mask(Game::AS)==64&&mask(Game::OR)==128);
static_assert(tableCount(Game::B2,Kind::StaticOrGift)==3);
static_assert(tableCount(Game::OR,Kind::NpcTrade)==1);
} // namespace Legality::Gen56PkhexOriginalRoute
