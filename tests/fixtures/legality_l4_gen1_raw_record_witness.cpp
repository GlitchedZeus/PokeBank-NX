#include "Legality/Gen1NativeRawRecordWitness.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
namespace W=Legality::Gen1NativeRawRecordWitness;
struct Native {
    uint8_t rawSpecies=84,status=0,type1=1,type2=1,catchRate=45,level=20;
    uint16_t currentHP=99,trainerId=17389,maxHP=120,attack=55,defense=40,speed=70,special=60;
    uint32_t experience=8000;
    std::array<uint8_t,4> moves{1,2,3,0},pp{35,20,15,0},ppUps{0,1,2,3};
    std::array<uint8_t,5> dvs{10,15,10,7,12};
    std::array<uint16_t,5> statExperience{1,2,3,4,5};
    bool partyRecord=true;
    std::array<uint8_t,44> rawBody{};
    std::size_t rawBodySize=44;
};
void be16(std::array<uint8_t,44>& b,size_t pos,uint16_t v) {
    b[pos]=uint8_t(v>>8);b[pos+1]=uint8_t(v);
}
Native specimen(bool party){
    Native v{};v.partyRecord=party;v.rawBodySize=party?44:33;
    auto& b=v.rawBody;
    b[0]=v.rawSpecies;be16(b,1,v.currentHP);b[4]=v.status;
    b[5]=v.type1;b[6]=v.type2;b[7]=v.catchRate;
    for(size_t i=0;i<4;++i)b[8+i]=v.moves[i];
    be16(b,12,v.trainerId);b[14]=uint8_t(v.experience>>16);
    b[15]=uint8_t(v.experience>>8);b[16]=uint8_t(v.experience);
    for(size_t i=0;i<5;++i)be16(b,0x11+i*2,v.statExperience[i]);
    be16(b,0x1b,0xfa7c); // ATK=F DEF=A SPE=7 SPC=C => HP DV=10
    for(size_t i=0;i<4;++i)b[0x1d+i]=uint8_t((v.ppUps[i]<<6)|v.pp[i]);
    b[party?0x21:3]=v.level;
    if(party){be16(b,0x22,v.maxHP);be16(b,0x24,v.attack);
        be16(b,0x26,v.defense);be16(b,0x28,v.speed);be16(b,0x2a,v.special);}
    return v;
}
int main(){
    using S=W::Status;
    for(bool party:{false,true}) {
        auto valid=specimen(party);
        assert(W::evaluate(valid).status==S::Consistent);
        for(unsigned raw=0;raw<(party?44u:33u);++raw)
        for(unsigned bit=0;bit<8;++bit) {
            auto n=valid;n.rawBody[raw]^=uint8_t(1u<<bit);
            auto r=W::evaluate(n);
            // The stored-party-only fields must always be checked; otherwise
            // every bit in a decoded canonical field invalidates consistency.
            const bool unobserved=(raw==3 && party);
            assert((r.status==S::Consistent)==unobserved);
            assert(!r.globallyInvalid && !r.originalSourceAuthenticated);
        }
        for(unsigned length=0;length<=50;++length){
            auto n=valid;n.rawBodySize=length;
            assert(W::evaluate(n).status==(length==(party?44u:33u)?S::Consistent:S::UnknownBody));
        }
        auto x=valid;x.dvs[0]^=1;
        assert((W::evaluate(x).contradicted&W::DVs)!=0);
        x=valid;x.ppUps[1]=4;
        assert((W::evaluate(x).contradicted&W::PPAndUps)!=0);
        x=valid;x.trainerId++;
        assert((W::evaluate(x).contradicted&W::TrainerId)!=0);
    }
    unsigned pairs=0;
    for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b){
        std::array<uint8_t,11> ot{};ot.fill(0x50);ot[0]=uint8_t(a);
        auto original=ot;
        std::array<uint8_t,11> nick{};nick.fill(0x50);nick[0]=uint8_t(b);
        assert(W::compareRawListNames(ot,nick,original,nick,true)==W::NameStatus::BytesMatch);
        original[0]^=1;
        assert(W::compareRawListNames(ot,nick,original,nick,true)==W::NameStatus::Mismatch);
        assert(W::compareRawListNames(ot,nick,original,nick,false)==W::NameStatus::UnknownSourceBytes);
        ++pairs;
    }
    std::array<uint8_t,6> japanese{};
    assert(W::compareRawListNames(japanese,japanese,japanese,japanese,true)==W::NameStatus::BytesMatch);
    std::array<uint8_t,10> bad{};
    assert(W::compareRawListNames(bad,bad,bad,bad,true)==W::NameStatus::UnknownSourceBytes);
    std::cout<<"L4 Gen I raw PK1 party/box decode and name-source witness: "<<pairs<<" pair checks PASS\n";
}
