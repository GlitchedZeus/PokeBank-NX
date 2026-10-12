#include "Legality/Gen3GbaOriginalVersionedLevelUpData.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace D=Legality::Gen3GbaOriginalVersionedLevelUpData;
constexpr uint16_t ruby(uint16_t species,uint16_t i) {
    return D::kRubySapphirePacked[D::kRubySapphireOffsets[species]+i];
}
constexpr uint16_t frlg(uint16_t species,uint16_t i) {
    return D::kFireRedLeafGreenPacked[D::kFireRedLeafGreenOffsets[species]+i];
}
constexpr uint16_t rubyCount(uint16_t sp){
    return D::kRubySapphireOffsets[sp+1]-D::kRubySapphireOffsets[sp];
}
constexpr uint16_t frlgCount(uint16_t sp){
    return D::kFireRedLeafGreenOffsets[sp+1]-D::kFireRedLeafGreenOffsets[sp];
}
static_assert(D::kRubySapphirePacked.size()==3947);
static_assert(D::kFireRedLeafGreenPacked.size()==4015);
static_assert(rubyCount(1)==11 && frlgCount(1)==11);
static_assert(ruby(4,3)==(13u<<9 | 108u));
static_assert(frlg(4,3)==(13u<<9 | 232u));
static_assert(rubyCount(290)==10 && frlgCount(290)==10);
static_assert(ruby(290,0)==(1u<<9 | 10u));
static_assert(ruby(290,1)==(1u<<9 | 106u));
static_assert(frlg(290,0)==(1u<<9 | 10u));
static_assert(rubyCount(252)==11 && frlgCount(252)==11);
static_assert(ruby(252,0)==(1u<<9 | 1u)); // Treecko Pound, not internal species #277
static_assert(ruby(386,0)==(1u<<9 | 43u)); // Deoxys Leer
static_assert(frlg(386,0)==(1u<<9 | 43u));
static_assert(rubyCount(386)==12 && frlgCount(386)==14);
static_assert(rubyCount(92)==9 && frlgCount(92)==12); // Gastly retail variant
static_assert(ruby(92,3)==(13u<<9 | 212u));
static_assert(frlg(92,3)==(13u<<9 | 174u));
static_assert(rubyCount(133)==9 && frlgCount(133)==9); // Eevee stays grounded
static_assert(ruby(133,0)==(1u<<9 | 33u));
static_assert(frlg(133,0)==(1u<<9 | 33u));
// Exhaustively check 386 National species, precise offsets, monotonic retail
// level offer ordering and original Gen III move domain in BOTH source families.
int main() {
    uint32_t rubyEntries=0,frlgEntries=0;
    uint16_t differingSpecies=0;
    uint8_t maxRuby=0,maxFrlg=0;
    uint16_t sameLevelPairs=0,identicalSpecies=0;
    for(uint16_t species=1;species<=386;++species) {
        const auto a=rubyCount(species),b=frlgCount(species);
        assert(a>=1 && a<=32 && b>=1 && b<=32);
        if(a>maxRuby)maxRuby=static_cast<uint8_t>(a);
        if(b>maxFrlg)maxFrlg=static_cast<uint8_t>(b);
        rubyEntries+=a;frlgEntries+=b;
        bool same=a==b;
        for(uint16_t j=0;j<a;++j) {
            const uint16_t value=ruby(species,j);
            const uint16_t level=value>>9,move=value&511u;
            assert(level>=1 && level<=100);
            assert(move>=1 && move<=354);
            if(j) {
                assert((ruby(species,j-1)>>9)<=level);
                if((ruby(species,j-1)>>9)==level)++sameLevelPairs;
            }
            if(j<b && value!=frlg(species,j))same=false;
        }
        for(uint16_t j=0;j<b;++j) {
            const uint16_t value=frlg(species,j);
            const uint16_t level=value>>9,move=value&511u;
            assert(level>=1 && level<=100);
            assert(move>=1 && move<=354);
            if(j)assert((frlg(species,j-1)>>9)<=level);
        }
        if(!same)++differingSpecies;
        else ++identicalSpecies;
    }
    assert(rubyEntries==3947 && frlgEntries==4015);
    assert(maxRuby==17 && maxFrlg==18);
    assert(differingSpecies==60 && identicalSpecies==326);
    assert(sameLevelPairs>100);
    std::cout<<"L3 original full Ruby/Sapphire & FRLG: 386 species, "
             <<rubyEntries<<"/"<<frlgEntries<<" offers, "
             <<differingSpecies<<" source-distinct species: PASS\n";
}
