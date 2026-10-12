#pragma once
// L1 E04/E05: BOUNDED Gold/Silver/Crystal original-stage Headbutt and
// Bug-Catching Contest row witnesses. Neither match proves a detached PK2.
// Primary sources, exact original decompilation pins:
// pret/pokegold@ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8
// pret/pokecrystal@3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6
// data/wild/treemon_maps.asm, data/wild/treemons.asm,
// data/wild/bug_contest_mons.asm. Source subset only: Route 29,
// Route 43, Ilex Forest; intentionally NOT all encounter locations.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen2OriginalTreeContestStage {
enum class TreeSet : uint8_t { Canyon, Forest, Route, Lake };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalMapRow, UnknownOriginalTreeRow,
    UnknownMapWitness, UnknownOriginalTreeEligibility, UnknownRarityWitness,
    UnknownOriginalSpecies, UnknownOriginalLevel, UnknownContestSession,
    ForeignGameRowNotEvidence, CandidateMapContradicted,
    CandidateTreeSetContradicted, CandidateRarityContradicted,
    CandidateSpeciesContradicted, CandidateLevelContradicted,
    SourceRowCompatibleHistoryUnverified
};
struct MapRow {uint8_t gameMask; std::string_view map; TreeSet set;};
inline constexpr auto kMapRows=std::to_array<MapRow>({
    {3,"Route29",TreeSet::Canyon},
    {4,"Route29",TreeSet::Route},
    {3,"Route43",TreeSet::Canyon},
    {4,"Route43",TreeSet::Lake},
    {3,"IlexForest",TreeSet::Forest},
    {4,"IlexForest",TreeSet::Forest},
});
struct TreeRow {uint8_t gameMask; TreeSet set; bool rare; uint8_t slot; uint16_t species; uint8_t level;};
inline constexpr auto kTreeRows=std::to_array<TreeRow>({
    // GS Canyon common: original physical slots
    {3,TreeSet::Canyon,false,0,21,10},
    {3,TreeSet::Canyon,false,1,21,10},
    {3,TreeSet::Canyon,false,2,21,10},
    {3,TreeSet::Canyon,false,3,190,10},
    {3,TreeSet::Canyon,false,4,190,10},
    {3,TreeSet::Canyon,false,5,190,10},
    // GS Canyon rare: original physical slots
    {3,TreeSet::Canyon,true,0,21,10},
    {3,TreeSet::Canyon,true,1,214,10},
    {3,TreeSet::Canyon,true,2,214,10},
    {3,TreeSet::Canyon,true,3,190,10},
    {3,TreeSet::Canyon,true,4,190,10},
    {3,TreeSet::Canyon,true,5,190,10},
    // G Forest common: original physical slots
    {1,TreeSet::Forest,false,0,10,10},
    {1,TreeSet::Forest,false,1,10,10},
    {1,TreeSet::Forest,false,2,11,10},
    {1,TreeSet::Forest,false,3,102,10},
    {1,TreeSet::Forest,false,4,102,10},
    {1,TreeSet::Forest,false,5,12,10},
    // G Forest rare: original physical slots
    {1,TreeSet::Forest,true,0,10,10},
    {1,TreeSet::Forest,true,1,204,10},
    {1,TreeSet::Forest,true,2,204,10},
    {1,TreeSet::Forest,true,3,102,10},
    {1,TreeSet::Forest,true,4,102,10},
    {1,TreeSet::Forest,true,5,12,10},
    // S Forest common: original physical slots
    {2,TreeSet::Forest,false,0,13,10},
    {2,TreeSet::Forest,false,1,13,10},
    {2,TreeSet::Forest,false,2,14,10},
    {2,TreeSet::Forest,false,3,102,10},
    {2,TreeSet::Forest,false,4,102,10},
    {2,TreeSet::Forest,false,5,15,10},
    // S Forest rare: original physical slots
    {2,TreeSet::Forest,true,0,13,10},
    {2,TreeSet::Forest,true,1,204,10},
    {2,TreeSet::Forest,true,2,204,10},
    {2,TreeSet::Forest,true,3,102,10},
    {2,TreeSet::Forest,true,4,102,10},
    {2,TreeSet::Forest,true,5,15,10},
    // C Forest common: original physical slots
    {4,TreeSet::Forest,false,0,163,10},
    {4,TreeSet::Forest,false,1,204,10},
    {4,TreeSet::Forest,false,2,204,10},
    {4,TreeSet::Forest,false,3,164,10},
    {4,TreeSet::Forest,false,4,12,10},
    {4,TreeSet::Forest,false,5,15,10},
    // C Forest rare: original physical slots
    {4,TreeSet::Forest,true,0,163,10},
    {4,TreeSet::Forest,true,1,10,10},
    {4,TreeSet::Forest,true,2,13,10},
    {4,TreeSet::Forest,true,3,163,10},
    {4,TreeSet::Forest,true,4,11,10},
    {4,TreeSet::Forest,true,5,14,10},
    // C Route common: original physical slots
    {4,TreeSet::Route,false,0,163,10},
    {4,TreeSet::Route,false,1,167,10},
    {4,TreeSet::Route,false,2,165,10},
    {4,TreeSet::Route,false,3,102,10},
    {4,TreeSet::Route,false,4,102,10},
    {4,TreeSet::Route,false,5,102,10},
    // C Route rare: original physical slots
    {4,TreeSet::Route,true,0,163,10},
    {4,TreeSet::Route,true,1,204,10},
    {4,TreeSet::Route,true,2,204,10},
    {4,TreeSet::Route,true,3,102,10},
    {4,TreeSet::Route,true,4,102,10},
    {4,TreeSet::Route,true,5,102,10},
    // C Lake common: original physical slots
    {4,TreeSet::Lake,false,0,163,10},
    {4,TreeSet::Lake,false,1,48,10},
    {4,TreeSet::Lake,false,2,163,10},
    {4,TreeSet::Lake,false,3,102,10},
    {4,TreeSet::Lake,false,4,102,10},
    {4,TreeSet::Lake,false,5,102,10},
    // C Lake rare: original physical slots
    {4,TreeSet::Lake,true,0,163,10},
    {4,TreeSet::Lake,true,1,204,10},
    {4,TreeSet::Lake,true,2,204,10},
    {4,TreeSet::Lake,true,3,102,10},
    {4,TreeSet::Lake,true,4,102,10},
    {4,TreeSet::Lake,true,5,102,10},
});
// Gen II encounter terminator (-1, VENOMOTH, 30, 40) is NOT a slot.
struct ContestRow {uint8_t chance; uint16_t species; uint8_t minimum,maximum;};
inline constexpr auto kContestRows=std::to_array<ContestRow>({
    {20,10,7,18},{20,13,7,18},{10,11,9,18},{10,14,9,18},
    {5,12,12,15},{5,15,12,15},{10,48,10,16},{10,46,10,17},
    {5,123,13,14},{5,127,13,14}
});
constexpr uint8_t gameMask(std::string_view id) noexcept {
    return id=="gold_gbc"?1:id=="silver_gbc"?2:id=="crystal_gbc"?4:0;
}
struct TreeCandidate {
    std::string_view originalGame{},originalMap{};
    std::size_t originalMapRow=0,originalTreeRow=0;
    bool rare=false;
    uint16_t originalSpecies=0;
    uint8_t originalLevel=0;
    bool gameAttested=false,mapRowAttested=false,treeRowAttested=false;
    bool originalMapAttested=false,treeEligibleAttested=false;
    bool originalRarityAttested=false,speciesAttested=false,levelAttested=false;
};
struct Result {Status status=Status::UnknownOriginalGame; uint16_t species=0;};
constexpr Result evaluateTree(const TreeCandidate& c) noexcept {
    const auto gm=gameMask(c.originalGame);
    if(!c.gameAttested || !gm)return {};
    if(!c.mapRowAttested || c.originalMapRow>=kMapRows.size())
        return {Status::UnknownOriginalMapRow};
    if(!c.treeRowAttested || c.originalTreeRow>=kTreeRows.size())
        return {Status::UnknownOriginalTreeRow};
    const auto& map=kMapRows[c.originalMapRow];
    const auto& row=kTreeRows[c.originalTreeRow];
    if(!(gm&map.gameMask) || !(gm&row.gameMask))
        return {Status::ForeignGameRowNotEvidence};
    if(!c.originalMapAttested || c.originalMap.empty())
        return {Status::UnknownMapWitness,row.species};
    if(!c.treeEligibleAttested)
        return {Status::UnknownOriginalTreeEligibility,row.species};
    if(!c.originalRarityAttested)
        return {Status::UnknownRarityWitness,row.species};
    if(!c.speciesAttested || !c.originalSpecies || c.originalSpecies>251)
        return {Status::UnknownOriginalSpecies,row.species};
    if(!c.levelAttested || !c.originalLevel || c.originalLevel>100)
        return {Status::UnknownOriginalLevel,row.species};
    if(c.originalMap!=map.map)
        return {Status::CandidateMapContradicted,row.species};
    if(map.set!=row.set)
        return {Status::CandidateTreeSetContradicted,row.species};
    if(c.rare!=row.rare)
        return {Status::CandidateRarityContradicted,row.species};
    if(c.originalSpecies!=row.species)
        return {Status::CandidateSpeciesContradicted,row.species};
    if(c.originalLevel!=row.level)
        return {Status::CandidateLevelContradicted,row.species};
    return {Status::SourceRowCompatibleHistoryUnverified,row.species};
}
struct ContestCandidate {
    std::string_view originalGame{};
    std::size_t originalContestRow=0;
    uint16_t originalSpecies=0;
    uint8_t originalLevel=0;
    bool gameAttested=false,rowAttested=false,contestSessionAttested=false;
    bool speciesAttested=false,levelAttested=false;
};
constexpr Result evaluateContest(const ContestCandidate& c) noexcept {
    if(!c.gameAttested || !gameMask(c.originalGame))return {};
    if(!c.rowAttested || c.originalContestRow>=kContestRows.size())
        return {Status::UnknownOriginalTreeRow};
    const auto& row=kContestRows[c.originalContestRow];
    if(!c.contestSessionAttested)
        return {Status::UnknownContestSession,row.species};
    if(!c.speciesAttested || !c.originalSpecies || c.originalSpecies>251)
        return {Status::UnknownOriginalSpecies,row.species};
    if(!c.levelAttested || !c.originalLevel || c.originalLevel>100)
        return {Status::UnknownOriginalLevel,row.species};
    if(c.originalSpecies!=row.species)
        return {Status::CandidateSpeciesContradicted,row.species};
    if(c.originalLevel<row.minimum || c.originalLevel>row.maximum)
        return {Status::CandidateLevelContradicted,row.species};
    return {Status::SourceRowCompatibleHistoryUnverified,row.species};
}
} // namespace Legality::Gen2OriginalTreeContestStage