#ifndef POKEBANK_GEN5_PROGRESSION_H
#define POKEBANK_GEN5_PROGRESSION_H

#include "Integration/Gen5/Gen5PersonalData.h"
#include "Encryption/Encryption5.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Generation III-V experience groups as indexed by the pinned BW/B2W2
// personal records: 0 Medium Fast, 1 Erratic, 2 Fluctuating,
// 3 Medium Slow, 4 Fast, 5 Slow. Integer floors match the game's tables.
[[nodiscard]] constexpr uint32_t experienceForLevel5(uint8_t growth,
                                                       uint32_t level) noexcept {
    if(level<=1)return 0;
    if(level>100 || growth>5)return 0;
    const int64_t l=level, cube=l*l*l;
    switch(growth) {
        case 0:return static_cast<uint32_t>(cube);
        case 1:
            if(l<=50)return static_cast<uint32_t>(cube*(100-l)/50);
            if(l<=68)return static_cast<uint32_t>(cube*(150-l)/100);
            if(l<=98)return static_cast<uint32_t>(cube*((1911-10*l)/3)/500);
            return static_cast<uint32_t>(cube*(160-l)/100);
        case 2:
            if(l<=15)return static_cast<uint32_t>(cube*(((l+1)/3)+24)/50);
            if(l<=36)return static_cast<uint32_t>(cube*(l+14)/50);
            return static_cast<uint32_t>(cube*((l/2)+32)/50);
        case 3:return static_cast<uint32_t>(
            std::max<int64_t>(0,(6*cube)/5-15*l*l+100*l-140));
        case 4:return static_cast<uint32_t>(4*cube/5);
        case 5:return static_cast<uint32_t>(5*cube/4);
        default:return 0;
    }
}
[[nodiscard]] constexpr uint8_t levelForExperience5(uint8_t growth,
                                                      uint32_t exp) noexcept {
    if(growth>5)return 0;
    for(uint32_t level=100;level>=2;--level)
        if(exp>=experienceForLevel5(growth,level))
            return static_cast<uint8_t>(level);
    return 1;
}

// PKHeX PK5.cs: Level 0x8C, current HP 0x8E, max HP 0x90,
// Attack/Defense/Speed/Sp.Atk/Sp.Def 0x92..0x9A. This recalculates only
// the party battle stat cache, leaving status, mail and every other byte alone.
// Preserve existing HP damage (or fainted state), never heal on edit.
[[nodiscard]] inline bool regeneratePartyStats5(std::vector<uint8_t>& plain,
                                                 const std::vector<uint8_t>& original,
                                                 bool black2White2) {
    if(plain.size()!=Crypto::PartySize || original.size()!=Crypto::PartySize)
        return false;
    const uint16_t species=Crypto::read16(plain,0x08);
    const uint8_t form=static_cast<uint8_t>(plain[0x40]>>3);
    const uint8_t level=plain[0x8C],nature=plain[0x41];
    const auto personal=personal5(species,form,black2White2);
    if(!personal || level<1 || level>100 || nature>24 ||
       (Crypto::read32(plain,0x38)&0x40000000u))
        return false; // Unknown identity/level or egg: do not guess.
    const auto& p=*personal.info;
    const uint8_t base[6]={p.hp,p.atk,p.def,p.spe,p.spa,p.spd};
    const uint32_t iv=Crypto::read32(plain,0x38);
    uint16_t stats[6]{};
    for(size_t i=0;i<6;++i) {
        const uint32_t value=static_cast<uint32_t>(2*base[i])+
            ((iv>>(5*i))&31u)+(plain[0x18+i]/4u);
        uint32_t computed=value*level/100;
        if(i==0)
            computed=species==292?1:computed+level+10; // Shedinja
        else {
            computed+=5;
            const size_t natureIndex=i-1; // Atk, Def, Spe, SpA, SpD
            if(nature/5==natureIndex && nature%5!=natureIndex)
                computed=computed*110/100;
            else if(nature%5==natureIndex && nature/5!=natureIndex)
                computed=computed*90/100;
        }
        stats[i]=static_cast<uint16_t>(computed);
    }
    // Immutable baseline damage makes replay independent of draft edit order.
    const uint16_t oldHP=Crypto::read16(original,0x8E);
    const uint16_t oldMax=Crypto::read16(original,0x90);
    const uint16_t damage=oldMax>oldHP?oldMax-oldHP:0;
    const uint16_t hp=oldHP==0?0:static_cast<uint16_t>(
        std::max<int>(1,static_cast<int>(stats[0])-damage));
    Crypto::write16(plain,0x8E,hp);
    for(size_t i=0;i<6;++i)
        Crypto::write16(plain,0x90+2*i,stats[i]);
    return true;
}
static_assert(experienceForLevel5(0,100)==1000000);
static_assert(experienceForLevel5(1,100)==600000);
static_assert(experienceForLevel5(2,100)==1640000);
static_assert(experienceForLevel5(3,100)==1059860);
static_assert(experienceForLevel5(4,100)==800000);
static_assert(experienceForLevel5(5,100)==1250000);
static_assert(experienceForLevel5(1,50)==125000);
static_assert(experienceForLevel5(3,2)==9);
static_assert(levelForExperience5(0,999999)==99);
static_assert(levelForExperience5(0,1000000)==100);

} // namespace PokeVault::Integration::Gen5
#endif
