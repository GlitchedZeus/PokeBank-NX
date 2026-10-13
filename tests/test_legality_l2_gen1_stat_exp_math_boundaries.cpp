#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace G=PokeVault::Integration::Gen1;
namespace Ref {
#include "../src/Integration/Gen1/Gen1PersonalData.inc"
}
static std::uint32_t integerCeilSqrt255(std::uint16_t exp) noexcept {
    std::uint32_t root=0;
    while(root<255 && root*root<exp)++root;
    return root;
}
static std::uint16_t independentStat(unsigned base,unsigned dv,
     std::uint16_t exp,unsigned level,bool hp) noexcept {
    const auto effort=integerCeilSqrt255(exp)/4u;
    const auto scaled=(((base+dv)*2u+effort)*level)/100u;
    return static_cast<std::uint16_t>(scaled+(hp?level+10u:5u));
}
int main(){
    std::uint64_t assertions=0;
    // Retail integer square-root step boundaries: 9->10 is 0->1;
    // 63001->63002 is 62->63 (maximum useful effort).
    for(auto [v,step]:std::array<std::pair<std::uint16_t,unsigned>,11>{{
       {0,0},{1,0},{9,0},{10,1},{15,1},{16,1},
       {63001,62},{63002,63},{65025,63},{65026,63},{65535,63}}}){
        assert(integerCeilSqrt255(v)/4u==step);++assertions;
    }
    unsigned count=0;
    for(unsigned raw=1;raw<256;++raw){
        const auto species=G::gen1InternalToNational(static_cast<std::uint8_t>(raw));
        if(species<1||species>151)continue;
        ++count;
        const auto& base=Ref::kPersonal[species];
        for(auto lvl:{1u,5u,50u,100u})
        for(auto dv:{0u,1u,8u,15u})
        for(auto xp:{0u,1u,9u,10u,16u,63001u,63002u,65025u,65026u,65535u}){
            const auto level=static_cast<std::uint8_t>(lvl);
            const std::array<std::uint8_t,4> dvs{
                static_cast<std::uint8_t>(dv),static_cast<std::uint8_t>(dv),
                static_cast<std::uint8_t>(dv),static_cast<std::uint8_t>(dv)};
            std::array<std::uint16_t,5> exp{};
            exp.fill(static_cast<std::uint16_t>(xp));
            const auto observed=G::StagedPokemonEditor::calculateBattleStats(
                species,level,dvs,exp).asArray();
            const auto hpDV=G::StagedPokemonEditor::derivedHPDV(dvs);
            const std::array<std::uint16_t,5> expected{
                independentStat(base[0],hpDV,exp[0],lvl,true),
                independentStat(base[1],dv,exp[1],lvl,false),
                independentStat(base[2],dv,exp[2],lvl,false),
                independentStat(base[3],dv,exp[3],lvl,false),
                independentStat(base[4],dv,exp[4],lvl,false)};
            assert(observed==expected);++assertions;
            assert(observed[0]>=lvl+10u);
            assert(observed[4]>0);assertions+=2;
        }
    }
    assert(count==151);++assertions;
    // Test *every* 16-bit Stat Experience value through production
    // five-stat math, including 63002 and 65535 cap behavior.
    std::array<std::uint16_t,5> exp{};
    for(unsigned x=0;x<65536;++x){
        exp.fill(static_cast<std::uint16_t>(x));
        const auto observed=G::StagedPokemonEditor::calculateBattleStats(
            25,100,{15,15,15,15},exp);
        const auto expected=independentStat(35,15,exp[0],100,true);
        assert(observed.hp==expected);++assertions;
        assert(observed.special==independentStat(50,15,exp[4],100,false));
        ++assertions;
    }
    // Invalid species, levels, and 4-bit DV overflow fail closed.
    const auto zero=std::array<std::uint16_t,5>{0,0,0,0,0};
    assert((G::StagedPokemonEditor::calculateBattleStats(0,50,{0,0,0,0},zero).asArray()==zero));++assertions;
    assert((G::StagedPokemonEditor::calculateBattleStats(152,50,{0,0,0,0},zero).asArray()==zero));++assertions;
    assert((G::StagedPokemonEditor::calculateBattleStats(25,0,{0,0,0,0},zero).asArray()==zero));++assertions;
    assert((G::StagedPokemonEditor::calculateBattleStats(25,101,{0,0,0,0},zero).asArray()==zero));++assertions;
    assert((G::StagedPokemonEditor::calculateBattleStats(25,50,{16,0,0,0},zero).asArray()==zero));++assertions;
    std::cout<<"Gen I production battle-stat full-species/integer-root boundaries: "
             <<assertions<<" assertions PASS\n";
}
