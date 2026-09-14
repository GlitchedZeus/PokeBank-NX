#pragma once
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Pokemon/Experience.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen2;

namespace {
struct L {
    VersionFamily family;
    size_t money, idx, names, party, current, end, sum, sum2, gender;
    size_t tm, items, key, balls, pc;
};
constexpr size_t N = static_cast<size_t>(-1);
constexpr L GS{VersionFamily::GoldSilver,0x23DB,0x2724,0x2727,0x288A,0x2D6C,0x2D68,0x2D69,0x7E6D,N,
               0x23E6,0x241F,0x2449,0x2464,0x247E};
constexpr L C {VersionFamily::Crystal,0x23DC,0x2700,0x2703,0x2865,0x2D10,0x2B82,0x2D0D,0x1F0D,0x3E3D,
               0x23E7,0x2420,0x244A,0x2465,0x247F};

inline size_t listLen(size_t cap,size_t body,size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
inline size_t stride(){return listLen(20,32,11)+2;}
inline size_t boxStart(size_t b){return b<7?0x4000+b*stride():0x6000+(b-7)*stride();}
inline size_t boxBody(size_t b){return boxStart(b)+1+21;}
inline size_t boxOT(size_t b){return boxBody(b)+20*32;}
inline size_t boxNick(size_t b){return boxOT(b)+20*11;}

inline uint8_t encodeChar(char c){
    if(c>='A'&&c<='Z')return static_cast<uint8_t>(0x80+c-'A');
    if(c>='a'&&c<='z')return static_cast<uint8_t>(0xA0+c-'a');
    if(c>='0'&&c<='9')return static_cast<uint8_t>(0xF6+c-'0');
    if(c==' ')return 0x7F;
    if(c=='-')return 0xE3;
    if(c=='!')return 0xE7;
    return 0xE6;
}
inline void text(std::vector<uint8_t>& d,size_t ofs,size_t len,const std::string& s){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(ofs),len,0x50);
    for(size_t i=0;i<std::min(len,s.size());++i)d[ofs+i]=encodeChar(s[i]);
}
inline void name(std::vector<uint8_t>& d,size_t ofs,size_t len,char c){text(d,ofs,len,std::string(1,c));}
inline void emptyList(std::vector<uint8_t>& d,size_t ofs){d[ofs]=0;d[ofs+1]=0xFF;}
inline void be16(std::vector<uint8_t>& d,size_t o,uint16_t v){d[o]=static_cast<uint8_t>(v>>8);d[o+1]=static_cast<uint8_t>(v);}
inline void be24(std::vector<uint8_t>& d,size_t o,uint32_t v){d[o]=static_cast<uint8_t>(v>>16);d[o+1]=static_cast<uint8_t>(v>>8);d[o+2]=static_cast<uint8_t>(v);}

inline void writeStoredPokemon(std::vector<uint8_t>& d,size_t box,size_t slot,uint8_t species,
                        const std::string& ot,const std::string& nick,uint8_t atk=7,
                        uint8_t def=10,uint8_t spe=10,uint8_t spc=10,uint8_t level=15){
    const size_t b=boxBody(box)+slot*32;
    d[b]=species;d[b+1]=1;
    d[b+2]=33;d[b+3]=45;d[b+4]=0;d[b+5]=0;
    be16(d,b+6,0x1234);be24(d,b+8,3375);
    for(int i=0;i<5;i++)be16(d,b+11+2*i,static_cast<uint16_t>(i+1));
    d[b+21]=static_cast<uint8_t>((atk<<4)|def);d[b+22]=static_cast<uint8_t>((spe<<4)|spc);
    d[b+23]=35;d[b+24]=40;d[b+25]=0;d[b+26]=0;
    d[b+27]=123;d[b+28]=0;be16(d,b+29,0);d[b+31]=level;
    text(d,boxOT(box)+slot*11,11,ot);text(d,boxNick(box)+slot*11,11,nick);
}
inline void setBoxCount(std::vector<uint8_t>& d,size_t box,size_t count,uint8_t species=25){
    const size_t s=boxStart(box);d[s]=static_cast<uint8_t>(count);
    for(size_t i=0;i<20;i++)d[s+1+i]=(i<count)?species:0;
    d[s+1+count]=0xFF;
}
inline void onePokemonBox(std::vector<uint8_t>& d,size_t box,const std::string& nick="MON"){
    setBoxCount(d,box,1,25);writeStoredPokemon(d,box,0,25,"OT",nick);
}
inline void fullBox(std::vector<uint8_t>& d,size_t box){
    setBoxCount(d,box,20,25);
    for(size_t i=0;i<20;i++)writeStoredPokemon(d,box,i,25,"OT","FULL");
}
inline void partyList(std::vector<uint8_t>& d,size_t ofs){
    const size_t cap=6,str=11,bodySize=48;
    d[ofs]=1;d[ofs+1]=25;d[ofs+2]=0xFF;
    const size_t body=ofs+1+(cap+1);d[body]=25;d[body+1]=1;d[body+2]=33;
    be16(d,body+6,0x1234);be24(d,body+8,3375);
    d[body+21]=0x7A;d[body+22]=0xAA;d[body+23]=35;d[body+27]=123;d[body+31]=15;
    const size_t ot=body+cap*bodySize,nick=ot+cap*str;text(d,ot,str,"OT");text(d,nick,str,"PARTY");
}
inline void pairPocket(std::vector<uint8_t>& d,size_t ofs,std::initializer_list<std::pair<uint8_t,uint8_t>> entries){
    d[ofs]=static_cast<uint8_t>(entries.size());size_t p=ofs+1;
    for(const auto& [id,qty]:entries){d[p++]=id;d[p++]=qty;}d[p]=0xFF;
}
inline void inventory(std::vector<uint8_t>& d,const L& l){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(l.tm),57,0);
    pairPocket(d,l.items,{{kPotionItemId,2}});d[l.key]=0;d[l.key+1]=0xFF;
    pairPocket(d,l.balls,{{kPokeBallItemId,5}});d[l.pc]=0;d[l.pc+1]=0xFF;
}
inline void checksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(size_t i=0x2009;i<=l.end;i++)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=static_cast<uint8_t>(s);d[l.sum+1]=static_cast<uint8_t>(s>>8);
}
inline std::vector<uint8_t> fixture(const L& l,bool rtc=false){
    std::vector<uint8_t>d(0x8000+(rtc?7:0));
    d[0x2009]=0x12;d[0x200A]=0x34;text(d,0x200B,11,"ASH");
    d[l.money]=0x01;d[l.money+1]=0xE2;d[l.money+2]=0x40;d[l.idx]=0;
    if(l.gender!=N)d[l.gender]=1;
    for(size_t b=0;b<14;b++){emptyList(d,boxStart(b));name(d,l.names+b*9,9,static_cast<char>('A'+b));}
    onePokemonBox(d,0,"PIKA");onePokemonBox(d,1,"PIKA2");
    emptyList(d,l.current);partyList(d,l.party);inventory(d,l);checksum(d,l);
    if(rtc)for(size_t i=0;i<7;i++)d[0x8000+i]=static_cast<uint8_t>(0xA0+i);
    return d;
}
inline uint16_t le16(const std::vector<uint8_t>& d,size_t o){return static_cast<uint16_t>(d[o]|(d[o+1]<<8));}
inline bool differsOnlyInRange(const std::vector<uint8_t>& a,const std::vector<uint8_t>& b,size_t begin,size_t end){
    assert(a.size()==b.size());for(size_t i=0;i<a.size();++i)if(a[i]!=b[i]&&(i<begin||i>=end))return false;return true;
}

}
