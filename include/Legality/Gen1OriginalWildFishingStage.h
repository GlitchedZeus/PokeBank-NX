#pragma once
// L1 E01: deliberately BOUNDED and source-pinned original R/B/Y wild and
// R/B Super Rod source-slot evidence. A slot witness is not a PK1 certificate.
// Sources: pret/pokered@af519899719f0754965776faac0e836a3b906e6d
//   data/wild/maps/Route1.asm, ViridianForest.asm, data/wild/super_rod.asm
// pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382
//   data/wild/maps/Route1.asm
// No Yellow fishing, other maps, Good/Old Rod, or catch RNG is asserted.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen1OriginalWildFishingStage {
enum class Method : uint8_t { Grass, Surf, OldRod, GoodRod, SuperRod };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalSourceRow, UnknownOriginalMap,
    UnknownOriginalMethod, UnknownOriginalSpecies, UnknownOriginalLevel,
    ForeignGameRowNotEvidence, CandidateOriginalMapContradicted,
    CandidateOriginalMethodContradicted, CandidateOriginalSpeciesContradicted,
    CandidateOriginalLevelContradicted, SourceSlotCompatibleRateAndHistoryUnverified
};
struct Slot {
    uint8_t gameMask;  // Red 1, Blue 2, Yellow 4
    std::string_view map;
    Method method;
    uint8_t physicalSlot;
    uint16_t species;
    uint8_t level;
};
inline constexpr std::array<Slot, 46> kVerifiedSlots{{
    {3, "Route1", Method::Grass, 0, 16, 3},
    {3, "Route1", Method::Grass, 1, 19, 3},
    {3, "Route1", Method::Grass, 2, 19, 3},
    {3, "Route1", Method::Grass, 3, 19, 2},
    {3, "Route1", Method::Grass, 4, 16, 2},
    {3, "Route1", Method::Grass, 5, 16, 3},
    {3, "Route1", Method::Grass, 6, 16, 3},
    {3, "Route1", Method::Grass, 7, 19, 4},
    {3, "Route1", Method::Grass, 8, 16, 4},
    {3, "Route1", Method::Grass, 9, 16, 5},
    {4, "Route1", Method::Grass, 0, 16, 3},
    {4, "Route1", Method::Grass, 1, 16, 4},
    {4, "Route1", Method::Grass, 2, 19, 2},
    {4, "Route1", Method::Grass, 3, 19, 3},
    {4, "Route1", Method::Grass, 4, 16, 2},
    {4, "Route1", Method::Grass, 5, 16, 3},
    {4, "Route1", Method::Grass, 6, 16, 5},
    {4, "Route1", Method::Grass, 7, 19, 4},
    {4, "Route1", Method::Grass, 8, 16, 6},
    {4, "Route1", Method::Grass, 9, 16, 7},
    {1, "ViridianForest", Method::Grass, 0, 13, 4},
    {1, "ViridianForest", Method::Grass, 1, 14, 5},
    {1, "ViridianForest", Method::Grass, 2, 13, 3},
    {1, "ViridianForest", Method::Grass, 3, 13, 5},
    {1, "ViridianForest", Method::Grass, 4, 14, 4},
    {1, "ViridianForest", Method::Grass, 5, 14, 6},
    {1, "ViridianForest", Method::Grass, 6, 11, 4},
    {1, "ViridianForest", Method::Grass, 7, 10, 3},
    {1, "ViridianForest", Method::Grass, 8, 25, 3},
    {1, "ViridianForest", Method::Grass, 9, 25, 5},
    {2, "ViridianForest", Method::Grass, 0, 10, 4},
    {2, "ViridianForest", Method::Grass, 1, 11, 5},
    {2, "ViridianForest", Method::Grass, 2, 10, 3},
    {2, "ViridianForest", Method::Grass, 3, 10, 5},
    {2, "ViridianForest", Method::Grass, 4, 11, 4},
    {2, "ViridianForest", Method::Grass, 5, 11, 6},
    {2, "ViridianForest", Method::Grass, 6, 14, 4},
    {2, "ViridianForest", Method::Grass, 7, 13, 3},
    {2, "ViridianForest", Method::Grass, 8, 25, 3},
    {2, "ViridianForest", Method::Grass, 9, 25, 5},
    {3, "PalletTown", Method::SuperRod, 0, 72, 15},
    {3, "PalletTown", Method::SuperRod, 1, 60, 15},
    {3, "FuchsiaCity", Method::SuperRod, 0, 119, 23},
    {3, "FuchsiaCity", Method::SuperRod, 1, 98, 15},
    {3, "FuchsiaCity", Method::SuperRod, 2, 118, 15},
    {3, "FuchsiaCity", Method::SuperRod, 3, 129, 15},
}};
struct Candidate {
    std::string_view originalGame{};
    std::size_t originalSourceIndex=0;
    std::string_view originalMap{};
    Method originalMethod=Method::Grass;
    uint16_t originalSpecies=0;
    uint8_t originalLevel=0;
    // These are separate surviving acquisition witnesses, NOT deductions
    // from detached PK1 data or a modern save's containing game.
    bool originalGameAttested=false, originalSourceSlotAttested=false;
    bool originalMapAttested=false, originalMethodAttested=false;
    bool originalSpeciesAttested=false, originalLevelAttested=false;
};
struct Result { Status status=Status::UnknownOriginalGame; uint16_t sourceSpecies=0; };
constexpr uint8_t gameMask(std::string_view game) noexcept {
    return game=="red_gb"?1:game=="blue_gb"?2:game=="yellow_gb"?4:0;
}
constexpr Result evaluate(const Candidate& c) noexcept {
    const uint8_t game=gameMask(c.originalGame);
    if(!c.originalGameAttested || game==0)return {};
    if(!c.originalSourceSlotAttested || c.originalSourceIndex>=kVerifiedSlots.size())
        return {Status::UnknownOriginalSourceRow};
    const Slot& row=kVerifiedSlots[c.originalSourceIndex];
    if((game&row.gameMask)==0)return {Status::ForeignGameRowNotEvidence,row.species};
    if(!c.originalMapAttested || c.originalMap.empty())
        return {Status::UnknownOriginalMap,row.species};
    if(!c.originalMethodAttested ||
       (c.originalMethod!=Method::Grass && c.originalMethod!=Method::Surf &&
        c.originalMethod!=Method::OldRod && c.originalMethod!=Method::GoodRod &&
        c.originalMethod!=Method::SuperRod))
        return {Status::UnknownOriginalMethod,row.species};
    if(!c.originalSpeciesAttested || c.originalSpecies==0 || c.originalSpecies>151)
        return {Status::UnknownOriginalSpecies,row.species};
    if(!c.originalLevelAttested || c.originalLevel==0 || c.originalLevel>100)
        return {Status::UnknownOriginalLevel,row.species};
    // Local contradictions apply ONLY to this independently selected slot.
    // Competing slots, evolving, traded, gift and future-gen histories remain.
    if(c.originalMap!=row.map)
        return {Status::CandidateOriginalMapContradicted,row.species};
    if(c.originalMethod!=row.method)
        return {Status::CandidateOriginalMethodContradicted,row.species};
    if(c.originalSpecies!=row.species)
        return {Status::CandidateOriginalSpeciesContradicted,row.species};
    if(c.originalLevel!=row.level)
        return {Status::CandidateOriginalLevelContradicted,row.species};
    return {Status::SourceSlotCompatibleRateAndHistoryUnverified,row.species};
}
} // namespace Legality::Gen1OriginalWildFishingStage
