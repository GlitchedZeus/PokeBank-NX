#include "Legality/Gen1PretWildRollStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace G=Legality::Gen1PretWildRollStage;
using S=G::Status;
using M=G::Method;
G::OriginalScene original(const char* title,uint8_t selected,uint8_t randAdd=0){
 G::OriginalScene c{};
 c.game=title;
 c.gameIndependentlyAttested=true;
 c.tileStateIndependentlyAttested=true;
 c.movementAndMapGatesIndependentlyAttested=true;
 c.movementAndMapAllowEncounter=true;
 c.originalRandomIndependentlyAttested=true;
 c.originalRandomAdd=randAdd;
 c.originalRandomSub=selected;
 c.leftBottomTile=c.rightBottomTile=0x14;
 c.grassTile=0x52;
 c.grassRate=80;c.waterRate=64;
 c.originalExpectedPhysicalSlot=G::slot(selected);
 c.originalExpectedMethod=M::Surf;
 c.sourceSlotIndependentlyAttested=true;
 c.originalRepelStateIndependentlyAttested=true;
 c.originalLeadPartyLevelIndependentlyAttested=true;
 c.originalSourceLevelIndependentlyAttested=true;
 c.originalLeadPartyLevel=10;c.originalSourceLevel=10;
 return c;
}
int main(){
 static_assert(G::sumWeights()==256);
 uint64_t positive=0,local=0,unknown=0;
 std::array<uint16_t,10> counts{};
 for(unsigned b=0;b<256;++b) {
  const auto n=G::slot(static_cast<uint8_t>(b));
  assert(n<10);
  ++counts[n];
 }
 for(unsigned i=0;i<10;++i)assert(counts[i]==G::kSlotWeight[i]);
 positive+=10;
 for(const char* version:{"red_gb","blue_gb","yellow_gb"}) {
  for(unsigned roll=0;roll<256;++roll) {
    auto c=original(version,static_cast<uint8_t>(roll));
    auto r=G::evaluate(c);
    assert(r.status==S::ConditionalOriginalSlotRollOnly);
    assert(r.physicalSlot==c.originalExpectedPhysicalSlot);
    assert(r.method==M::Surf && r.threshold==64);
    assert(!r.globallyLegalSupported&&!r.globalInvalidSupported);
    positive+=5;
    auto bad=c;bad.originalExpectedPhysicalSlot=
        static_cast<uint8_t>((c.originalExpectedPhysicalSlot+1)%10);
    assert(G::evaluate(bad).status==S::CandidateLocalSlotMismatch);
    bad=c;bad.originalExpectedMethod=M::Grass;
    assert(G::evaluate(bad).status==S::CandidateLocalMethodMismatch);
    local+=2;
    auto missing=c;missing.originalRandomIndependentlyAttested=false;
    assert(G::evaluate(missing).status==S::UnknownOriginalRandom);
    missing=c;missing.tileStateIndependentlyAttested=false;
    assert(G::evaluate(missing).status==S::UnknownOriginalTileState);
    missing=c;missing.sourceSlotIndependentlyAttested=false;
    assert(G::evaluate(missing).status==S::UnknownOriginalEncounterGate);
    missing=c;missing.originalRepelStateIndependentlyAttested=false;
    assert(G::evaluate(missing).status==S::UnknownOriginalRepelContext);
    unknown+=4;
  }
  for(unsigned i=0;i<256;++i) {
   auto c=original(version,0,static_cast<uint8_t>(i));
   const auto r=G::evaluate(c);
   if(i<64)assert(r.status==S::ConditionalOriginalSlotRollOnly);
   else assert(r.status==S::CandidateLocalNoEncounter);
   ++(i<64?positive:local);
  }
 }
 // Red and Blue use bottom RIGHT for encounter chance, but bottom LEFT
 // for the selected species table. Yellow checks LEFT for both.
 for(const char* title:{"red_gb","blue_gb"}) {
   auto c=original(title,0);
   c.leftBottomTile=0x01;c.rightBottomTile=0x14;
   c.originalExpectedMethod=M::Grass;
   const auto r=G::evaluate(c);
   assert(r.status==S::ConditionalOriginalSlotRollOnly);
   assert(r.method==M::Grass&&r.threshold==64);
   positive+=2;
   c.originalExpectedMethod=M::Surf;
   assert(G::evaluate(c).status==S::CandidateLocalMethodMismatch);
   ++local;
 }
 auto yellow=original("yellow_gb",0);
 yellow.leftBottomTile=0x01;yellow.rightBottomTile=0x14;
 yellow.originalExpectedMethod=M::Grass;
 assert(G::evaluate(yellow).status==S::CandidateLocalNoEncounter);
 ++local;
 // Indoor non-FOREST default grass encounters can occur on non-grass tiles.
 auto cave=original("red_gb",0);
 cave.leftBottomTile=cave.rightBottomTile=0x01;
 cave.indoorMap=true;cave.forestTileset=false;
 cave.originalExpectedMethod=M::Grass;
 assert(G::evaluate(cave).status==S::ConditionalOriginalSlotRollOnly);
 cave.forestTileset=true;
 assert(G::evaluate(cave).status==S::CandidateLocalNoEncounter);
 ++positive;++local;
 // Fully attested repel blocking applies to ONLY this one witnessed slot.
 auto repel=original("red_gb",0);
 repel.repelActive=true;repel.originalLeadPartyLevel=15;repel.originalSourceLevel=14;
 assert(G::evaluate(repel).status==S::CandidateLocalRepelBlocked);
 repel.originalSourceLevel=15;
 assert(G::evaluate(repel).status==S::ConditionalOriginalSlotRollOnly);
 repel.originalLeadPartyLevelIndependentlyAttested=false;
 assert(G::evaluate(repel).status==S::UnknownOriginalRepelContext);
 ++local;++positive;++unknown;
 // Enumerate adversarial malformed game identifiers, tile gates and
 // conditional chance combinations without ever claiming global legality.
 for(unsigned game=0;game<256;++game) {
   for(unsigned roll=0;roll<256;++roll){
     auto c=original("red_gb",static_cast<uint8_t>(roll));
     c.game=(game%4==0?"red_gb":game%4==1?"blue_gb":
             game%4==2?"yellow_gb":"unverified_game");
     c.originalRandomAdd=static_cast<uint8_t>(roll);
     const auto r=G::evaluate(c);
     assert(!r.globallyLegalSupported&&!r.globalInvalidSupported);
     if(game%4==3)assert(r.status==S::UnknownOriginalGame);
     else if(roll>=64)assert(r.status==S::CandidateLocalNoEncounter);
     else assert(r.status==S::ConditionalOriginalSlotRollOnly);
     ++unknown;
   }
 }
 std::cout<<"L1 pret RBY source-true 10-slot 256 RNG weights and shoreline: "
          <<positive<<" positive, "<<local<<" local negative, "
          <<unknown<<" Unknown/boundary scenarios PASS\n";
}
