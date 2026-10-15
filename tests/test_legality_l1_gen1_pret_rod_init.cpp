#include "Legality/Gen1OriginalWildFishingStage.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
namespace H=Legality::Gen1OriginalWildFishingStage;
using S=H::Status;
H::Candidate make(std::size_t i,const H::PretSlot& row,const char* game) {
 H::Candidate c{};
 c.originalGame=game;
 c.originalGameAttested=c.originalSourceSlotAttested=true;
 c.originalSourceIndex=i;
 c.originalMap=row.map=="*"?"Route12":row.map;
 c.originalMapAttested=true;
 c.originalMethod=row.method;c.originalMethodAttested=true;
 c.originalSpecies=row.species;c.originalSpeciesAttested=true;
 c.originalLevel=row.level;c.originalLevelAttested=true;
 return c;
}
int main(){
 static_assert(H::kPretSlotCount==1621);
 uint64_t positive=0,negative=0,unknown=0;
 for(std::size_t i=0;i<H::kPretSlotCount;++i){
   const H::PretSlot* row=H::pretSlot(i);
   assert(row);
   if(row->method!=H::Method::OldRod &&
      row->method!=H::Method::GoodRod &&
      row->method!=H::Method::SuperRod)continue;
   for(unsigned g=0;g<3;++g){
     const uint8_t mask=static_cast<uint8_t>(1u<<g);
     const char* title=g==0?"red_gb":g==1?"blue_gb":"yellow_gb";
     auto c=make(i,*row,title);
     if(!(row->gameMask&mask)){
       assert(H::evaluatePretFishingInit(c,{true,true}).status==
              S::ForeignGameRowNotEvidence);
       ++unknown;continue;
     }
     const bool global=row->map=="*";
     assert(H::evaluatePret(c).status==
          (global?S::UnknownMapApplicabilityForGlobalRod:
                  S::SourceSlotCompatibleRateAndHistoryUnverified));
     auto r=H::evaluatePretFishingInit(c,{true,true});
     assert(r.status==S::SourceSlotCompatibleRateAndHistoryUnverified);
     assert(r.sourceSpecies==row->species);
     positive+=3;
     r=H::evaluatePretFishingInit(c,{false,true});
     assert(r.status==S::UnknownOriginalFishingInit);
     ++unknown;
     r=H::evaluatePretFishingInit(c,{true,false});
     assert(r.status==S::CandidateOriginalFishingInitContradicted);
     ++negative;
     // A local impossible fishing action never excludes a wild/starter/
     // NPC-trade, prior cross-game trade, or a competing rod source route.
     c.originalLevel=static_cast<uint8_t>(row->level==100?99:row->level+1);
     assert(H::evaluatePretFishingInit(c,{true,true}).status==
            S::CandidateOriginalLevelContradicted);
     ++negative;
   }
 }
 // Separate Red/Blue shared Super Rod and Yellow Super Rod sources are
 // not transferable to another original version by container location.
 std::size_t y=H::kPretSlotCount,r=H::kPretSlotCount;
 for(std::size_t i=0;i<H::kPretSlotCount;++i){
   const auto* row=H::pretSlot(i);
   if(row->map!="PalletTown"||row->method!=H::Method::SuperRod||
      row->physicalSlot!=0)continue;
   if(row->gameMask==4)y=i;
   if(row->gameMask==3)r=i;
 }
 assert(y<H::kPretSlotCount && r<H::kPretSlotCount);
 assert(H::pretSlot(y)->species==120 && H::pretSlot(y)->level==10);
 assert(H::pretSlot(r)->species==72 && H::pretSlot(r)->level==15);
 // The Red/Blue Super Rod rows of the pret namespace and of the legacy
 // kVerifiedSlots namespace come from the same pinned super_rod.asm: each
 // pret row matches exactly one legacy row, and both hold all 109 rows.
 {
   std::size_t legacy=0,pretRows=0;
   for(const auto& slot:H::kVerifiedSlots)
     if(slot.method==H::Method::SuperRod&&slot.gameMask==3)++legacy;
   for(std::size_t i=0;i<H::kPretSlotCount;++i){
     const auto* row=H::pretSlot(i);
     if(row->method!=H::Method::SuperRod||row->gameMask!=3)continue;
     std::size_t matches=0;
     for(const auto& slot:H::kVerifiedSlots)
       matches+=slot.method==H::Method::SuperRod&&slot.gameMask==3&&
                slot.map==row->map&&slot.physicalSlot==row->physicalSlot&&
                slot.species==row->species&&slot.level==row->level;
     assert(matches==1);
     ++pretRows;
   }
   assert(legacy==109&&pretRows==109);
 }
 auto yellow=make(y,*H::pretSlot(y),"yellow_gb");
 assert(H::evaluatePretFishingInit(yellow,{true,true}).status==
        S::SourceSlotCompatibleRateAndHistoryUnverified);
 yellow.originalGame="red_gb";
 assert(H::evaluatePretFishingInit(yellow,{true,true}).status==
        S::ForeignGameRowNotEvidence);
 std::cout<<"L1 Gen I original FishingInit rod source check: "
          <<positive<<" compatible, "<<negative
          <<" candidate-only negative and "<<unknown<<" Unknown/foreign PASS\n";
}
