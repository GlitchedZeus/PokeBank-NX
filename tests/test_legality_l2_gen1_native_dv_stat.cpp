#include "Legality/Gen1NativeDVStatEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen1NativeDVStat;
namespace G=PokeVault::Integration::Gen1;
static void be16(std::array<std::uint8_t,44>& b,unsigned o,std::uint16_t n){
    b[o]=static_cast<std::uint8_t>(n>>8);
    b[o+1]=static_cast<std::uint8_t>(n);
}
static G::PokemonRecord specimen(bool party,std::uint8_t level,
       std::array<std::uint8_t,4> stored={8,8,8,8},
       std::array<std::uint16_t,5> exp={}) {
    G::PokemonRecord r{};
    r.partyRecord=party;r.rawBodySize=party?44:33;
    r.rawSpecies=0x99;r.species=1;r.level=level;
    r.trainerId=0x1234;r.experience=1000;
    r.dvs={L::hpDV(stored),stored[0],stored[1],stored[2],stored[3]};
    r.statExperience=exp;
    auto& b=r.rawBody;
    b[0]=r.rawSpecies;b[3]=level;b[4]=r.status;b[5]=r.type1;
    b[6]=r.type2;b[7]=r.catchRate;
    be16(b,0x0C,r.trainerId);
    b[0x0E]=0;b[0x0F]=3;b[0x10]=0xE8;
    for(unsigned i=0;i<5;++i)be16(b,0x11+2*i,exp[i]);
    be16(b,0x1B,L::packedDVs(stored));
    if(party) {
        b[0x21]=level;
        const auto stats=G::StagedPokemonEditor::calculateBattleStats(1,level,stored,exp);
        r.maxHP=stats.hp;r.attack=stats.attack;r.defense=stats.defense;
        r.speed=stats.speed;r.special=stats.special;
        const auto values=stats.asArray();
        for(unsigned i=0;i<5;++i)be16(b,0x22+2*i,values[i]);
        r.currentHP=r.maxHP;
    }
    be16(b,1,r.currentHP);
    return r;
}
int main(){
    std::uint64_t verified=0;
    const L::SourceContext trusted{true,false,false,true};
    const L::SourceContext fresh{true,true,true,true};
    const auto sample=specimen(false,5);
    auto sampleResult=L::evaluate(sample,trusted);
    assert(sampleResult.status==L::Status::ConditionalNativeFields);
    assert((sampleResult.calculated.asArray()==std::array<std::uint16_t,5>{19,10,10,10,12}));
    verified+=2;
    // All 65,536 four-DV bit patterns; HP is *derived*, never stored.
    for(unsigned packed=0;packed<=0xFFFF;++packed){
        const auto dvs=L::unpackDVs(static_cast<std::uint16_t>(packed));
        assert(L::packedDVs(dvs)==packed);++verified;
        const auto hp=L::hpDV(dvs);
        assert(hp<=15);++verified;
        auto r=specimen(false,static_cast<std::uint8_t>(1+(packed%100)),dvs);
        const auto result=L::evaluate(r,trusted);
        assert(result.status==L::Status::ConditionalNativeFields);
        assert(result.derivedHPDV==hp);
        assert(result.storedDVs==dvs);
        verified+=3;
    }
    // Reuse the real PK1 species index and Gen I personal/battle-stat
    // tables across ALL 151 native species, never a modern base-stat table.
    unsigned nativeSpecies=0;
    for(unsigned raw=1;raw<=255;++raw){
        const auto species=G::gen1InternalToNational(static_cast<std::uint8_t>(raw));
        if(!species || species>151)continue;
        ++nativeSpecies;
        for(auto level:{1u,5u,50u,100u}){
            auto one=specimen(false,static_cast<std::uint8_t>(level),
                              {15,0,15,0},{65535,0,65535,0,65535});
            one.rawSpecies=one.rawBody[0]=static_cast<std::uint8_t>(raw);
            one.species=species;
            const auto result=L::evaluate(one,trusted);
            assert(result.status==L::Status::ConditionalNativeFields);
            assert(result.calculated.hp!=0);
            verified+=2;
        }
    }
    assert(nativeSpecies==151);++verified;
    // Every one of the 16 possible HP DV values arises from low-bit flags.
    for(unsigned h=0;h<16;++h){
        std::array<std::uint8_t,4> d{
            static_cast<std::uint8_t>((h>>3)&1),
            static_cast<std::uint8_t>((h>>2)&1),
            static_cast<std::uint8_t>((h>>1)&1),
            static_cast<std::uint8_t>(h&1)};
        assert(L::hpDV(d)==h);++verified;
    }
    // Five independent Gen I 16-bit Stat Experience stores, not 6 EV slots.
    for(auto v:{0u,1u,15u,16u,255u,256u,65025u,65026u,65535u})
      for(unsigned idx=0;idx<5;++idx){
        std::array<std::uint16_t,5> exp{};
        exp[idx]=static_cast<std::uint16_t>(v);
        auto r=specimen(false,100,{15,15,15,15},exp);
        const auto result=L::evaluate(r,trusted);
        assert(result.status==L::Status::ConditionalNativeFields);
        assert(result.statExperience[idx]==v);
        assert(result.calculated.hp!=0);
        verified+=3;
      }
    for(unsigned level=1;level<=100;++level){
        const auto r=specimen(true,static_cast<std::uint8_t>(level),{15,15,15,15},
                              {65535,65535,65535,65535,65535});
        const auto result=L::evaluate(r,fresh);
        assert(result.status==L::Status::ConditionalPartyStatMatch);
        assert(result.comparedCachedPartyStats);verified+=2;
        const auto uncertain=L::evaluate(r,trusted);
        assert(uncertain.status==L::Status::UnresolvedPartyCache);
        assert(!uncertain.comparedCachedPartyStats);verified+=2;
    }
    // Level zero stays unknown, not automatically an impossible historical save.
    auto zero=specimen(false,0);
    assert(L::evaluate(zero,trusted).status==L::Status::UnknownLevelContext);
    auto high=specimen(false,101);
    assert(L::evaluate(high,trusted).status==L::Status::InvalidNativeRecord);
    auto unsure=specimen(false,5);
    assert(L::evaluate(unsure,{}).status==L::Status::UnknownSource);
    assert(L::evaluate(unsure,{true,false,false,false}).status==L::Status::UnknownLevelContext);
    verified+=4;
    std::cout<<"L2 Gen I native DVs and Stat Experience: "<<verified<<" PASS\n";
    return 0;
}
