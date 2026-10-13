// SPDX-License-Identifier: AGPL-3.0-or-later
// L1: native Red/Blue/Yellow Game Corner ORIGINAL redemption evidence.
// Source pinned to pret/pokered@af519899719f0754965776faac0e836a3b906e6d
// and pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382.
// Source files: data/events/prizes.asm, prize_mon_levels.asm,
// engine/events/prize_menu.asm. Preserve per-vendor physical slot, version,
// original level and coin price; present PK1 proves no original purchase.
#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen1PretAcquisitionStage {
struct PretGameCornerEntry {
 uint8_t gameMask, vendor, physicalSlot;
 uint16_t species;
 uint8_t level;
 uint16_t coinPrice;
};
#include "Legality/Gen1PretGameCorner.inc"
constexpr uint8_t gameMask(std::string_view game) noexcept {
 return game=="red_gb"?1:game=="blue_gb"?2:game=="yellow_gb"?4:0;
}
constexpr const PretGameCornerEntry* offer(uint8_t mask,
                                           uint8_t vendor,
                                           uint8_t slot) noexcept {
 if(mask!=1&&mask!=2&&mask!=4)return nullptr;
 if(vendor>1||slot>2)return nullptr;
 for(const auto& row:kPretGameCornerOffers)
  if(row.gameMask==mask && row.vendor==vendor && row.physicalSlot==slot)
   return &row;
 return nullptr;
}
enum class Status : uint8_t {
 UnknownOriginalGame, UnknownOriginalVendor, UnknownOriginalSlot,
 UnknownOriginalSpecies, UnknownOriginalLevel, UnknownOriginalPurchase,
 UnknownOriginalPayment, CandidateOriginalSpeciesMismatch,
 CandidateOriginalLevelMismatch, CandidateOriginalPriceMismatch,
 ConditionalOriginalRedemptionOnly
};
struct Candidate {
 std::string_view originalGame{};
 uint8_t vendor=255,physicalSlot=255;
 uint16_t originalSpecies=0;
 uint8_t originalLevel=0;
 uint16_t originalCoinsPaid=0;
 bool gameIndependentlyAttested=false;
 bool vendorIndependentlyAttested=false;
 bool sourceSlotIndependentlyAttested=false;
 bool speciesIndependentlyAttested=false;
 bool levelIndependentlyAttested=false;
 bool purchaseIndependentlyAttested=false;
 bool paymentIndependentlyAttested=false;
};
struct Result {
 Status status=Status::UnknownOriginalGame;
 uint16_t sourceSpecies=0;
 uint8_t sourceLevel=0;
 uint16_t sourcePrice=0;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr Result evaluate(const Candidate& c) noexcept {
 const uint8_t game=gameMask(c.originalGame);
 if(!c.gameIndependentlyAttested||!game)return {};
 if(!c.vendorIndependentlyAttested||c.vendor>1)
   return {Status::UnknownOriginalVendor};
 if(!c.sourceSlotIndependentlyAttested||c.physicalSlot>2)
   return {Status::UnknownOriginalSlot};
 const auto* row=offer(game,c.vendor,c.physicalSlot);
 if(!row)return {Status::UnknownOriginalSlot};
 if(!c.speciesIndependentlyAttested||
    c.originalSpecies==0||c.originalSpecies>151)
   return {Status::UnknownOriginalSpecies,row->species,row->level,row->coinPrice};
 if(!c.levelIndependentlyAttested||!c.originalLevel||c.originalLevel>100)
   return {Status::UnknownOriginalLevel,row->species,row->level,row->coinPrice};
 if(c.originalSpecies!=row->species)
   return {Status::CandidateOriginalSpeciesMismatch,row->species,row->level,row->coinPrice};
 if(c.originalLevel!=row->level)
   return {Status::CandidateOriginalLevelMismatch,row->species,row->level,row->coinPrice};
 // Original ROM menus alone do not prove a prize was purchased at all.
 if(!c.purchaseIndependentlyAttested)
   return {Status::UnknownOriginalPurchase,row->species,row->level,row->coinPrice};
 if(!c.paymentIndependentlyAttested)
   return {Status::UnknownOriginalPayment,row->species,row->level,row->coinPrice};
 if(c.originalCoinsPaid!=row->coinPrice)
   return {Status::CandidateOriginalPriceMismatch,row->species,row->level,row->coinPrice};
 // Receipt + row compatibility still cannot certify original OT/DVs,
 // untouched catch-rate, later trades or original trainer provenance.
 return {Status::ConditionalOriginalRedemptionOnly,row->species,row->level,row->coinPrice};
}
static_assert(offer(1,0,0)->species==63 && offer(1,0,0)->level==9);
static_assert(offer(2,0,0)->species==63 && offer(2,0,0)->level==6);
static_assert(offer(4,0,0)->species==63 && offer(4,0,0)->level==15);
static_assert(offer(1,1,2)->coinPrice==9999);
static_assert(offer(2,1,2)->coinPrice==6500);
} // namespace Legality::Gen1PretAcquisitionStage
