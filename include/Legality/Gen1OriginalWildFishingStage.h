#pragma once
// L1 E01: deliberately BOUNDED and source-pinned original R/B/Y wild and
// R/B Super Rod source-slot evidence. A slot witness is not a PK1 certificate.
// Sources: pret/pokered@af519899719f0754965776faac0e836a3b906e6d
//   data/wild/maps/Route1.asm, ViridianForest.asm, data/wild/super_rod.asm
// pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382
//   data/wild/maps/Route1.asm
// The original 149-slot namespace is preserved. Expanded source-pinned
// physical R/B/Y tables, all three rods and the original wild roll live
// in independent additional namespaces. Source witnesses never prove PK1.
#include "Legality/Gen1PretWildRollStage.h"
#include "Legality/Gen1PretFishingRollStage.h"
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
    CandidateOriginalLevelContradicted, SourceSlotCompatibleRateAndHistoryUnverified,
    UnknownMapApplicabilityForGlobalRod,
    UnknownOriginalFishingInit, CandidateOriginalFishingInitContradicted
};
struct Slot {
    uint8_t gameMask;  // Red 1, Blue 2, Yellow 4
    std::string_view map;
    Method method;
    uint8_t physicalSlot;
    uint16_t species;
    uint8_t level;
};
inline constexpr std::array<Slot, 149> kVerifiedSlots{{
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
    {3, "ViridianCity", Method::SuperRod, 0, 72, 15},
    {3, "ViridianCity", Method::SuperRod, 1, 60, 15},
    {3, "CeruleanCity", Method::SuperRod, 0, 54, 15},
    {3, "CeruleanCity", Method::SuperRod, 1, 118, 15},
    {3, "CeruleanCity", Method::SuperRod, 2, 98, 15},
    {3, "VermilionCity", Method::SuperRod, 0, 98, 15},
    {3, "VermilionCity", Method::SuperRod, 1, 90, 15},
    {3, "CeladonCity", Method::SuperRod, 0, 61, 23},
    {3, "CeladonCity", Method::SuperRod, 1, 79, 15},
    {3, "CinnabarIsland", Method::SuperRod, 0, 120, 15},
    {3, "CinnabarIsland", Method::SuperRod, 1, 116, 15},
    {3, "CinnabarIsland", Method::SuperRod, 2, 90, 15},
    {3, "CinnabarIsland", Method::SuperRod, 3, 118, 15},
    {3, "Route4", Method::SuperRod, 0, 54, 15},
    {3, "Route4", Method::SuperRod, 1, 118, 15},
    {3, "Route4", Method::SuperRod, 2, 98, 15},
    {3, "Route6", Method::SuperRod, 0, 98, 15},
    {3, "Route6", Method::SuperRod, 1, 90, 15},
    {3, "Route10", Method::SuperRod, 0, 61, 23},
    {3, "Route10", Method::SuperRod, 1, 79, 15},
    {3, "Route11", Method::SuperRod, 0, 98, 15},
    {3, "Route11", Method::SuperRod, 1, 90, 15},
    {3, "Route12", Method::SuperRod, 0, 72, 5},
    {3, "Route12", Method::SuperRod, 1, 98, 15},
    {3, "Route12", Method::SuperRod, 2, 118, 15},
    {3, "Route12", Method::SuperRod, 3, 129, 15},
    {3, "Route13", Method::SuperRod, 0, 72, 5},
    {3, "Route13", Method::SuperRod, 1, 98, 15},
    {3, "Route13", Method::SuperRod, 2, 118, 15},
    {3, "Route13", Method::SuperRod, 3, 129, 15},
    {3, "Route17", Method::SuperRod, 0, 72, 5},
    {3, "Route17", Method::SuperRod, 1, 98, 15},
    {3, "Route17", Method::SuperRod, 2, 118, 15},
    {3, "Route17", Method::SuperRod, 3, 129, 15},
    {3, "Route18", Method::SuperRod, 0, 72, 5},
    {3, "Route18", Method::SuperRod, 1, 98, 15},
    {3, "Route18", Method::SuperRod, 2, 118, 15},
    {3, "Route18", Method::SuperRod, 3, 129, 15},
    {3, "Route19", Method::SuperRod, 0, 120, 15},
    {3, "Route19", Method::SuperRod, 1, 116, 15},
    {3, "Route19", Method::SuperRod, 2, 90, 15},
    {3, "Route19", Method::SuperRod, 3, 118, 15},
    {3, "Route20", Method::SuperRod, 0, 120, 15},
    {3, "Route20", Method::SuperRod, 1, 116, 15},
    {3, "Route20", Method::SuperRod, 2, 90, 15},
    {3, "Route20", Method::SuperRod, 3, 118, 15},
    {3, "Route21", Method::SuperRod, 0, 120, 15},
    {3, "Route21", Method::SuperRod, 1, 116, 15},
    {3, "Route21", Method::SuperRod, 2, 90, 15},
    {3, "Route21", Method::SuperRod, 3, 118, 15},
    {3, "Route22", Method::SuperRod, 0, 118, 15},
    {3, "Route22", Method::SuperRod, 1, 60, 15},
    {3, "Route23", Method::SuperRod, 0, 80, 23},
    {3, "Route23", Method::SuperRod, 1, 119, 23},
    {3, "Route23", Method::SuperRod, 2, 99, 23},
    {3, "Route23", Method::SuperRod, 3, 117, 23},
    {3, "Route24", Method::SuperRod, 0, 54, 15},
    {3, "Route24", Method::SuperRod, 1, 118, 15},
    {3, "Route24", Method::SuperRod, 2, 98, 15},
    {3, "Route25", Method::SuperRod, 0, 54, 15},
    {3, "Route25", Method::SuperRod, 1, 118, 15},
    {3, "Route25", Method::SuperRod, 2, 98, 15},
    {3, "CeruleanGym", Method::SuperRod, 0, 54, 15},
    {3, "CeruleanGym", Method::SuperRod, 1, 118, 15},
    {3, "CeruleanGym", Method::SuperRod, 2, 98, 15},
    {3, "VermilionDock", Method::SuperRod, 0, 98, 15},
    {3, "VermilionDock", Method::SuperRod, 1, 90, 15},
    {3, "SeafoamIslandsB3F", Method::SuperRod, 0, 120, 15},
    {3, "SeafoamIslandsB3F", Method::SuperRod, 1, 116, 15},
    {3, "SeafoamIslandsB3F", Method::SuperRod, 2, 90, 15},
    {3, "SeafoamIslandsB3F", Method::SuperRod, 3, 118, 15},
    {3, "SeafoamIslandsB4F", Method::SuperRod, 0, 120, 15},
    {3, "SeafoamIslandsB4F", Method::SuperRod, 1, 116, 15},
    {3, "SeafoamIslandsB4F", Method::SuperRod, 2, 90, 15},
    {3, "SeafoamIslandsB4F", Method::SuperRod, 3, 118, 15},
    {3, "SafariZoneEast", Method::SuperRod, 0, 147, 15},
    {3, "SafariZoneEast", Method::SuperRod, 1, 98, 15},
    {3, "SafariZoneEast", Method::SuperRod, 2, 54, 15},
    {3, "SafariZoneEast", Method::SuperRod, 3, 79, 15},
    {3, "SafariZoneNorth", Method::SuperRod, 0, 147, 15},
    {3, "SafariZoneNorth", Method::SuperRod, 1, 98, 15},
    {3, "SafariZoneNorth", Method::SuperRod, 2, 54, 15},
    {3, "SafariZoneNorth", Method::SuperRod, 3, 79, 15},
    {3, "SafariZoneWest", Method::SuperRod, 0, 147, 15},
    {3, "SafariZoneWest", Method::SuperRod, 1, 98, 15},
    {3, "SafariZoneWest", Method::SuperRod, 2, 54, 15},
    {3, "SafariZoneWest", Method::SuperRod, 3, 79, 15},
    {3, "SafariZoneCenter", Method::SuperRod, 0, 147, 15},
    {3, "SafariZoneCenter", Method::SuperRod, 1, 98, 15},
    {3, "SafariZoneCenter", Method::SuperRod, 2, 54, 15},
    {3, "SafariZoneCenter", Method::SuperRod, 3, 79, 15},
    {3, "CeruleanCave2F", Method::SuperRod, 0, 80, 23},
    {3, "CeruleanCave2F", Method::SuperRod, 1, 119, 23},
    {3, "CeruleanCave2F", Method::SuperRod, 2, 99, 23},
    {3, "CeruleanCave2F", Method::SuperRod, 3, 117, 23},
    {3, "CeruleanCaveB1F", Method::SuperRod, 0, 80, 23},
    {3, "CeruleanCaveB1F", Method::SuperRod, 1, 119, 23},
    {3, "CeruleanCaveB1F", Method::SuperRod, 2, 99, 23},
    {3, "CeruleanCaveB1F", Method::SuperRod, 3, 117, 23},
    {3, "CeruleanCave1F", Method::SuperRod, 0, 80, 23},
    {3, "CeruleanCave1F", Method::SuperRod, 1, 119, 23},
    {3, "CeruleanCave1F", Method::SuperRod, 2, 99, 23},
    {3, "CeruleanCave1F", Method::SuperRod, 3, 117, 23},
}};
// Expanded source-pinned Red/Blue/Yellow archive. kVerifiedSlots and its
// existing 149 indices remain untouched. kPret* is a NEW index namespace.
struct PretSlot {
    uint8_t gameMask;
    std::string_view map;
    Method method;
    uint8_t physicalSlot;
    uint16_t species;
    uint8_t level;
    uint8_t encounterRate;
};
#include "Legality/Gen1PretWildRB0.inc"
#include "Legality/Gen1PretWildRB1.inc"
#include "Legality/Gen1PretWildRB2.inc"
#include "Legality/Gen1PretWildRB3.inc"
#include "Legality/Gen1PretWildYellow0.inc"
#include "Legality/Gen1PretWildYellow1.inc"
#include "Legality/Gen1PretWildYellow2.inc"
#include "Legality/Gen1PretWildYellow3.inc"
#include "Legality/Gen1PretFishing.inc"
inline constexpr std::size_t kPretSlotCount =
    kPretRB0.size()+
    kPretRB1.size()+
    kPretRB2.size()+
    kPretRB3.size()+
    kPretYellow0.size()+
    kPretYellow1.size()+
    kPretYellow2.size()+
    kPretYellow3.size()+
    kPretFishing.size();
