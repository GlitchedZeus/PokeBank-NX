// SPDX-License-Identifier: AGPL-3.0-or-later
// Read-only original Red/Blue/Yellow fishing RNG stage. ORIGINAL first-party
// pret sources, not modern-gen encounter assumptions:
// pret/pokered@af519899719f0754965776faac0e836a3b906e6d
// engine/items/item_effects.asm blob c4ce13de6fb6c7036e94e20b845dd6c5ade8951e
//   ReadSuperRodData and ItemUseGoodRod RandomLoop: bit0=no bite,
//   low two bits after right shift choose a group entry, rejection rerolls.
// data/wild/super_rod.asm blob 20a52a6c6238685704dd1b0e9957a38be67579b9
// pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382
// engine/items/super_rod.asm blob db82f5fb869799e8cf1f060121538264e578b766
//   4-slot thresholds 0x66,0xb2,0xe5 then separate bite/random bit0.
// engine/items/item_effects.asm blob a6bdf857b15f73609c15f2f22511ea816eb2fd7f
// Both Good Rod algorithms have the same redraw/rejection semantics.
// A source-compatible original roll is NOT a specimen-wide Legal verdict.
#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen1PretFishingRollStage {
enum class Rod : uint8_t { Old, Good, Super };
enum class Status : uint8_t {
 UnknownOriginalGame, UnknownOriginalGroup, UnknownOriginalRandomHistory,
 UnknownRerollLimit, UnknownOriginalSlotWitness, UnknownOriginalBiteWitness,
 CandidateLocalNoBite, CandidateLocalSlotMismatch,
 CandidateLocalBiteMismatch, CandidateLocalWrongRod,
 ConditionalOriginalRodRollOnly
};
struct Witness {
 std::string_view game{};
 Rod rod=Rod::Super;
 uint8_t originalGroupSize=0; // RB Super: 2/3/4, Good: 2, Yellow Super: 4
 uint8_t expectedPhysicalSlot=0;
 bool expectedBite=true;
 bool gameAttested=false, groupAttested=false;
 bool originalDrawsAttested=false, sourceSlotAttested=false;
 bool originalBiteAttested=false;
 // Original sequential Random() results, NOT values reconstructed from
 // the present PK1. No invented draws after the bounded source evidence.
 std::array<uint8_t,16> originalDraws{};
 uint8_t drawCount=0;
};
struct Result {
 Status status=Status::UnknownOriginalGame;
 uint8_t physicalSlot=0;
 bool bite=false;
 uint8_t drawsConsumed=0;
 bool globallyLegalSupported=false,globalInvalidSupported=false;
};
constexpr uint8_t yellowSuperSlot(uint8_t value) noexcept {
 return value<0x66?0:value<0xb2?1:value<0xe5?2:3;
}
constexpr Result evaluate(const Witness& w) noexcept {
 const bool red=w.game=="red_gb",blue=w.game=="blue_gb";
 const bool yellow=w.game=="yellow_gb";
 if(!w.gameAttested||!(red||blue||yellow))return {};
 if(w.rod!=Rod::Old&&w.rod!=Rod::Good&&w.rod!=Rod::Super)
    return {Status::CandidateLocalWrongRod};
 if(!w.sourceSlotAttested)return {Status::UnknownOriginalSlotWitness};
 if(!w.originalBiteAttested)return {Status::UnknownOriginalBiteWitness};
 if(w.rod==Rod::Old) {
   // Always Magikarp L5 on success of FishingInit, no Random() call.
   if(!w.expectedBite)return {Status::CandidateLocalBiteMismatch};
   if(w.expectedPhysicalSlot!=0)return {Status::CandidateLocalSlotMismatch};
   return {Status::ConditionalOriginalRodRollOnly,0,true,0};
 }
 if(!w.groupAttested)return {Status::UnknownOriginalGroup};
 const bool yellowSuper=yellow&&w.rod==Rod::Super;
 if((yellowSuper&&w.originalGroupSize!=4) ||
    (!yellowSuper&&w.rod==Rod::Good&&w.originalGroupSize!=2) ||
    (!yellowSuper&&w.rod==Rod::Super&&
        (w.originalGroupSize<2||w.originalGroupSize>4)))
   return {Status::UnknownOriginalGroup};
 if(!w.originalDrawsAttested||w.drawCount==0||
    w.drawCount>w.originalDraws.size())
   return {Status::UnknownOriginalRandomHistory};
 uint8_t cursor=0,selected=0;
 bool bite=false;
 if(yellowSuper) {
   selected=yellowSuperSlot(w.originalDraws[cursor++]);
   // In Yellow, the second Random() independently controls bite, not
   // the physical slot selected by the first Random().
   if(cursor>=w.drawCount)return {Status::UnknownOriginalRandomHistory};
   bite=(w.originalDraws[cursor++]&1u)!=0;
 } else {
   // Red/Blue Super and all R/B/Y Good Rod:
   // srl a => bit0 carry: no bite if set. Otherwise use
   // (original byte >> 1)&3, reroll on value >= source group count.
   bool resolved=false;
   while(cursor<w.drawCount){
     const uint8_t draw=w.originalDraws[cursor++];
     if((draw&1u)!=0){bite=false;resolved=true;break;}
     selected=static_cast<uint8_t>((draw>>1u)&3u);
     if(selected>=w.originalGroupSize)continue;
     bite=true;resolved=true;break;
   }
   if(!resolved)return {Status::UnknownRerollLimit,0,false,cursor};
 }
 if(bite!=w.expectedBite)
    return {Status::CandidateLocalBiteMismatch,selected,bite,cursor};
 if(!bite)return {Status::CandidateLocalNoBite,0,false,cursor};
 if(selected!=w.expectedPhysicalSlot)
    return {Status::CandidateLocalSlotMismatch,selected,true,cursor};
 return {Status::ConditionalOriginalRodRollOnly,selected,true,cursor};
}
static_assert(yellowSuperSlot(0)==0 && yellowSuperSlot(101)==0);
static_assert(yellowSuperSlot(102)==1 && yellowSuperSlot(177)==1);
static_assert(yellowSuperSlot(178)==2 && yellowSuperSlot(228)==2);
static_assert(yellowSuperSlot(229)==3 && yellowSuperSlot(255)==3);
} // namespace Legality::Gen1PretFishingRollStage
