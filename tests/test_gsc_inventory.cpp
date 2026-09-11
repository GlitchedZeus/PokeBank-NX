#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Names/ItemNames.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
using namespace PokeVault::Integration::Gen2;
namespace {
struct O{size_t tm,item,key,ball,pc;};constexpr O IGS{0x23E6,0x241F,0x2449,0x2464,0x247E};constexpr O IC{0x23E7,0x2420,0x244A,0x2465,0x247F};constexpr O JGS{0x23C7,0x2400,0x242A,0x2445,0x245F};constexpr O JC{0x23C9,0x2402,0x242C,0x2447,0x2461};
void emptyPairs(std::vector<uint8_t>&b,size_t o){b[o]=0;b[o+1]=0xFF;}void emptyKeys(std::vector<uint8_t>&b,size_t o){b[o]=0;b[o+1]=0xFF;}
std::vector<uint8_t> f(O o){std::vector<uint8_t>b(0x10000);emptyPairs(b,o.item);emptyKeys(b,o.key);emptyPairs(b,o.ball);emptyPairs(b,o.pc);return b;}
void pair(std::vector<uint8_t>&b,size_t o,uint8_t id,uint8_t q){b[o]=1;b[o+1]=id;b[o+2]=q;b[o+3]=0xFF;}void key(std::vector<uint8_t>&b,size_t o,uint8_t id){b[o]=1;b[o+1]=id;b[o+2]=0xFF;}
void checkEmpty(O o,RegionLayout r,VersionFamily v){auto b=f(o);auto x=decodeInventory(b,r,v);assert(x.available&&x.tmhm.empty()&&x.items.empty()&&x.keyItems.empty()&&x.balls.empty()&&x.pcItems.empty());}
}
int main(){
 checkEmpty(IGS,RegionLayout::International,VersionFamily::GoldSilver);checkEmpty(IC,RegionLayout::International,VersionFamily::Crystal);checkEmpty(JGS,RegionLayout::Japanese,VersionFamily::GoldSilver);checkEmpty(JC,RegionLayout::Japanese,VersionFamily::Crystal);
 {auto b=f(IGS);b[IGS.tm]=3;b[IGS.tm+50]=1;pair(b,IGS.item,173,2);key(b,IGS.key,7);pair(b,IGS.ball,1,9);pair(b,IGS.pc,191,4);auto before=b;auto x=decodeInventory(b,RegionLayout::International,VersionFamily::GoldSilver);assert(x.available);assert(x.tmhm.size()==2&&x.items.size()==1&&x.keyItems.size()==1&&x.balls.size()==1&&x.pcItems.size()==1);assert(x.items[0].itemId==173&&x.items[0].quantity==2);assert(b==before);}
 {auto b=f(IC);key(b,IC.key,129);auto x=decodeInventory(b,RegionLayout::International,VersionFamily::Crystal);assert(x.available&&x.keyItems.size()==1);auto y=decodeInventory(b,RegionLayout::International,VersionFamily::GoldSilver);assert(!y.available);}
 {auto b=f(IGS);b[IGS.item]=21;auto x=decodeInventory(b,RegionLayout::International,VersionFamily::GoldSilver);assert(!x.available&&x.items.empty());}
 {auto b=f(IGS);pair(b,IGS.item,173,0);auto x=decodeInventory(b,RegionLayout::International,VersionFamily::GoldSilver);assert(!x.available);}
 {auto b=f(IGS);pair(b,IGS.item,173,100);auto x=decodeInventory(b,RegionLayout::International,VersionFamily::GoldSilver);assert(!x.available);}
 {auto b=f(IGS);b[IGS.item]=1;b[IGS.item+1]=173;b[IGS.item+2]=1;b[IGS.item+3]=0;auto x=decodeInventory(b,RegionLayout::International,VersionFamily::GoldSilver);assert(!x.available);}
 {auto b=f(IGS);pair(b,IGS.item,7,1);auto x=decodeInventory(b,RegionLayout::International,VersionFamily::GoldSilver);assert(!x.available);}
 {auto b=f(IGS);b[IGS.tm+50]=2;auto x=decodeInventory(b,RegionLayout::International,VersionFamily::GoldSilver);assert(!x.available);}
 assert(std::string(inventoryCategoryName(0))=="TM/HM");assert(std::string(inventoryCategoryName(4))=="PC Items");assert(std::string(inventoryCategoryName(5))=="Invalid category");
 assert(std::string(Names::getItemNameG2(1))=="Master Ball");
 assert(std::string(Names::getItemNameG2(67))=="Red Scale");
 assert(std::string(Names::getItemNameG2(173))=="Berry");
 assert(std::string(Names::getItemNameG2(191))=="TM01");
 assert(std::string(Names::getItemNameG2(243))=="HM01");
 assert(std::string(Names::getItemNameG2(249))=="HM07");
 assert(std::string(Names::getItemNameG2(6))=="???");
 assert(std::string(Names::getItemNameG2(195))=="???");
 std::cout<<"GSC strict read-only inventory tests passed\n";
}
