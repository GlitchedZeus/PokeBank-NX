#include "Integration/Gen2/Gen2ReadOnlyInventory.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace PokeVault::Integration::Gen2;

namespace {
struct I { size_t tm, items, key, balls, pc; };
constexpr I IGS{0x23E6,0x241F,0x2449,0x2464,0x247E};
constexpr I IC {0x23E7,0x2420,0x244A,0x2465,0x247F};
constexpr I JGS{0x23C7,0x2400,0x242A,0x2445,0x245F};
constexpr I JC {0x23C9,0x2402,0x242C,0x2447,0x2461};

std::vector<uint8_t> fixture(I i) {
    std::vector<uint8_t>d(0x10000);
    d[i.items]=1; d[i.items+1]=173; d[i.items+2]=2; d[i.items+3]=0xFF;
    d[i.key]=1; d[i.key+1]=7; d[i.key+2]=0xFF;
    d[i.balls]=1; d[i.balls+1]=1; d[i.balls+2]=12; d[i.balls+3]=0xFF;
    d[i.pc]=1; d[i.pc+1]=126; d[i.pc+2]=1; d[i.pc+3]=0xFF;
    d[i.tm]=3;
    d[i.tm+50]=1;
    return d;
}
void check(I i,RegionLayout r,VersionFamily f) {
    auto d=fixture(i); const auto before=d;
    auto inv=decodeInventory(d,r,f);
    assert(inv.available);
    assert(inv.tmhm.size()==2);
    assert(inv.tmhm[0].itemId==191&&inv.tmhm[0].quantity==3&&inv.tmhm[0].name=="TM01");
    assert(inv.tmhm[1].itemId==243&&inv.tmhm[1].quantity==1&&inv.tmhm[1].name=="HM01");
    assert(inv.items.size()==1&&inv.items[0].name=="BERRY");
    assert(inv.keyItems.size()==1&&inv.keyItems[0].name=="BICYCLE"&&inv.keyItems[0].quantity==1);
    assert(inv.balls.size()==1&&inv.balls[0].name=="MASTER BALL");
    assert(inv.pcItems.size()==1&&inv.pcItems[0].name=="LUCKY EGG");
    assert(d==before);
}
}

int main(){
    check(IGS,RegionLayout::International,VersionFamily::GoldSilver);
    check(IC,RegionLayout::International,VersionFamily::Crystal);
    check(JGS,RegionLayout::Japanese,VersionFamily::GoldSilver);
    check(JC,RegionLayout::Japanese,VersionFamily::Crystal);
    assert(gen2ItemName(70)=="CLEAR BELL");
    assert(gen2ItemName(115)=="GS BALL");
    assert(gen2ItemName(116)=="BLUE CARD");
    assert(gen2ItemName(129)=="EGG TICKET");
    assert(gen2ItemName(249)=="HM07");
    {
        auto d=fixture(IGS); d[IGS.items]=21;
        auto inv=decodeInventory(d,RegionLayout::International,VersionFamily::GoldSilver);
        assert(!inv.available);
    }
    {
        auto d=fixture(IGS); d[IGS.items+3]=0;
        auto inv=decodeInventory(d,RegionLayout::International,VersionFamily::GoldSilver);
        assert(!inv.available);
    }
    {
        auto d=fixture(IGS); d[IGS.tm+50]=2;
        auto inv=decodeInventory(d,RegionLayout::International,VersionFamily::GoldSilver);
        assert(!inv.available);
    }
    std::cout<<"Generation II read-only inventory: PASS\n";
}
