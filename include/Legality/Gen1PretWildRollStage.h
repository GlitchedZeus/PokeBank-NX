// SPDX-License-Identifier: AGPL-3.0-or-later
// Read-only original Gen I Red/Blue/Yellow grass/water slot trigger stage.
// Exact pret disassembly source pins:
// pret/pokered@af519899719f0754965776faac0e836a3b906e6d:
// data/wild/probabilities.asm 6896d93704230993297625f23837631b7da4adf0
// engine/battle/wild_encounters.asm b5cf9f518e9b03ce6122c1c8eea2193d6f500be0
// pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382:
// data/wild/probabilities.asm 6896d93704230993297625f23837631b7da4adf0
// engine/battle/wild_encounters.asm 9fe0c96c16ee897608709eeb6ce006058af4e6f8
// The RNG values are *independently witnessed original* hRandomAdd/Sub;
// a present-day PK1 cannot supply these. No global Legal/Invalid results.
#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen1PretWildRollStage {
enum class Game : uint8_t { Unknown, Red, Blue, Yellow };
enum class Method : uint8_t { Grass, Surf };
enum class Status : uint8_t {
 UnknownOriginalGame,UnknownOriginalTileState,UnknownOriginalRandom,
 UnknownOriginalEncounterGate,UnknownOriginalRepelContext,
 CandidateLocalNoEncounter, CandidateLocalSlotMismatch,
 CandidateLocalMethodMismatch, CandidateLocalRepelBlocked,
 ConditionalOriginalSlotRollOnly
};
inline constexpr std::array<uint8_t,10> kSlotWeight{{
 51,51,39,25,25,25,13,13,11,3
}};
constexpr unsigned sumWeights() noexcept {
 unsigned s=0;for(auto w:kSlotWeight)s+=w;return s;
}
static_assert(sumWeights()==256);
constexpr uint8_t slot(uint8_t roll) noexcept {
 uint16_t upper=0;
 for(uint8_t i=0;i<10;++i) {
   upper=static_cast<uint16_t>(upper+kSlotWeight[i]);
   if(roll<upper)return i;
 }
 return 9; // Unreachable (sum=256), total constexpr.
}
constexpr Game gameFromId(std::string_view s) noexcept {
 return s=="red_gb"?Game::Red:s=="blue_gb"?Game::Blue:
        s=="yellow_gb"?Game::Yellow:Game::Unknown;
}
struct OriginalScene {
 std::string_view game{};
 uint8_t leftBottomTile=0,rightBottomTile=0,grassTile=0;
 uint8_t grassRate=0,waterRate=0,originalRandomAdd=0,originalRandomSub=0;
 uint8_t originalSourceLevel=0,originalLeadPartyLevel=0;
 uint8_t originalExpectedPhysicalSlot=0;
 Method originalExpectedMethod=Method::Grass;
 bool gameIndependentlyAttested=false,tileStateIndependentlyAttested=false;
 bool movementAndMapGatesIndependentlyAttested=false;
 bool movementAndMapAllowEncounter=false;
 bool originalRandomIndependentlyAttested=false;
 bool sourceSlotIndependentlyAttested=false;
 bool originalRepelStateIndependentlyAttested=false;
 bool repelActive=false;
 bool originalLeadPartyLevelIndependentlyAttested=false;
 bool originalSourceLevelIndependentlyAttested=false;
 bool indoorMap=false,forestTileset=false;
};
struct Result {
 Status status=Status::UnknownOriginalGame;
 uint8_t physicalSlot=0;
 Method method=Method::Grass;
 uint8_t threshold=0;
 bool globallyLegalSupported=false,globalInvalidSupported=false;
};
constexpr Result evaluate(const OriginalScene& c) noexcept {
 const Game game=gameFromId(c.game);
 if(!c.gameIndependentlyAttested||game==Game::Unknown)return {};
 if(!c.tileStateIndependentlyAttested)
    return {Status::UnknownOriginalTileState};
 if(!c.movementAndMapGatesIndependentlyAttested)
    return {Status::UnknownOriginalEncounterGate};
 if(!c.movementAndMapAllowEncounter)
    return {Status::CandidateLocalNoEncounter};
 if(!c.originalRandomIndependentlyAttested)
    return {Status::UnknownOriginalRandom};
 // R/B trigger uses bottom RIGHT tile (9,9); Yellow changed it to
 // bottom LEFT tile (8,9), preventing R/B left-shore behavior.
 const uint8_t trigger=(game==Game::Yellow?
                         c.leftBottomTile:c.rightBottomTile);
 uint8_t threshold=0;
 if(trigger==c.grassTile)threshold=c.grassRate;
 else if(trigger==0x14)threshold=c.waterRate;
 else if(c.indoorMap&&!c.forestTileset)threshold=c.grassRate;
 if(threshold==0||c.originalRandomAdd>=threshold)
    return {Status::CandidateLocalNoEncounter,0,Method::Grass,threshold};
 // The original wild method is *always* selected from the bottom LEFT.
 // On Red/Blue left-shore, water can trigger but grass slots are loaded.
 const Method method=c.leftBottomTile==0x14?Method::Surf:Method::Grass;
 const uint8_t selected=slot(c.originalRandomSub);
 if(!c.sourceSlotIndependentlyAttested)
    return {Status::UnknownOriginalEncounterGate,selected,method,threshold};
 if(c.originalExpectedMethod!=method)
    return {Status::CandidateLocalMethodMismatch,selected,method,threshold};
 if(c.originalExpectedPhysicalSlot!=selected)
    return {Status::CandidateLocalSlotMismatch,selected,method,threshold};
 if(!c.originalRepelStateIndependentlyAttested)
    return {Status::UnknownOriginalRepelContext,selected,method,threshold};
 if(c.repelActive){
    if(!c.originalLeadPartyLevelIndependentlyAttested||
       !c.originalSourceLevelIndependentlyAttested)
       return {Status::UnknownOriginalRepelContext,selected,method,threshold};
    if(c.originalSourceLevel<c.originalLeadPartyLevel)
       return {Status::CandidateLocalRepelBlocked,selected,method,threshold};
 }
 return {Status::ConditionalOriginalSlotRollOnly,selected,method,threshold};
}
static_assert(slot(0)==0&&slot(50)==0&&slot(51)==1);
static_assert(slot(255)==9&&slot(253)==9);
} // namespace Legality::Gen1PretWildRollStage
