#pragma once
// L1 Stage 3: source-attested, BOUNDED original wild slot/level/nature stages.
// Arithmetic: pret/pokeemerald@731ad5b src/random.c, src/wild_encounter.c;
// pret/pokefirered@037335f src/wild_encounter.c; pret/pokeruby@5784633.
// Caller attests stage-boundary RNG seeds separately. This is NOT a complete
// capture transcript, PID/IV proof or claim of globally illegal alternatives.
#include "Legality/Gen3OriginalWildSourceStage.h"
#include <array>
#include <cstdint>
namespace Legality::Gen3OriginalWildRngWitness {
namespace W=Legality::Gen3OriginalWildSourceStage;
enum class Lead : uint8_t { Unknown, NoLead, Pressure, Hustle, VitalSpirit, Synchronize };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalStage, UnknownOriginalLead,
    UnknownOrUnsupportedLeadPath, UnknownSafariPokeblock,
    UnknownAttestedSlot, UnknownAttestedLevel, UnknownAttestedNature,
    CandidateSlotContradicted, CandidateLevelContradicted,
    CandidateNatureContradicted, StageOnlyCompatibleHistoryUnverified
};
constexpr uint32_t next(uint32_t state) noexcept {return 1103515245u*state+24691u;}
constexpr uint16_t sample(uint32_t state) noexcept {return static_cast<uint16_t>(state>>16);}
inline constexpr std::array<uint8_t,12> kLandThresholds{{20,40,50,60,70,80,85,90,94,98,99,100}};
inline constexpr std::array<uint8_t,5> kWaterThresholds{{60,90,95,99,100}};
inline constexpr std::array<uint8_t,10> kFishThresholds{{70,100,60,80,100,40,80,95,99,100}};
constexpr uint8_t slotFromRoll(W::Method method,W::Rod rod,uint16_t value) noexcept {
    const uint8_t roll=static_cast<uint8_t>(value%100u);
    if(method==W::Method::Grass){
        for(uint8_t i=0;i<kLandThresholds.size();++i)if(roll<kLandThresholds[i])return i;
    }else if(method==W::Method::Surf||method==W::Method::RockSmash){
        for(uint8_t i=0;i<kWaterThresholds.size();++i)if(roll<kWaterThresholds[i])return i;
    }else if(method==W::Method::Fishing){
        uint8_t start=0,end=0;
        if(rod==W::Rod::Old){start=0;end=2;}
        else if(rod==W::Rod::Good){start=2;end=5;}
        else if(rod==W::Rod::Super){start=5;end=10;}
        else return 255;
        for(uint8_t i=start;i<end;++i)if(roll<kFishThresholds[i])return i;
    }
    return 255;
}
struct Stage {
    W::Game exactOriginalGame=W::Game::Unknown;
    std::size_t exactGroupIndex=0;
    W::Rod rod=W::Rod::Unknown;
    Lead originalLead=Lead::Unknown;
    // This is the PRE-SLOT RNG state independently recovered from the
    // ORIGINAL capture stage, not reversely guessed from current PK3 PID.
    uint32_t preSlotSeed=0;
    uint8_t attestedSlot=255,attestedLevel=0;
    // Nature stage can occur with intervening RNG consumption; hence an
    // independently attested separate PRE-NATURE seed, not slot+N shortcut.
    uint32_t preNatureSeed=0;
    uint8_t attestedNature=255,originalSyncNature=255;
    bool originalGameAndGroupAttested=false,originalSlotSeedAttested=false;
    bool originalSlotAndLevelAttested=false,originalLeadAttested=false;
    bool originalNatureSeedAttested=false,originalNatureAttested=false;
    bool originalSyncNatureAttested=false;
    bool originalSafariContextKnown=false,originalInSafari=false;
    bool originalSafariPokeblockPathRuledOut=false;
};
struct Result {
    Status status=Status::UnknownOriginalStage;
    uint8_t calculatedSlot=255,calculatedLevel=0,calculatedNature=255;
    uint32_t stateAfterLevel=0;
};
constexpr Result evaluate(const Stage& c) noexcept {
    if(!c.originalGameAndGroupAttested||c.exactOriginalGame>=W::Game::Unknown)
        return {Status::UnknownOriginalGame};
    if(c.exactGroupIndex>=W::kGroups.size())return {};
    const auto& g=W::kGroups[c.exactGroupIndex];
    if(g.game!=c.exactOriginalGame)return {};
    if(!c.originalSlotSeedAttested || !c.originalNatureSeedAttested)return {};
    if(!c.originalLeadAttested || c.originalLead==Lead::Unknown)
        return {Status::UnknownOriginalLead};
    if(!c.originalSlotAndLevelAttested || c.attestedSlot>=W::slotCount(g))
        return {Status::UnknownAttestedSlot};
    if(c.attestedLevel==0 || c.attestedLevel>100)return {Status::UnknownAttestedLevel};
    if(!c.originalNatureAttested || c.attestedNature>=25)return {Status::UnknownAttestedNature};
    if(g.method==W::Method::Fishing && c.rod==W::Rod::Unknown)
        return {Status::UnknownOriginalStage};
    // Emerald Static/Magnet Pull and Cute Charm can change RNG consumption;
    // unknown or unrelated leads cannot be ruled out by no-lead replay.
    const bool emerald=c.exactOriginalGame==W::Game::Emerald;
    const bool levelAbility=emerald&&(c.originalLead==Lead::Pressure||
        c.originalLead==Lead::Hustle||c.originalLead==Lead::VitalSpirit);
    if(c.originalLead!=Lead::NoLead && c.originalLead!=Lead::Synchronize && !levelAbility)
        return {Status::UnknownOrUnsupportedLeadPath};
    if(!emerald && c.originalLead!=Lead::NoLead)
        return {Status::UnknownOrUnsupportedLeadPath};
    if(!c.originalSafariContextKnown)return {Status::UnknownOriginalStage};
    if(c.originalInSafari && !c.originalSafariPokeblockPathRuledOut)
        return {Status::UnknownSafariPokeblock};
    if(c.originalLead==Lead::Synchronize && (!c.originalSyncNatureAttested || c.originalSyncNature>=25))
        return {Status::UnknownOriginalLead};
    uint32_t s=next(c.preSlotSeed);
    const uint8_t chosen=slotFromRoll(g.method,c.rod,sample(s));
    if(chosen>=W::slotCount(g))return {Status::UnknownOriginalStage};
    if(chosen!=c.attestedSlot)return {Status::CandidateSlotContradicted,chosen};
    const auto source=W::decode(g,chosen);
    if(source.minLevel==0 || source.minLevel>source.maxLevel)return {};
    const uint8_t range=static_cast<uint8_t>(source.maxLevel-source.minLevel+1u);
    s=next(s);
    uint8_t value=static_cast<uint8_t>(sample(s)%range);
    if(levelAbility){
        s=next(s);
        if(sample(s)%2u==0u)value=static_cast<uint8_t>(range-1u);
        else if(value!=0u)--value;
    }
    const uint8_t level=static_cast<uint8_t>(source.minLevel+value);
    if(level!=c.attestedLevel)return {Status::CandidateLevelContradicted,chosen,level,255,s};
    uint32_t ns=next(c.preNatureSeed);
    uint8_t nature=0;
    if(emerald && c.originalLead==Lead::Synchronize && sample(ns)%2u==0u){
        nature=c.originalSyncNature;
    }else{
        if(emerald && c.originalLead==Lead::Synchronize)ns=next(ns);
        nature=static_cast<uint8_t>(sample(ns)%25u);
    }
    if(nature!=c.attestedNature)
        return {Status::CandidateNatureContradicted,chosen,level,nature,s};
    return {Status::StageOnlyCompatibleHistoryUnverified,chosen,level,nature,s};
}
} // namespace Legality::Gen3OriginalWildRngWitness
