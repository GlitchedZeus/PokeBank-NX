#pragma once
// L1 E04: complete ORIGINAL FISHGROUP 0..12 physical slot table from
// pret/pokegold@ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8
// pret/pokecrystal@3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6
// data/wild/fish.asm: all Old/Good/Super Rod slots, day/night time_group
// substitutions and event swarm-group variants. No map/rod ownership
// or original fishing transcript is inferred; an external historical
// map->fishgroup witness is still needed to certify an actual encounter.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen2OriginalFishingStage {
enum class Rod : uint8_t { Old, Good, Super };
enum class DayPhase : uint8_t { Any, Day, Night };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalGroup, UnknownOriginalSlot,
    UnknownOriginalRod, UnknownOriginalDayNight, UnknownOriginalSwarm,
    UnknownOriginalSpecies, UnknownOriginalLevel, CandidateRodContradicted,
    CandidateDayNightContradicted, CandidateSwarmContradicted,
    CandidateSpeciesContradicted, CandidateLevelContradicted,
    SourceSlotCompatibleWorldAndRngUnverified
};
struct Slot {
    uint8_t fishGroup;
    Rod rod;
    uint8_t physicalSlot;
    DayPhase phase;
    uint16_t species;
    uint8_t level;
};
inline constexpr auto kOriginalSlots=std::to_array<Slot>({
    // fishgroup #0 (original ASM order)
    {0,Rod::Old,0,DayPhase::Any,129,10},
    {0,Rod::Old,1,DayPhase::Any,129,10},
    {0,Rod::Old,2,DayPhase::Any,98,10},
    {0,Rod::Good,0,DayPhase::Any,129,20},
    {0,Rod::Good,1,DayPhase::Any,98,20},
    {0,Rod::Good,2,DayPhase::Any,98,20},
    {0,Rod::Good,3,DayPhase::Day,222,20},
    {0,Rod::Good,3,DayPhase::Night,121,20},
    {0,Rod::Super,0,DayPhase::Any,98,40},
    {0,Rod::Super,1,DayPhase::Day,222,40},
    {0,Rod::Super,1,DayPhase::Night,121,40},
    {0,Rod::Super,2,DayPhase::Any,98,40},
    {0,Rod::Super,3,DayPhase::Any,99,40},
    // fishgroup #1 (original ASM order)
    {1,Rod::Old,0,DayPhase::Any,129,10},
    {1,Rod::Old,1,DayPhase::Any,129,10},
    {1,Rod::Old,2,DayPhase::Any,72,10},
    {1,Rod::Good,0,DayPhase::Any,129,20},
    {1,Rod::Good,1,DayPhase::Any,72,20},
    {1,Rod::Good,2,DayPhase::Any,170,20},
    {1,Rod::Good,3,DayPhase::Any,90,20},
    {1,Rod::Super,0,DayPhase::Any,170,40},
    {1,Rod::Super,1,DayPhase::Any,90,40},
    {1,Rod::Super,2,DayPhase::Any,73,40},
    {1,Rod::Super,3,DayPhase::Any,171,40},
    // fishgroup #2 (original ASM order)
    {2,Rod::Old,0,DayPhase::Any,129,10},
    {2,Rod::Old,1,DayPhase::Any,129,10},
    {2,Rod::Old,2,DayPhase::Any,118,10},
    {2,Rod::Good,0,DayPhase::Any,129,20},
    {2,Rod::Good,1,DayPhase::Any,118,20},
    {2,Rod::Good,2,DayPhase::Any,118,20},
    {2,Rod::Good,3,DayPhase::Any,118,20},
    {2,Rod::Super,0,DayPhase::Any,118,40},
    {2,Rod::Super,1,DayPhase::Any,118,40},
    {2,Rod::Super,2,DayPhase::Any,129,40},
    {2,Rod::Super,3,DayPhase::Any,119,40},
    // fishgroup #3 (original ASM order)
    {3,Rod::Old,0,DayPhase::Any,129,10},
    {3,Rod::Old,1,DayPhase::Any,129,10},
    {3,Rod::Old,2,DayPhase::Any,60,10},
    {3,Rod::Good,0,DayPhase::Any,129,20},
    {3,Rod::Good,1,DayPhase::Any,60,20},
    {3,Rod::Good,2,DayPhase::Any,60,20},
    {3,Rod::Good,3,DayPhase::Any,60,20},
    {3,Rod::Super,0,DayPhase::Any,60,40},
    {3,Rod::Super,1,DayPhase::Any,60,40},
    {3,Rod::Super,2,DayPhase::Any,129,40},
    {3,Rod::Super,3,DayPhase::Any,60,40},
    // fishgroup #4 (original ASM order)
    {4,Rod::Old,0,DayPhase::Any,129,10},
    {4,Rod::Old,1,DayPhase::Any,129,10},
    {4,Rod::Old,2,DayPhase::Any,129,10},
    {4,Rod::Good,0,DayPhase::Any,129,20},
    {4,Rod::Good,1,DayPhase::Any,129,20},
    {4,Rod::Good,2,DayPhase::Any,129,20},
    {4,Rod::Good,3,DayPhase::Any,147,20},
    {4,Rod::Super,0,DayPhase::Any,129,40},
    {4,Rod::Super,1,DayPhase::Any,147,40},
    {4,Rod::Super,2,DayPhase::Any,129,40},
    {4,Rod::Super,3,DayPhase::Any,148,40},
    // fishgroup #5 (original ASM order)
    {5,Rod::Old,0,DayPhase::Any,129,5},
    {5,Rod::Old,1,DayPhase::Any,129,5},
    {5,Rod::Old,2,DayPhase::Any,211,5},
    {5,Rod::Good,0,DayPhase::Any,129,20},
    {5,Rod::Good,1,DayPhase::Any,211,20},
    {5,Rod::Good,2,DayPhase::Any,211,20},
    {5,Rod::Good,3,DayPhase::Any,211,20},
    {5,Rod::Super,0,DayPhase::Any,211,40},
    {5,Rod::Super,1,DayPhase::Any,211,40},
    {5,Rod::Super,2,DayPhase::Any,211,40},
    {5,Rod::Super,3,DayPhase::Any,211,40},
    // fishgroup #6 (original ASM order)
    {6,Rod::Old,0,DayPhase::Any,129,10},
    {6,Rod::Old,1,DayPhase::Any,129,10},
    {6,Rod::Old,2,DayPhase::Any,223,10},
    {6,Rod::Good,0,DayPhase::Any,129,20},
    {6,Rod::Good,1,DayPhase::Any,223,20},
    {6,Rod::Good,2,DayPhase::Any,223,20},
    {6,Rod::Good,3,DayPhase::Any,223,20},
    {6,Rod::Super,0,DayPhase::Any,223,40},
    {6,Rod::Super,1,DayPhase::Any,223,40},
    {6,Rod::Super,2,DayPhase::Any,223,40},
    {6,Rod::Super,3,DayPhase::Any,223,40},
    // fishgroup #7 (original ASM order)
    {7,Rod::Old,0,DayPhase::Any,129,10},
    {7,Rod::Old,1,DayPhase::Any,129,10},
    {7,Rod::Old,2,DayPhase::Any,129,10},
    {7,Rod::Good,0,DayPhase::Any,129,20},
    {7,Rod::Good,1,DayPhase::Any,129,20},
    {7,Rod::Good,2,DayPhase::Any,129,20},
    {7,Rod::Good,3,DayPhase::Any,130,20},
    {7,Rod::Super,0,DayPhase::Any,129,40},
    {7,Rod::Super,1,DayPhase::Any,130,40},
    {7,Rod::Super,2,DayPhase::Any,129,40},
    {7,Rod::Super,3,DayPhase::Any,129,40},
    // fishgroup #8 (original ASM order)
    {8,Rod::Old,0,DayPhase::Any,129,10},
    {8,Rod::Old,1,DayPhase::Any,129,10},
    {8,Rod::Old,2,DayPhase::Any,129,10},
    {8,Rod::Good,0,DayPhase::Any,129,10},
    {8,Rod::Good,1,DayPhase::Any,129,10},
    {8,Rod::Good,2,DayPhase::Any,129,10},
    {8,Rod::Good,3,DayPhase::Any,147,10},
    {8,Rod::Super,0,DayPhase::Any,129,10},
    {8,Rod::Super,1,DayPhase::Any,147,10},
    {8,Rod::Super,2,DayPhase::Any,129,10},
    {8,Rod::Super,3,DayPhase::Any,148,10},
    // fishgroup #9 (original ASM order)
    {9,Rod::Old,0,DayPhase::Any,129,10},
    {9,Rod::Old,1,DayPhase::Any,129,10},
    {9,Rod::Old,2,DayPhase::Any,98,10},
    {9,Rod::Good,0,DayPhase::Any,129,20},
    {9,Rod::Good,1,DayPhase::Any,98,20},
    {9,Rod::Good,2,DayPhase::Any,98,20},
    {9,Rod::Good,3,DayPhase::Any,116,20},
    {9,Rod::Super,0,DayPhase::Any,98,40},
    {9,Rod::Super,1,DayPhase::Any,116,40},
    {9,Rod::Super,2,DayPhase::Any,99,40},
    {9,Rod::Super,3,DayPhase::Any,117,40},
    // fishgroup #10 (original ASM order)
    {10,Rod::Old,0,DayPhase::Any,129,10},
    {10,Rod::Old,1,DayPhase::Any,129,10},
    {10,Rod::Old,2,DayPhase::Any,72,10},
    {10,Rod::Good,0,DayPhase::Any,129,20},
    {10,Rod::Good,1,DayPhase::Any,72,20},
    {10,Rod::Good,2,DayPhase::Any,72,20},
    {10,Rod::Good,3,DayPhase::Any,72,20},
    {10,Rod::Super,0,DayPhase::Any,72,40},
    {10,Rod::Super,1,DayPhase::Any,72,40},
    {10,Rod::Super,2,DayPhase::Any,129,40},
    {10,Rod::Super,3,DayPhase::Any,211,40},
    // fishgroup #11 (original ASM order)
    {11,Rod::Old,0,DayPhase::Any,129,10},
    {11,Rod::Old,1,DayPhase::Any,129,10},
    {11,Rod::Old,2,DayPhase::Any,60,10},
    {11,Rod::Good,0,DayPhase::Any,129,20},
    {11,Rod::Good,1,DayPhase::Any,60,20},
    {11,Rod::Good,2,DayPhase::Any,60,20},
    {11,Rod::Good,3,DayPhase::Any,60,20},
    {11,Rod::Super,0,DayPhase::Any,60,40},
    {11,Rod::Super,1,DayPhase::Any,60,40},
    {11,Rod::Super,2,DayPhase::Any,129,40},
    {11,Rod::Super,3,DayPhase::Any,223,40},
    // fishgroup #12 (original ASM order)
    {12,Rod::Old,0,DayPhase::Any,129,10},
    {12,Rod::Old,1,DayPhase::Any,129,10},
    {12,Rod::Old,2,DayPhase::Any,72,10},
    {12,Rod::Good,0,DayPhase::Any,129,20},
    {12,Rod::Good,1,DayPhase::Any,72,20},
    {12,Rod::Good,2,DayPhase::Any,72,20},
    {12,Rod::Good,3,DayPhase::Any,72,20},
    {12,Rod::Super,0,DayPhase::Any,72,40},
    {12,Rod::Super,1,DayPhase::Any,72,40},
    {12,Rod::Super,2,DayPhase::Any,129,40},
    {12,Rod::Super,3,DayPhase::Any,211,40},
});
struct Candidate {
    std::string_view exactOriginalGame{};
    std::size_t originalRowIndex=0;
    uint8_t originalFishGroup=0;
    Rod originalRod=Rod::Old;
    bool originalNight=false,originalSwarmActive=false;
    uint16_t originalSpecies=0;
    uint8_t originalLevel=0;
    bool gameAttested=false,rowAttested=false,groupAttested=false;
    bool rodAttested=false,dayNightAttested=false,swarmStateAttested=false;
    bool speciesAttested=false,levelAttested=false;
};
struct Result {Status status=Status::UnknownOriginalGame; uint16_t species=0;};
constexpr Result evaluate(const Candidate& c) noexcept {
    if(!c.gameAttested ||
       (c.exactOriginalGame!="gold_gbc" &&
        c.exactOriginalGame!="silver_gbc" &&
        c.exactOriginalGame!="crystal_gbc"))return {};
    if(!c.rowAttested || c.originalRowIndex>=kOriginalSlots.size())
        return {Status::UnknownOriginalSlot};
    const auto& row=kOriginalSlots[c.originalRowIndex];
    if(!c.groupAttested || c.originalFishGroup>=13u)
        return {Status::UnknownOriginalGroup,row.species};
    if(!c.rodAttested || (c.originalRod!=Rod::Old &&
       c.originalRod!=Rod::Good && c.originalRod!=Rod::Super))
        return {Status::UnknownOriginalRod,row.species};
    if(row.phase!=DayPhase::Any && !c.dayNightAttested)
        return {Status::UnknownOriginalDayNight,row.species};
    // Group 5 Qwilfish / 6 Remoraid are swarm-activated; group 12 is
    // the original no-swarm alternative. Current swarm state says nothing
    // about this acquisition, which requires a separate witness.
    const bool swarmSensitive=row.fishGroup==5u || row.fishGroup==6u ||
                              row.fishGroup==12u;
    if(swarmSensitive && !c.swarmStateAttested)
        return {Status::UnknownOriginalSwarm,row.species};
    if(!c.speciesAttested || !c.originalSpecies || c.originalSpecies>251)
        return {Status::UnknownOriginalSpecies,row.species};
    if(!c.levelAttested || !c.originalLevel || c.originalLevel>100)
        return {Status::UnknownOriginalLevel,row.species};
    if(c.originalFishGroup!=row.fishGroup)
        return {Status::UnknownOriginalGroup,row.species};
    if(c.originalRod!=row.rod)
        return {Status::CandidateRodContradicted,row.species};
    if(row.phase!=DayPhase::Any &&
       ((row.phase==DayPhase::Night)!=c.originalNight))
        return {Status::CandidateDayNightContradicted,row.species};
    if(swarmSensitive && (c.originalSwarmActive!=(row.fishGroup!=12u)))
        return {Status::CandidateSwarmContradicted,row.species};
    if(c.originalSpecies!=row.species)
        return {Status::CandidateSpeciesContradicted,row.species};
    if(c.originalLevel!=row.level)
        return {Status::CandidateLevelContradicted,row.species};
    return {Status::SourceSlotCompatibleWorldAndRngUnverified,row.species};
}
} // namespace Legality::Gen2OriginalFishingStage