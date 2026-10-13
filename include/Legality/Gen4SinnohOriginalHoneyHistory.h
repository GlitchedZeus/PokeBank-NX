#pragma once
// L1 Stage 4: original D/P/Pt Honey Tree acquisition-state history.
// Source: original saved honey 1440 -> 1080 -> 0 minute countdown, 21 trees,
// three encounter groups; Project Pokemon Gen4 Honey Tree reverse engineering
// https://projectpokemon.org/home/docs/gen-4/honey-tree-r22/
// Munchlax selection: pinned PKHeX HoneyTreeUtil (four TID32-derived indices).
// A current PK4 species, save clock or trainer is NEVER original slather proof.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen4SinnohOriginalHoneyHistory {
enum class Group : uint8_t { Unknown, Common, Rare, Munchlax };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalSave, UnknownTree, UnknownSlather,
    UnknownHistoricalTimer, UnknownTrainer, UnknownSavedSelection,
    CandidateTimerBeforeAppearance, CandidateTimerExpired,
    CandidateMunchlaxTreeContradicted, CandidateGroupContradicted,
    CandidateSpeciesContradicted, ConditionalOriginalHoneyHistory
};
constexpr std::array<uint8_t,21> kOriginalMetLocations{{
    20,20,21,22,23,24,25,25,26,27,27,28,29,30,33,36,37,47,48,49,58
}};
// Tree index, not merely the ambiguous shared met-location number, must be
// independently preserved to confirm a source-specific Munchlax tree.
constexpr std::array<uint8_t,4> munchlaxTreeIndices(uint32_t trainerId) noexcept {
    std::array<uint8_t,4> chosen{{
        static_cast<uint8_t>((trainerId>>24)&255u),
        static_cast<uint8_t>((trainerId>>16)&255u),
        static_cast<uint8_t>((trainerId>>8)&255u),
        static_cast<uint8_t>(trainerId&255u)
    }};
    for(auto& i:chosen)i=static_cast<uint8_t>(i%21u);
    // Retail increment-and-continue overlap quirk, not deduplication.
    for(std::size_t i=1;i<chosen.size();++i)
        for(std::size_t j=0;j<i;++j)
            if(chosen[i]==chosen[j])chosen[i]=static_cast<uint8_t>((chosen[i]+1u)%21u);
    return chosen;
}
constexpr bool permitsMunchlax(uint32_t id,uint8_t tree) noexcept {
    for(auto v:munchlaxTreeIndices(id))if(v==tree)return true;
    return false;
}
struct Candidate {
    std::string_view exactGame{};
    uint8_t originalTreeIndex=255u;
    uint16_t originalMetLocation=0;
    uint16_t originalMinutesRemaining=0;
    uint8_t savedEncounterRow=255u; // six rows, 0..5
    Group savedEncounterGroup=Group::Unknown;
    uint16_t originalSpecies=0;
    uint32_t originalSaveTrainerId32=0;
    bool originalSaveIndependentlyAttested=false;
    bool originalTreeIndependentlyAttested=false;
    bool originalSlatherEventIndependentlyAttested=false;
    bool originalTimerIndependentlyAttested=false;
    bool originalSavedSelectionIndependentlyAttested=false;
    bool originalSpeciesIndependentlyAttested=false;
    bool originalTrainerIdIndependentlyAttested=false;
};
struct Result { Status status=Status::UnknownOriginalGame; Group group=Group::Unknown; };
constexpr Result evaluate(const Candidate& c) noexcept {
    if(c.exactGame!="diamond_nds" && c.exactGame!="pearl_nds" &&
       c.exactGame!="platinum_nds")return {};
    if(!c.originalSaveIndependentlyAttested)return {Status::UnknownOriginalSave};
    if(!c.originalTreeIndependentlyAttested || c.originalTreeIndex>=21 ||
       c.originalMetLocation!=kOriginalMetLocations[c.originalTreeIndex])
        return {Status::UnknownTree};
    if(!c.originalSlatherEventIndependentlyAttested)return {Status::UnknownSlather};
    if(!c.originalTimerIndependentlyAttested || c.originalMinutesRemaining>1440)
        return {Status::UnknownHistoricalTimer};
    if(c.originalMinutesRemaining==0)
        return {Status::CandidateTimerExpired};
    if(c.originalMinutesRemaining>1080)
        return {Status::CandidateTimerBeforeAppearance};
    if(!c.originalSavedSelectionIndependentlyAttested ||
       c.savedEncounterRow>=6 || c.savedEncounterGroup==Group::Unknown)
        return {Status::UnknownSavedSelection};
    if(!c.originalSpeciesIndependentlyAttested || c.originalSpecies==0 ||
       c.originalSpecies>493)return {Status::UnknownSavedSelection,c.savedEncounterGroup};
    if(c.savedEncounterGroup==Group::Munchlax || c.originalSpecies==446u){
        if(!c.originalTrainerIdIndependentlyAttested)
            return {Status::UnknownTrainer,c.savedEncounterGroup};
        if(!permitsMunchlax(c.originalSaveTrainerId32,c.originalTreeIndex))
            return {Status::CandidateMunchlaxTreeContradicted,c.savedEncounterGroup};
    }
    if(c.originalSpecies==446u && c.savedEncounterGroup!=Group::Munchlax)
        return {Status::CandidateGroupContradicted,c.savedEncounterGroup};
    if(c.savedEncounterGroup==Group::Munchlax && c.originalSpecies!=446u)
        return {Status::CandidateSpeciesContradicted,c.savedEncounterGroup};
    // Other groups require a separate versioned six-species-row match; this
    // is slather-state compatibility ONLY, not complete species legality.
    return {Status::ConditionalOriginalHoneyHistory,c.savedEncounterGroup};
}
} // namespace Legality::Gen4SinnohOriginalHoneyHistory
