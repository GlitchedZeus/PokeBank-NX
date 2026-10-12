#include "Legality/Gen3GbaOriginalMachineData.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace T=Legality::Gen3GbaOriginalMachineData;
constexpr bool can(uint16_t species,uint8_t item,bool frlg=false) {
    if(!species||species>386||!item||item>58)return false;
    const auto mask=frlg?T::kFireRedLeafGreenMasks[species]:
                         T::kRubySapphireMasks[species];
    return (mask&(uint64_t{1}<<(item-1)))!=0;
}
static_assert(T::kMachineMove.size()==58);
static_assert(T::kMachineMove[5]==92); // TM06 Toxic
static_assert(T::kMachineMove[9]==237); // TM10 Hidden Power
static_assert(T::kMachineMove[50]==15); // HM01 Cut
static_assert(T::kMachineMove[57]!=0); // HM08 Dive
static_assert(can(1,6) && can(1,10) && can(1,51)); // Bulbasaur
static_assert(can(1,6,true) && can(1,10,true) && can(1,51,true));
static_assert(!can(1,58)); // Bulbasaur cannot learn HM08 Dive
static_assert(can(133,6) && !can(133,58)); // Eevee
static_assert(!can(0,6) && !can(387,6) && !can(1,0) && !can(1,59));
static_assert(T::kRubySapphireMasks[290]==T::kFireRedLeafGreenMasks[290]);
// Compare ALL source-attested species, all 58 numeric receipts and
// validate bitmask domain. Game-family equality is independently checked
// against the pinned Emerald per-species .learnset data as well.
int main() {
    uint32_t allCompatible=0;
    uint16_t withNoMachines=0,withAtLeastOne=0;
    constexpr uint64_t kDomain=(uint64_t{1}<<58)-1;
    for(uint16_t species=1;species<=386;++species) {
        const uint64_t a=T::kRubySapphireMasks[species];
        const uint64_t b=T::kFireRedLeafGreenMasks[species];
        assert((a&~kDomain)==0 && (b&~kDomain)==0);
        assert(a==b); // independent 386-species Ruby/FRLG source parity
        if(a)++withAtLeastOne;else ++withNoMachines;
        for(uint8_t item=1;item<=58;++item) {
            assert(can(species,item)==can(species,item,true));
            if(can(species,item)) {
                assert(T::kMachineMove[item-1]>=1 &&
                       T::kMachineMove[item-1]<=354);
                ++allCompatible;
            }
        }
    }
    assert(withAtLeastOne+withNoMachines==386);
    assert(allCompatible>3000);
    std::cout << "L3 original 386-species x 58 GBA machine bitmap: "
              << allCompatible << " compatible source methods: PASS\n";
}
