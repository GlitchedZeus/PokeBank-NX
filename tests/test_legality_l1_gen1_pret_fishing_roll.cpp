#include "Legality/Gen1PretFishingRollStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen1PretFishingRollStage;
using S=H::Status;
using R=H::Rod;
H::Witness witness(const char* game,R rod,uint8_t n=4){
 H::Witness w{};
 w.game=game;w.rod=rod;w.originalGroupSize=n;
 w.gameAttested=w.groupAttested=true;
 w.originalDrawsAttested=w.sourceSlotAttested=w.originalBiteAttested=true;
 w.expectedBite=true;return w;
}
int main(){
 static_assert(H::yellowSuperSlot(101)==0);
 static_assert(H::yellowSuperSlot(102)==1);
 static_assert(H::yellowSuperSlot(178)==2);
 static_assert(H::yellowSuperSlot(229)==3);
 std::array<unsigned,4> slotCounts{};
 uint64_t compatible=0,negative=0,unknown=0;
 for(unsigned s=0;s<256;++s)++slotCounts[H::yellowSuperSlot(s)];
 assert((slotCounts==std::array<unsigned,4>{102,76,51,27}));
 compatible+=4;
 // Yellow chooses the source physical slot before a separate 50% bite
 // draw; every original pair of 8-bit rolls is exercised.
 for(unsigned first=0;first<256;++first){
   const uint8_t slot=H::yellowSuperSlot(static_cast<uint8_t>(first));
   for(unsigned second=0;second<256;++second){
     auto w=witness("yellow_gb",R::Super);
     w.originalDraws[0]=static_cast<uint8_t>(first);
     w.originalDraws[1]=static_cast<uint8_t>(second);
     w.drawCount=2;
     w.expectedPhysicalSlot=slot;
     w.expectedBite=(second&1u)!=0;
     const auto r=H::evaluate(w);
     assert(r.status==(w.expectedBite?S::ConditionalOriginalRodRollOnly:
                                      S::CandidateLocalNoBite));
     assert(r.drawsConsumed==2);
     assert(r.bite==w.expectedBite);
     assert(!r.globallyLegalSupported&&!r.globalInvalidSupported);
     if(w.expectedBite)compatible+=4;else negative+=4;
     auto mismatched=w;mismatched.expectedBite=!w.expectedBite;
     assert(H::evaluate(mismatched).status==S::CandidateLocalBiteMismatch);
     ++negative;
     if(w.expectedBite){
       mismatched=w;mismatched.expectedPhysicalSlot=(slot+1u)%4u;
       assert(H::evaluate(mismatched).status==S::CandidateLocalSlotMismatch);
       ++negative;
     }
   }
 }
 // Red/Blue Super Rod groups have 2, 3 or 4 source entries; the Good
 // Rod uses the same random loop with 2 entries across all R/B/Y.
 for(const char* game:{"red_gb","blue_gb","yellow_gb"}){
   for(const R rod:{R::Good,R::Super}){
     if(game[0]=='y'&&rod==R::Super)continue;
     for(uint8_t count=2;count<=4;++count) {
       if(rod==R::Good&&count!=2)continue;
       for(unsigned first=0;first<256;++first){
         auto w=witness(game,rod,count);
         w.originalDraws[0]=static_cast<uint8_t>(first);
         w.drawCount=1;
         const bool odd=(first&1u)!=0;
         const uint8_t slot=static_cast<uint8_t>((first>>1)&3u);
         w.expectedPhysicalSlot=slot;w.expectedBite=!odd;
         const auto r=H::evaluate(w);
         if(odd){
           assert(r.status==S::CandidateLocalNoBite);
           assert(r.drawsConsumed==1);++negative;
         }else if(slot>=count){
           assert(r.status==S::UnknownRerollLimit);
           ++unknown;
         }else{
           assert(r.status==S::ConditionalOriginalRodRollOnly);
           assert(r.physicalSlot==slot&&r.drawsConsumed==1);
           compatible+=2;
         }
         // Independently recorded reroll (even first draw rejected).
         if(!odd&&slot>=count) {
           w.originalDraws[1]=0;w.drawCount=2;
           w.expectedPhysicalSlot=0;
           assert(H::evaluate(w).status==S::ConditionalOriginalRodRollOnly);
           compatible++;
           w.originalDraws[1]=3;w.expectedBite=false;
           assert(H::evaluate(w).status==S::CandidateLocalNoBite);
           negative++;
         }
       }
     }
   }
 }
 // Original Old Rod always chooses level-5 Magikarp slot 0 after
 // FishingInit. The original game consumes no source Random draws.
 for(const char* game:{"red_gb","blue_gb","yellow_gb"}){
   auto w=witness(game,R::Old,0);
   assert(H::evaluate(w).status==S::ConditionalOriginalRodRollOnly);
   assert(H::evaluate(w).drawsConsumed==0);
   compatible+=2;
   w.expectedPhysicalSlot=1;
   assert(H::evaluate(w).status==S::CandidateLocalSlotMismatch);
   ++negative;
   w.expectedPhysicalSlot=0;w.expectedBite=false;
   assert(H::evaluate(w).status==S::CandidateLocalBiteMismatch);
   ++negative;
 }
 for(unsigned i=0;i<256;++i){
   auto w=witness("yellow_gb",R::Super,4);
   w.originalDraws[0]=static_cast<uint8_t>(i);
   w.drawCount=1;
   assert(H::evaluate(w).status==S::UnknownOriginalRandomHistory);
   ++unknown;
   w.drawCount=2;w.originalDrawsAttested=false;
   assert(H::evaluate(w).status==S::UnknownOriginalRandomHistory);
   ++unknown;
   w=witness("red_gb",R::Super,4);
   w.groupAttested=false;
   assert(H::evaluate(w).status==S::UnknownOriginalGroup);
   ++unknown;
 }
 auto unknownGame=witness("unverified_gb",R::Super);
 assert(H::evaluate(unknownGame).status==S::UnknownOriginalGame);
 auto badGroup=witness("red_gb",R::Super,1);
 assert(H::evaluate(badGroup).status==S::UnknownOriginalGroup);
 auto unknownSlot=witness("yellow_gb",R::Super);
 unknownSlot.sourceSlotAttested=false;
 assert(H::evaluate(unknownSlot).status==S::UnknownOriginalSlotWitness);
 auto badRod=witness("blue_gb",static_cast<R>(255));
 assert(H::evaluate(badRod).status==S::CandidateLocalWrongRod);
 unknown+=3;++negative;
 std::cout<<"L1 exact RBY rod RNG source test "<<compatible
          <<" conditional checks, "<<negative<<" local negatives, "
          <<unknown<<" Unknown/RNG rerolls PASS\n";
}
