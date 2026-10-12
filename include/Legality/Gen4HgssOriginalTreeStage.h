#pragma once
// L1 Stage 4: original HGSS Headbutt tree assignment by *full save TID32*.
// Source: reverse-engineered HGSS Headbutt tree table and tree-index rule at
// https://bulbapedia.bulbagarden.net/wiki/Headbutt_(move)/Wild_Pok%C3%A9mon
// https://projectpokemon.org/home/forums/topic/41693-hgss-encounter-file-specification/
// This identifies one tree's A/B/empty group only; it does not reconstruct
// a native tree's species/level/encounter RNG or its original coordinate.
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen4HgssOriginalTreeStage {
enum class Group : uint8_t { Unknown, A, B, Empty, SpecialC };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalTrainer, UnknownTreeList,
    UnknownTreeIdentity, UnknownSpecialTreeSource, UnknownObservedGroup,
    CandidateTreeGroupContradicted, CandidateTreeEmpty,
    ConditionalOriginalTreeGroup
};
// Published HGSS game-reversed lookup. '-' is a tree with no encounter;
// extra trailing columns are explicitly absent, not Group A defaults.
inline constexpr std::array<std::string_view, 4> kEncounterGroups{{
    "ABBAABBAABBAABBAABBA", // 2-tree column, 20 encounter IDs,
    "AB-A-BBA-B-A-AB-BAAB-A-BBA-B-A", // 3-tree column, 30 encounter IDs,
    "-AABB-AABB-AABB-AABB-ABAB-ABAB-ABAB-ABAB", // 4-tree column, 40 encounter IDs,
    "-AABBB-AABBB-AAABB-AAABB--ABABB-ABAAB-ABBAB-AABAB-", // 5-tree column, 50 encounter IDs
}};
constexpr Group decode(char c) noexcept {
    return c=='A'?Group::A:c=='B'?Group::B:
           c=='-'?Group::Empty:Group::Unknown;
}
constexpr Group groupFor(uint8_t designatedTrees, uint8_t originalTreeId,
                         uint32_t originalFullTrainerId) noexcept {
    if(designatedTrees==0 || designatedTrees>45 ||
       originalTreeId>=designatedTrees)return Group::Unknown;
    const uint8_t x=static_cast<uint8_t>(originalFullTrainerId%10u);
    if(designatedTrees==1) return x%2u ? Group::B : Group::A; // unused source case
    const uint8_t column=designatedTrees>=5?5:designatedTrees;
    const uint8_t index=static_cast<uint8_t>(column==5?
        originalTreeId%5u+5u*x:originalTreeId+column*x);
    if(column==5) {
        // Can never exceed 49, even for 45 physical trees.
        return decode(kEncounterGroups[3][index]);
    }
    return index<kEncounterGroups[column-2].size() ?
        decode(kEncounterGroups[column-2][index]):Group::Unknown;
}
struct Candidate {
    std::string_view exactOriginalGame{};
    uint32_t originalFullTrainerId=0;
    uint8_t originalDesignatedTreeCount=0,originalTreeId=0;
    Group witnessedGroup=Group::Unknown;
    bool originalSaveIndependentlyAttested=false;
    bool fullTrainerIdIndependentlyAttested=false;
    bool originalTreeListIndependentlyAttested=false;
    bool physicalTreeIdIndependentlyAttested=false;
    bool originalGroupIndependentlyAttested=false;
    bool specialTreeC=false;
    bool specialTreePositionIndependentlyAttested=false;
};
struct Result { Status status=Status::UnknownOriginalGame; Group computed=Group::Unknown; uint8_t encounterId=0; };
constexpr Result evaluate(const Candidate& c) noexcept {
    if(c.exactOriginalGame!="heartgold_nds" && c.exactOriginalGame!="soulsilver_nds")return {};
    if(!c.originalSaveIndependentlyAttested || !c.fullTrainerIdIndependentlyAttested)
        return {Status::UnknownOriginalTrainer};
    if(!c.originalTreeListIndependentlyAttested || c.originalDesignatedTreeCount==0 ||
       c.originalDesignatedTreeCount>45)return {Status::UnknownTreeList};
    if(!c.physicalTreeIdIndependentlyAttested ||
       (!c.specialTreeC && c.originalTreeId>=c.originalDesignatedTreeCount))
        return {Status::UnknownTreeIdentity};
    if(c.specialTreeC){
        if(!c.specialTreePositionIndependentlyAttested)
            return {Status::UnknownSpecialTreeSource};
        if(!c.originalGroupIndependentlyAttested || c.witnessedGroup==Group::Unknown)
            return {Status::UnknownObservedGroup,Group::SpecialC};
        return c.witnessedGroup==Group::SpecialC ?
            Result{Status::ConditionalOriginalTreeGroup,Group::SpecialC}:
            Result{Status::CandidateTreeGroupContradicted,Group::SpecialC};
    }
    const Group found=groupFor(c.originalDesignatedTreeCount,c.originalTreeId,c.originalFullTrainerId);
    if(found==Group::Unknown)return {Status::UnknownTreeIdentity};
    if(!c.originalGroupIndependentlyAttested || c.witnessedGroup==Group::Unknown)
        return {Status::UnknownObservedGroup,found};
    // Local contradiction can ONLY negate the witnessed original tree. The
    // species could have been caught on a different tree or via another game.
    if(found==Group::Empty)return {Status::CandidateTreeEmpty,found};
    if(c.witnessedGroup!=found)return {Status::CandidateTreeGroupContradicted,found};
    return {Status::ConditionalOriginalTreeGroup,found};
}
} // namespace Legality::Gen4HgssOriginalTreeStage