constexpr const PretSlot* pretSlot(std::size_t index) noexcept {
    if(index<kPretRB0.size())return &kPretRB0[index];
    index-=kPretRB0.size();
    if(index<kPretRB1.size())return &kPretRB1[index];
    index-=kPretRB1.size();
    if(index<kPretRB2.size())return &kPretRB2[index];
    index-=kPretRB2.size();
    if(index<kPretRB3.size())return &kPretRB3[index];
    index-=kPretRB3.size();
    if(index<kPretYellow0.size())return &kPretYellow0[index];
    index-=kPretYellow0.size();
    if(index<kPretYellow1.size())return &kPretYellow1[index];
    index-=kPretYellow1.size();
    if(index<kPretYellow2.size())return &kPretYellow2[index];
    index-=kPretYellow2.size();
    if(index<kPretYellow3.size())return &kPretYellow3[index];
    index-=kPretYellow3.size();
    if(index<kPretFishing.size())return &kPretFishing[index];
    index-=kPretFishing.size();
    return nullptr;
}
static_assert(kPretSlotCount==1512);
// Original wild grass/surf slots are probability-weighted at 8-bit
// resolution; each map/method table sums to 256. Rod sources instead
// use their own scripts and must NOT inherit these wild-slot odds.
constexpr uint8_t pretWildSlotWeight(std::size_t index) noexcept {
    const PretSlot* source=pretSlot(index);
    if(!source)return 0;
    if((source->method!=Method::Grass&&source->method!=Method::Surf) ||
       source->physicalSlot>=Gen1PretWildRollStage::kSlotWeight.size())
       return 0;
    return Gen1PretWildRollStage::kSlotWeight[source->physicalSlot];
}
static_assert(kVerifiedSlots.size()==149);

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
// Candidate result for the NEW pret source-index namespace; never a
// whole-Pokemon legal/invalid verdict. Global rod entries cannot certify map
// eligibility without an independently sourced fishable-map witness.
constexpr Result evaluatePret(const Candidate& c) noexcept {
    const uint8_t game=gameMask(c.originalGame);
    if(!c.originalGameAttested||!game)return {};
    if(!c.originalSourceSlotAttested)return {Status::UnknownOriginalSourceRow};
    const PretSlot* row=pretSlot(c.originalSourceIndex);
    if(!row)return {Status::UnknownOriginalSourceRow};
    if((game&row->gameMask)==0)
        return {Status::ForeignGameRowNotEvidence,row->species};
    if(!c.originalMapAttested||c.originalMap.empty())
        return {Status::UnknownOriginalMap,row->species};
    if(!c.originalMethodAttested||
       (c.originalMethod!=Method::Grass&&c.originalMethod!=Method::Surf&&
        c.originalMethod!=Method::OldRod&&c.originalMethod!=Method::GoodRod&&
        c.originalMethod!=Method::SuperRod))
        return {Status::UnknownOriginalMethod,row->species};
    if(!c.originalSpeciesAttested||c.originalSpecies==0||c.originalSpecies>151)
        return {Status::UnknownOriginalSpecies,row->species};
    if(!c.originalLevelAttested||c.originalLevel==0||c.originalLevel>100)
        return {Status::UnknownOriginalLevel,row->species};
    if(row->map!="*"&&c.originalMap!=row->map)
        return {Status::CandidateOriginalMapContradicted,row->species};
    if(c.originalMethod!=row->method)
        return {Status::CandidateOriginalMethodContradicted,row->species};
    if(c.originalSpecies!=row->species)
        return {Status::CandidateOriginalSpeciesContradicted,row->species};
    if(c.originalLevel!=row->level)
        return {Status::CandidateOriginalLevelContradicted,row->species};
    if(row->map=="*")
        return {Status::UnknownMapApplicabilityForGlobalRod,row->species};
    return {Status::SourceSlotCompatibleRateAndHistoryUnverified,row->species};
}
// Source: pret/pokered@af519899719f0754965776faac0e836a3b906e6d
// engine/items/item_effects.asm [c4ce13de6fb6c7036e94e20b845dd6c5ade8951e]
// and pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382
// engine/items/item_effects.asm [a6bdf857b15f73609c15f2f22511ea816eb2fd7f].
// FishingInit gates rod use on source physical scene state including facing
// water/shore tile and not being in battle. Source table is insufficient to
// certify a particular original map could be fished at that time.
struct OriginalFishingInit {
    bool originalInitOutcomeIndependentlyAttested=false;
    bool originalFishingInitPassed=false;
};
constexpr Result evaluatePretFishingInit(const Candidate& c,
                                         OriginalFishingInit fishing) noexcept {
    const auto source=evaluatePret(c);
    const PretSlot* row=pretSlot(c.originalSourceIndex);
    if(source.status!=Status::UnknownMapApplicabilityForGlobalRod &&
       source.status!=Status::SourceSlotCompatibleRateAndHistoryUnverified)
        return source;
    if(!row || (row->method!=Method::OldRod && row->method!=Method::GoodRod &&
                row->method!=Method::SuperRod))
        return source; // Ordinary grass and surf are not rod-init stages.
    if(!fishing.originalInitOutcomeIndependentlyAttested)
        return {Status::UnknownOriginalFishingInit,source.sourceSpecies};
    if(!fishing.originalFishingInitPassed)
        return {Status::CandidateOriginalFishingInitContradicted,source.sourceSpecies};
    // Original FishingInit passed, but acquisition-time RNG/rods, event
    // history and original save bytes have not been independently verified.
    return {Status::SourceSlotCompatibleRateAndHistoryUnverified,source.sourceSpecies};
}
// Optional, independently witnessed original R/B/Y RNG-physical-slot join.
// The 1,512 pret source rows retain their historical physical slot ordinals;
// roll/half-block tile context is never fabricated from a detached PK1.
// A source-row contradiction and a wild-roll contradiction are two distinct
// candidate-local stages; neither one authorizes whole-Pokemon Invalid.
struct PretWildRollJoined {
    Status originalSourceStatus=Status::UnknownOriginalSourceRow;
    Gen1PretWildRollStage::Result originalRoll{};
    bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr PretWildRollJoined evaluatePretWildRoll(
    const Candidate& c,Gen1PretWildRollStage::OriginalScene scene) noexcept {
    const auto source=evaluatePret(c);
    PretWildRollJoined result{};
    result.originalSourceStatus=source.status;
    if(source.status!=Status::SourceSlotCompatibleRateAndHistoryUnverified)
        return result;
    const auto* row=pretSlot(c.originalSourceIndex);
    if(!row|| (row->method!=Method::Grass&&row->method!=Method::Surf))
        return result;
    // Each attestation is intentionally separate. The original source row
    // cannot attest an original random value, facing tile, or repel state.
    if(scene.game!=c.originalGame)return result;
    scene.originalExpectedPhysicalSlot=row->physicalSlot;
    scene.originalExpectedMethod=row->method==Method::Grass?
        Gen1PretWildRollStage::Method::Grass:Gen1PretWildRollStage::Method::Surf;
    scene.originalSourceLevel=c.originalLevel;
    scene.originalSourceLevelIndependentlyAttested=c.originalLevelAttested;
    result.originalRoll=Gen1PretWildRollStage::evaluate(scene);
    return result;
}
// Combine the independently selected original physical rod source with
// original FishingInit outcome and the correct per-version RNG algorithm.
// Existing Red/Blue Super Rod rows live in the UNCHANGED kVerifiedSlots
// index namespace; Yellow Super Rod/Old/Good are in the 1512-row pret
// namespace. No connection is inferred between these index namespaces.
struct PretFishingRollJoined {
    Status sourceStatus=Status::UnknownOriginalSourceRow;
    Gen1PretFishingRollStage::Result roll{};
    bool globallyLegalSupported=false,globalInvalidSupported=false;
};
constexpr PretFishingRollJoined evaluatePretFishingRoll(
    const Candidate& c, OriginalFishingInit fishing,
    Gen1PretFishingRollStage::Witness random,
    bool legacyRedBlueSuper=false) noexcept {
    PretFishingRollJoined result{};
    const auto source=legacyRedBlueSuper?evaluate(c):evaluatePret(c);
    result.sourceStatus=source.status;
    if(source.status!=Status::SourceSlotCompatibleRateAndHistoryUnverified &&
       source.status!=Status::UnknownMapApplicabilityForGlobalRod)
        return result;
    Method method=Method::Grass;
    uint8_t originalSlot=0;
    if(legacyRedBlueSuper){
        if(c.originalSourceIndex>=kVerifiedSlots.size())return result;
        const auto& row=kVerifiedSlots[c.originalSourceIndex];
        method=row.method;originalSlot=row.physicalSlot;
        if(method!=Method::SuperRod || gameMask(c.originalGame)==4)return result;
    }else{
        const auto* row=pretSlot(c.originalSourceIndex);
        if(!row)return result;
        method=row->method;originalSlot=row->physicalSlot;
    }
    if(method!=Method::OldRod&&method!=Method::GoodRod&&
       method!=Method::SuperRod)return result;
    if(!fishing.originalInitOutcomeIndependentlyAttested){
        result.sourceStatus=Status::UnknownOriginalFishingInit;return result;
    }
    if(!fishing.originalFishingInitPassed){
        result.sourceStatus=Status::CandidateOriginalFishingInitContradicted;
        return result;
    }
    // A verified original source row cannot fabricate random draws or
    // attestation of game, bite, physical slot or source group size.
    if(random.game!=c.originalGame)return result;
    random.expectedPhysicalSlot=originalSlot;
    random.rod=method==Method::OldRod?Gen1PretFishingRollStage::Rod::Old:
       method==Method::GoodRod?Gen1PretFishingRollStage::Rod::Good:
                               Gen1PretFishingRollStage::Rod::Super;
    result.roll=Gen1PretFishingRollStage::evaluate(random);
    return result;
}
} // namespace Legality::Gen1OriginalWildFishingStage
