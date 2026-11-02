#include "Legality/Gen1OriginalWildFishingStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
namespace S=Legality::Gen1OriginalWildFishingStage;
namespace W=Legality::Gen1PretWildRollStage;
int main(){
 static_assert(S::kPretSlotCount==1621);
 static_assert(W::sumWeights()==256);
 std::uint64_t physical=0,groups=0,rod=0,checks=0;
 for(std::size_t i=0;i<S::kPretSlotCount;++i){
  const auto* source=S::pretSlot(i);
  assert(source);
  const auto weight=S::pretWildSlotWeight(i);
  if(source->method!=S::Method::Grass&&source->method!=S::Method::Surf){
    assert(weight==0);
    ++rod;++checks;continue;
  }
  assert(source->physicalSlot<10);
  assert(weight==W::kSlotWeight[source->physicalSlot]);
  ++physical;checks+=2;
  // Each source-table slot belongs to exactly one map, method and version.
  // A repeated species+level in the table is still a distinct weighted slot.
  if(source->physicalSlot!=0)continue;
  for(uint8_t mask: {static_cast<uint8_t>(1),
                     static_cast<uint8_t>(2),
                     static_cast<uint8_t>(4)}){
    if(!(source->gameMask&mask))continue;
    std::array<unsigned,10> slots{};
    unsigned weightSum=0;
    for(std::size_t j=0;j<S::kPretSlotCount;++j){
      const auto* item=S::pretSlot(j);
      if(!item || !(item->gameMask&mask) ||
         item->map!=source->map || item->method!=source->method)continue;
      assert(item->physicalSlot<10);
      ++slots[item->physicalSlot];
      weightSum+=S::pretWildSlotWeight(j);
    }
    for(unsigned physicalSlot=0;physicalSlot<10;++physicalSlot)
      assert(slots[physicalSlot]==1); // no missing or duplicated physical slots
    assert(weightSum==256);
    ++groups;checks+=11;
  }
 }
 assert(physical==1385); // map-physical R/B/Y grass + surf in 8 includes
 assert(rod==236); // Yellow 124 + Red/Blue 109 Super Rod + Old/Good game-global 3
 assert(S::pretWildSlotWeight(S::kPretSlotCount)==0);
 assert(S::pretSlot(S::kPretSlotCount)==nullptr);
 checks+=4;
 std::cout<<"L1 original RBY slot-probability check: "<<physical
          <<" grass/surf rows, "<<rod<<" rods without wild weights, "
          <<groups<<" complete version/map/method 256/256 tables, "
          <<checks<<" assertions PASS\n";
}
