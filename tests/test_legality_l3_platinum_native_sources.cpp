#include "Legality/Gen4PlatinumNativeMethodData.h"
#include "Legality/Gen4PlatinumPinnedMachines.h"
#include <cassert>
#include <iostream>
namespace L=Legality::Gen4PlatinumNativeMethodData;
namespace M=Legality::Gen4PlatinumPinnedMachines;
static_assert(L::kRows.size()==12);
static_assert(L::kOffers.size()==174);
static_assert(L::kTutors.size()==100);
static_assert(M::kPinned.size()==12);
static_assert(L::offerCount(1)==14 && L::offerCount(6)==18);
static_assert((L::packedOffer(1,8)>>9)==21);
static_assert((L::packedOffer(1,8)&511u)==230);
static_assert(L::row(386)!=nullptr && L::row(152)==nullptr);
static_assert(M::exactMove(false,53,412));
static_assert(M::exactMove(true,2,19));
static_assert(!M::known(false,1));
static_assert(M::compatible(1,false,53,412));
static_assert(!M::compatible(1,true,2,19));
int main(){
    uint32_t offers=0,tutors=0,compat=0;
    for(const auto& species:L::kRows){
        assert(species.species>=1 && species.species<=386);
        uint8_t previous=0;
        for(uint16_t i=0;i<L::offerCount(species.species);++i){
            const auto packed=L::packedOffer(species.species,i);
            const auto level=packed>>9,move=packed&511u;
            assert(level>=1 && level<=100 && level>=previous);
            assert(move>=1 && move<=467);
            previous=static_cast<uint8_t>(level);++offers;
        }
        tutors+=species.tutorCount;
        for(uint8_t n=1;n<=92;++n)if(L::machineCompatible(species.species,false,n))++compat;
        for(uint8_t n=1;n<=8;++n)if(L::machineCompatible(species.species,true,n))++compat;
    }
    assert(offers==174 && tutors==100 && compat==429);
    uint8_t identified=0;
    for(const auto& item:M::kPinned){
        assert(M::known(item.hidden,item.number));
        assert(M::exactMove(item.hidden,item.number,item.move));
        assert(item.move>=1&&item.move<=467);
        ++identified;
    }
    assert(identified==12);
    std::cout<<"L3 pinned native Platinum: 12 species, 174 offers, 429 machines, 100 tutor options: PASS\n";
}
