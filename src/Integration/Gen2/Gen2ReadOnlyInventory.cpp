#include "Integration/Gen2/Gen2ReadOnlyInventory.h"

#include <algorithm>
#include <array>

namespace PokeVault::Integration::Gen2 {
namespace {
struct Offsets { std::size_t tmhm,items,key,balls,pc; };
constexpr Offsets kIntGS{0x23E6,0x241F,0x2449,0x2464,0x247E};
constexpr Offsets kIntC {0x23E7,0x2420,0x244A,0x2465,0x247F};
constexpr Offsets kJpGS {0x23C7,0x2400,0x242A,0x2445,0x245F};
constexpr Offsets kJpC  {0x23C9,0x2402,0x242C,0x2447,0x2461};
const Offsets& offsets(RegionLayout r, VersionFamily f) noexcept {
    if(r==RegionLayout::Japanese) return f==VersionFamily::Crystal?kJpC:kJpGS;
    return f==VersionFamily::Crystal?kIntC:kIntGS;
}
constexpr std::array<uint8_t,141> kGeneral{{
3,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,46,47,48,49,51,52,53,57,60,62,63,64,65,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,91,92,93,94,95,96,97,98,99,101,102,103,104,105,106,107,108,109,110,111,112,113,114,117,118,119,121,122,123,124,125,126,131,132,138,139,140,143,144,146,150,151,152,156,158,163,167,168,169,170,172,173,174,180,181,182,183,184,185,186,187,188,189,
}};
constexpr std::array<uint8_t,11> kBalls{{1,2,4,5,157,159,160,161,164,165,166}};
constexpr std::array<uint8_t,18> kKeyGS{{7,54,55,58,59,61,66,67,68,69,71,127,128,130,133,134,175,178}};
constexpr std::array<uint8_t,4> kKeyCExtra{{70,115,116,129}};
constexpr std::array<uint8_t,57> kMachine{{
191,192,193,194,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,
}};
template<class C> bool contains(const C& c,uint8_t v) noexcept { return std::find(c.begin(),c.end(),v)!=c.end(); }
bool keyValid(uint8_t id,VersionFamily f) noexcept { return contains(kKeyGS,id)||(f==VersionFamily::Crystal&&contains(kKeyCExtra,id)); }
bool pcValid(uint8_t id,VersionFamily f) noexcept { return contains(kGeneral,id)||contains(kBalls,id)||contains(kMachine,id)||keyValid(id,f); }

bool decodePairs(std::span<const uint8_t> p,std::size_t o,std::size_t cap,VersionFamily f,std::size_t category,std::vector<InventoryEntry>& out,std::string& err) {
    const std::size_t fixed=1+cap*2+1; if(o>p.size()||p.size()-o<fixed){err="inventory block truncated";return false;}
    const uint8_t count=p[o]; if(count>cap){err="inventory count exceeds pouch capacity";return false;}
    out.clear(); out.reserve(count);
    for(std::size_t i=0;i<count;++i){ const uint8_t id=p[o+1+i*2],q=p[o+2+i*2]; if(!isValidGen2Item(id,f,category)){err="invalid raw Gen II item id";return false;} if(q<1||q>99){err="Gen II item quantity outside 1-99";return false;} out.push_back({id,q}); }
    if(p[o+1+count*2]!=0xFF){err="inventory terminator missing";return false;} return true;
}
bool decodeKeys(std::span<const uint8_t> p,std::size_t o,VersionFamily f,std::vector<InventoryEntry>& out,std::string& err) {
    constexpr std::size_t cap=26,fixed=1+cap+1; if(o>p.size()||p.size()-o<fixed){err="key-item block truncated";return false;}
    const uint8_t count=p[o]; if(count>cap){err="key-item count exceeds pouch capacity";return false;} out.clear();out.reserve(count);
    for(std::size_t i=0;i<count;++i){const uint8_t id=p[o+1+i];if(!keyValid(id,f)){err="invalid Gen II key-item id";return false;}out.push_back({id,1});}
    if(p[o+1+count]!=0xFF){err="key-item terminator missing";return false;} return true;
}
bool decodeTMHM(std::span<const uint8_t> p,std::size_t o,std::vector<InventoryEntry>& out,std::string& err) {
    if(o>p.size()||p.size()-o<kMachine.size()){err="TM/HM block truncated";return false;} out.clear();
    for(std::size_t i=0;i<kMachine.size();++i){uint8_t q=p[o+i];if(q==0)continue;if(i>=50&&q!=1){err="HM quantity must be 0 or 1";return false;}if(q>99){err="TM quantity exceeds 99";return false;}out.push_back({kMachine[i],q});} return true;
}
}
const char* inventoryCategoryName(std::size_t c) noexcept { static constexpr const char* n[]={"TM/HM","Items","Key Items","Balls","PC Items"}; return c<5?n[c]:"Invalid category"; }
bool isValidGen2Item(uint8_t id,VersionFamily f,std::size_t c) noexcept {
    switch(c){case 0:return contains(kMachine,id);case 1:return contains(kGeneral,id);case 2:return keyValid(id,f);case 3:return contains(kBalls,id);case 4:return pcValid(id,f);default:return false;}
}
ReadOnlyInventory decodeInventory(std::span<const uint8_t> p,RegionLayout r,VersionFamily f){
    ReadOnlyInventory x; const auto& o=offsets(r,f); std::string err;
    if(!decodeTMHM(p,o.tmhm,x.tmhm,err)||!decodePairs(p,o.items,20,f,1,x.items,err)||!decodeKeys(p,o.key,f,x.keyItems,err)||!decodePairs(p,o.balls,12,f,3,x.balls,err)||!decodePairs(p,o.pc,50,f,4,x.pcItems,err)){x.error=std::move(err);x.tmhm.clear();x.items.clear();x.keyItems.clear();x.balls.clear();x.pcItems.clear();return x;}
    x.available=true;return x;
}
} // namespace PokeVault::Integration::Gen2
