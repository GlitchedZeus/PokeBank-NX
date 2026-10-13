// Exhaustive pret/pokered+pret/pokeyellow original Gen I physical slot
// regression. No synthetic dataset may replace the pinned generated rows.
#include "Legality/Gen1OriginalWildFishingStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
namespace H=Legality::Gen1OriginalWildFishingStage;
static H::Candidate candidate(std::size_t i,const H::PretSlot& row,
                             std::string_view game) {
 H::Candidate c{};c.originalGame=game;c.originalSourceIndex=i;
 c.originalMap=row.map=="*"?"Route12":row.map;
 c.originalMethod=row.method;c.originalSpecies=row.species;
 c.originalLevel=row.level;
 c.originalGameAttested=c.originalSourceSlotAttested=true;
 c.originalMapAttested=c.originalMethodAttested=true;
 c.originalSpeciesAttested=c.originalLevelAttested=true;
 return c;
}
static std::size_t find(std::string_view map,H::Method method,unsigned slot,
                        uint8_t mask) {
 for(std::size_t i=0;i<H::kPretSlotCount;++i) {
   const auto* r=H::pretSlot(i);
   if(r && r->map==map&&r->method==method&&r->physicalSlot==slot
      &&(r->gameMask&mask))return i;
 }
 return H::kPretSlotCount;
}
int main(){
 static_assert(H::kVerifiedSlots.size()==149);
 static_assert(H::kPretSlotCount==1511);
 using Status=H::Status;
 std::uint64_t positive=0,negative=0,unknown=0,foreign=0;
 for(std::size_t i=0;i<H::kPretSlotCount;++i) {
   const auto* r=H::pretSlot(i);assert(r);
   assert(r->gameMask!=0&&r->gameMask<=7);
   assert(r->species>=1&&r->species<=151);
   assert(r->level>=1&&r->level<=100);
   assert(r->physicalSlot<=9);
   if(r->method==H::Method::Grass||r->method==H::Method::Surf)
     assert(r->encounterRate!=0);
   else assert(r->encounterRate==0);
   for(unsigned g=0;g<3;++g) {
     const uint8_t bit=static_cast<uint8_t>(1u<<g);
     const std::string_view game=g==0?"red_gb":g==1?"blue_gb":"yellow_gb";
     auto c=candidate(i,*r,game);
     const auto result=H::evaluatePret(c);
     if(!(r->gameMask&bit)){
        assert(result.status==Status::ForeignGameRowNotEvidence);
        ++foreign;continue;
     }
     if(r->map=="*")
       assert(result.status==Status::UnknownMapApplicabilityForGlobalRod);
     else
       assert(result.status==Status::SourceSlotCompatibleRateAndHistoryUnverified);
     ++positive;
     auto bad=c;bad.originalSpecies=static_cast<uint16_t>(r->species==1?2:1);
     assert(H::evaluatePret(bad).status==Status::CandidateOriginalSpeciesContradicted);
     bad=c;bad.originalLevel=static_cast<uint8_t>(r->level==100?99:r->level+1);
     assert(H::evaluatePret(bad).status==Status::CandidateOriginalLevelContradicted);
     bad=c;bad.originalMethod=r->method==H::Method::Grass?
         H::Method::SuperRod:H::Method::Grass;
     assert(H::evaluatePret(bad).status==Status::CandidateOriginalMethodContradicted);
     negative+=3;
     if(r->map!="*"){
        bad=c;bad.originalMap="AnotherPhysicalMap";
        assert(H::evaluatePret(bad).status==Status::CandidateOriginalMapContradicted);
        ++negative;
     }
     auto absent=c;absent.originalGameAttested=false;
     assert(H::evaluatePret(absent).status==Status::UnknownOriginalGame);
     absent=c;absent.originalSourceSlotAttested=false;
     assert(H::evaluatePret(absent).status==Status::UnknownOriginalSourceRow);
     absent=c;absent.originalMapAttested=false;
     assert(H::evaluatePret(absent).status==Status::UnknownOriginalMap);
     absent=c;absent.originalMethodAttested=false;
     assert(H::evaluatePret(absent).status==Status::UnknownOriginalMethod);
     absent=c;absent.originalSpeciesAttested=false;
     assert(H::evaluatePret(absent).status==Status::UnknownOriginalSpecies);
     absent=c;absent.originalLevelAttested=false;
     assert(H::evaluatePret(absent).status==Status::UnknownOriginalLevel);
     unknown+=6;
   }
 }
 assert(H::pretSlot(H::kPretSlotCount)==nullptr);
 auto missing=candidate(H::kPretSlotCount,*H::pretSlot(0),"red_gb");
 assert(H::evaluatePret(missing).status==Status::UnknownOriginalSourceRow);
 const auto vRed=find("ViridianForest",H::Method::Grass,0,1);
 const auto vBlue=find("ViridianForest",H::Method::Grass,0,2);
 const auto vYellow=find("ViridianForest",H::Method::Grass,0,4);
 assert(vRed<H::kPretSlotCount&&vBlue<H::kPretSlotCount&&vYellow<H::kPretSlotCount);
 assert(H::pretSlot(vRed)->species==13);
 assert(H::pretSlot(vBlue)->species==10);
 assert(H::pretSlot(vYellow)->species==10);
 assert(H::pretSlot(vYellow)->level==3);
 const auto yRod=find("PalletTown",H::Method::SuperRod,0,4);
 assert(yRod<H::kPretSlotCount);
 assert(H::pretSlot(yRod)->species==120&&H::pretSlot(yRod)->level==10);
 assert(find("Route19",H::Method::Surf,0,1)<H::kPretSlotCount);
 assert(find("Route20",H::Method::Surf,0,2)<H::kPretSlotCount);
 assert(find("Route19",H::Method::Surf,0,4)<H::kPretSlotCount);
 assert(find("Route20",H::Method::Surf,0,4)<H::kPretSlotCount);
 // Multiple identical species/levels remain separate physical slots.
 const auto a=find("Route1",H::Method::Grass,0,1);
 const auto b=find("Route1",H::Method::Grass,5,1);
 assert(a!=b&&a<H::kPretSlotCount&&b<H::kPretSlotCount);
 assert(H::pretSlot(a)->species==H::pretSlot(b)->species);
 for(unsigned byte=5;byte<256;++byte){
   auto bad=candidate(a,*H::pretSlot(a),"red_gb");
   bad.originalMethod=static_cast<H::Method>(byte);
   assert(H::evaluatePret(bad).status==Status::UnknownOriginalMethod);
   ++unknown;
 }
 std::cout<<"L1 pret Gen I "<<H::kPretSlotCount
          <<" physical Grass/Surf/rod rows: "<<positive
          <<" candidate-compatible/partial, "<<negative
          <<" candidate-local contradictions, "<<unknown
          <<" Unknown, "<<foreign<<" foreign-game checks PASS\n";
}
