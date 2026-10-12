#pragma once
// L1 E04/E05: ORIGINAL Gen II special grass / water swarm rows.
// pret/pokegold@ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8
// pret/pokecrystal@3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6
// data/wild/swarm_grass.asm, data/wild/swarm_water.asm (Crystal water EMPTY).
// Gold/Silver Route38 Snubbull differs by version and time; Crystal has
// Dunsparce & Yanma swarms but NO Marill Surf swarm. No false global Invalid.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen2OriginalSwarmStage {
enum class Method : uint8_t {Grass, Surf};
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalRow, UnknownOriginalEvent,
    UnknownOriginalTime, UnknownOriginalMethod, UnknownOriginalMap,
    UnknownOriginalSpecies, UnknownOriginalLevel,
    ForeignGameRowNotEvidence, CandidateTimeContradicted,
    CandidateMethodContradicted, CandidateMapContradicted,
    CandidateSpeciesContradicted, CandidateLevelContradicted,
    SourceSwarmRowCompatibleOriginalWorldUnverified
};
struct Row {uint8_t gameMask; std::string_view map; Method method;
    uint8_t timeGroup,slot; uint16_t species; uint8_t level;};
inline constexpr auto kSwarmRows=std::to_array<Row>({
    {3,"Route35",Method::Grass,0,0,32,12},
    {3,"Route35",Method::Grass,0,1,29,12},
    {3,"Route35",Method::Grass,0,2,193,12},
    {3,"Route35",Method::Grass,0,3,193,14},
    {3,"Route35",Method::Grass,0,4,16,14},
    {3,"Route35",Method::Grass,0,5,132,10},
    {3,"Route35",Method::Grass,0,6,132,10},
    {4,"Route35",Method::Grass,0,0,32,12},
    {4,"Route35",Method::Grass,0,1,29,12},
    {4,"Route35",Method::Grass,0,2,193,12},
    {4,"Route35",Method::Grass,0,3,193,14},
    {4,"Route35",Method::Grass,0,4,16,14},
    {4,"Route35",Method::Grass,0,5,132,10},
    {4,"Route35",Method::Grass,0,6,132,10},
    {3,"DarkCaveVioletEntrance",Method::Grass,0,0,74,3},
    {3,"DarkCaveVioletEntrance",Method::Grass,0,1,206,3},
    {3,"DarkCaveVioletEntrance",Method::Grass,0,2,41,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,0,3,74,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,0,4,206,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,0,5,206,4},
    {3,"DarkCaveVioletEntrance",Method::Grass,0,6,206,4},
    {4,"DarkCaveVioletEntrance",Method::Grass,0,0,74,3},
    {4,"DarkCaveVioletEntrance",Method::Grass,0,1,206,3},
    {4,"DarkCaveVioletEntrance",Method::Grass,0,2,41,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,0,3,74,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,0,4,206,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,0,5,206,4},
    {4,"DarkCaveVioletEntrance",Method::Grass,0,6,206,4},
    {3,"MountMortar1FOutside",Method::Grass,0,0,183,13},
    {3,"MountMortar1FOutside",Method::Grass,0,1,41,15},
    {3,"MountMortar1FOutside",Method::Grass,0,2,66,14},
    {3,"MountMortar1FOutside",Method::Grass,0,3,183,15},
    {3,"MountMortar1FOutside",Method::Grass,0,4,74,14},
    {3,"MountMortar1FOutside",Method::Grass,0,5,19,16},
    {3,"MountMortar1FOutside",Method::Grass,0,6,19,16},
    {1,"Route38",Method::Grass,0,0,209,16},
    {1,"Route38",Method::Grass,0,1,20,16},
    {1,"Route38",Method::Grass,0,2,81,16},
    {1,"Route38",Method::Grass,0,3,83,16},
    {1,"Route38",Method::Grass,0,4,241,13},
    {1,"Route38",Method::Grass,0,5,128,13},
    {1,"Route38",Method::Grass,0,6,128,13},
    {2,"Route38",Method::Grass,0,0,209,16},
    {2,"Route38",Method::Grass,0,1,52,16},
    {2,"Route38",Method::Grass,0,2,81,16},
    {2,"Route38",Method::Grass,0,3,83,16},
    {2,"Route38",Method::Grass,0,4,241,13},
    {2,"Route38",Method::Grass,0,5,128,13},
    {2,"Route38",Method::Grass,0,6,128,13},
    {3,"Route35",Method::Grass,1,0,32,12},
    {3,"Route35",Method::Grass,1,1,29,12},
    {3,"Route35",Method::Grass,1,2,193,12},
    {3,"Route35",Method::Grass,1,3,193,14},
    {3,"Route35",Method::Grass,1,4,16,14},
    {3,"Route35",Method::Grass,1,5,132,10},
    {3,"Route35",Method::Grass,1,6,132,10},
    {4,"Route35",Method::Grass,1,0,32,12},
    {4,"Route35",Method::Grass,1,1,29,12},
    {4,"Route35",Method::Grass,1,2,193,12},
    {4,"Route35",Method::Grass,1,3,193,14},
    {4,"Route35",Method::Grass,1,4,16,14},
    {4,"Route35",Method::Grass,1,5,132,10},
    {4,"Route35",Method::Grass,1,6,132,10},
    {3,"DarkCaveVioletEntrance",Method::Grass,1,0,74,3},
    {3,"DarkCaveVioletEntrance",Method::Grass,1,1,206,3},
    {3,"DarkCaveVioletEntrance",Method::Grass,1,2,41,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,1,3,74,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,1,4,206,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,1,5,206,4},
    {3,"DarkCaveVioletEntrance",Method::Grass,1,6,206,4},
    {4,"DarkCaveVioletEntrance",Method::Grass,1,0,74,3},
    {4,"DarkCaveVioletEntrance",Method::Grass,1,1,206,3},
    {4,"DarkCaveVioletEntrance",Method::Grass,1,2,41,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,1,3,74,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,1,4,206,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,1,5,206,4},
    {4,"DarkCaveVioletEntrance",Method::Grass,1,6,206,4},
    {3,"MountMortar1FOutside",Method::Grass,1,0,183,13},
    {3,"MountMortar1FOutside",Method::Grass,1,1,41,15},
    {3,"MountMortar1FOutside",Method::Grass,1,2,66,14},
    {3,"MountMortar1FOutside",Method::Grass,1,3,183,15},
    {3,"MountMortar1FOutside",Method::Grass,1,4,74,14},
    {3,"MountMortar1FOutside",Method::Grass,1,5,19,16},
    {3,"MountMortar1FOutside",Method::Grass,1,6,19,16},
    {1,"Route38",Method::Grass,1,0,209,16},
    {1,"Route38",Method::Grass,1,1,20,16},
    {1,"Route38",Method::Grass,1,2,81,16},
    {1,"Route38",Method::Grass,1,3,83,16},
    {1,"Route38",Method::Grass,1,4,241,13},
    {1,"Route38",Method::Grass,1,5,128,13},
    {1,"Route38",Method::Grass,1,6,128,13},
    {2,"Route38",Method::Grass,1,0,209,16},
    {2,"Route38",Method::Grass,1,1,52,16},
    {2,"Route38",Method::Grass,1,2,81,16},
    {2,"Route38",Method::Grass,1,3,83,16},
    {2,"Route38",Method::Grass,1,4,241,13},
    {2,"Route38",Method::Grass,1,5,128,13},
    {2,"Route38",Method::Grass,1,6,128,13},
    {3,"Route35",Method::Grass,2,0,32,12},
    {3,"Route35",Method::Grass,2,1,29,12},
    {3,"Route35",Method::Grass,2,2,193,12},
    {3,"Route35",Method::Grass,2,3,193,14},
    {3,"Route35",Method::Grass,2,4,163,14},
    {3,"Route35",Method::Grass,2,5,132,10},
    {3,"Route35",Method::Grass,2,6,132,10},
    {4,"Route35",Method::Grass,2,0,32,12},
    {4,"Route35",Method::Grass,2,1,29,12},
    {4,"Route35",Method::Grass,2,2,193,12},
    {4,"Route35",Method::Grass,2,3,193,14},
    {4,"Route35",Method::Grass,2,4,163,14},
    {4,"Route35",Method::Grass,2,5,132,10},
    {4,"Route35",Method::Grass,2,6,132,10},
    {3,"DarkCaveVioletEntrance",Method::Grass,2,0,74,3},
    {3,"DarkCaveVioletEntrance",Method::Grass,2,1,206,3},
    {3,"DarkCaveVioletEntrance",Method::Grass,2,2,41,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,2,3,74,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,2,4,206,2},
    {3,"DarkCaveVioletEntrance",Method::Grass,2,5,206,4},
    {3,"DarkCaveVioletEntrance",Method::Grass,2,6,206,4},
    {4,"DarkCaveVioletEntrance",Method::Grass,2,0,74,3},
    {4,"DarkCaveVioletEntrance",Method::Grass,2,1,206,3},
    {4,"DarkCaveVioletEntrance",Method::Grass,2,2,41,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,2,3,74,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,2,4,206,2},
    {4,"DarkCaveVioletEntrance",Method::Grass,2,5,206,4},
    {4,"DarkCaveVioletEntrance",Method::Grass,2,6,206,4},
    {3,"MountMortar1FOutside",Method::Grass,2,0,183,13},
    {3,"MountMortar1FOutside",Method::Grass,2,1,41,15},
    {3,"MountMortar1FOutside",Method::Grass,2,2,66,14},
    {3,"MountMortar1FOutside",Method::Grass,2,3,183,15},
    {3,"MountMortar1FOutside",Method::Grass,2,4,74,14},
    {3,"MountMortar1FOutside",Method::Grass,2,5,19,16},
    {3,"MountMortar1FOutside",Method::Grass,2,6,19,16},
    {1,"Route38",Method::Grass,2,0,209,16},
    {1,"Route38",Method::Grass,2,1,20,16},
    {1,"Route38",Method::Grass,2,2,81,16},
    {1,"Route38",Method::Grass,2,3,19,16},
    {1,"Route38",Method::Grass,2,4,241,13},
    {1,"Route38",Method::Grass,2,5,128,13},
    {1,"Route38",Method::Grass,2,6,128,13},
    {2,"Route38",Method::Grass,2,0,209,16},
    {2,"Route38",Method::Grass,2,1,52,16},
    {2,"Route38",Method::Grass,2,2,81,16},
    {2,"Route38",Method::Grass,2,3,52,16},
    {2,"Route38",Method::Grass,2,4,241,13},
    {2,"Route38",Method::Grass,2,5,128,13},
    {2,"Route38",Method::Grass,2,6,128,13},
    {3,"MountMortar1FOutside",Method::Surf,0,0,118,20},
    {3,"MountMortar1FOutside",Method::Surf,0,1,183,20},
    {3,"MountMortar1FOutside",Method::Surf,0,2,119,20},
});
struct Candidate {
    std::string_view exactOriginalGame{},originalMap{};
    std::size_t originalRowIndex=0;
    Method originalMethod=Method::Grass;
    uint8_t originalTimeGroup=0;
    uint16_t originalSpecies=0;
    uint8_t originalLevel=0;
    bool gameAttested=false,rowAttested=false,swarmActiveAtCaptureAttested=false;
    bool timeAttested=false,methodAttested=false,mapAttested=false;
    bool speciesAttested=false,levelAttested=false;
};
constexpr uint8_t gameMask(std::string_view id) noexcept {
    return id=="gold_gbc"?1:id=="silver_gbc"?2:id=="crystal_gbc"?4:0;
}
struct Result {Status status=Status::UnknownOriginalGame; uint16_t species=0;};
constexpr Result evaluate(const Candidate& c) noexcept {
    auto gm=gameMask(c.exactOriginalGame);
    if(!c.gameAttested || !gm)return {};
    if(!c.rowAttested || c.originalRowIndex>=kSwarmRows.size())
        return {Status::UnknownOriginalRow};
    const auto& row=kSwarmRows[c.originalRowIndex];
    if(!(gm&row.gameMask))return {Status::ForeignGameRowNotEvidence,row.species};
    if(!c.swarmActiveAtCaptureAttested)
        return {Status::UnknownOriginalEvent,row.species};
    if(!c.timeAttested || c.originalTimeGroup>2)
        return {Status::UnknownOriginalTime,row.species};
    if(!c.methodAttested || (c.originalMethod!=Method::Grass &&
       c.originalMethod!=Method::Surf))
        return {Status::UnknownOriginalMethod,row.species};
    if(!c.mapAttested || c.originalMap.empty())
        return {Status::UnknownOriginalMap,row.species};
    if(!c.speciesAttested || c.originalSpecies==0 || c.originalSpecies>251)
        return {Status::UnknownOriginalSpecies,row.species};
    if(!c.levelAttested || c.originalLevel==0 || c.originalLevel>100)
        return {Status::UnknownOriginalLevel,row.species};
    if(c.originalTimeGroup!=row.timeGroup && row.method!=Method::Surf)
        return {Status::CandidateTimeContradicted,row.species};
    if(c.originalMethod!=row.method)
        return {Status::CandidateMethodContradicted,row.species};
    if(c.originalMap!=row.map)
        return {Status::CandidateMapContradicted,row.species};
    if(c.originalSpecies!=row.species)
        return {Status::CandidateSpeciesContradicted,row.species};
    if(c.originalLevel!=row.level)
        return {Status::CandidateLevelContradicted,row.species};
    return {Status::SourceSwarmRowCompatibleOriginalWorldUnverified,row.species};
}
} // namespace Legality::Gen2OriginalSwarmStage
