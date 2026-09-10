#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Utils/StringHelpers.h"

using namespace PokeVault::Integration::Gen1;
using namespace PokeVault::Legacy;

namespace {
constexpr size_t MAIN=0x2598, MONEY=0x25F3, TID=0x2605, CURIDX=0x284C;
constexpr size_t PARTY=0x2F2C, CURBOX=0x30C0, SUM=0x3523, MAINLEN=0x0F8B;
constexpr size_t BOXSIZE=0x462, BANKSIZE=0x1A4C;
constexpr size_t CAP=20, BOXES=12, STR=11;

uint8_t diff8(std::span<const uint8_t> s) {
    uint8_t v=0xFF; for (uint8_t b:s) v=static_cast<uint8_t>(v-b); return v;
}
void be16(std::vector<uint8_t>& d,size_t o,uint16_t v){d[o]=v>>8;d[o+1]=v;}
void be24(std::vector<uint8_t>& d,size_t o,uint32_t v){d[o]=v>>16;d[o+1]=v>>8;d[o+2]=v;}
std::vector<uint8_t> text(const std::string& s) {
    std::vector<uint8_t> out(STR,0x50);
    for(size_t i=0;i<s.size()&&i<STR;++i){char c=s[i];assert(c>='A'&&c<='Z');out[i]=0x80+(c-'A');}
    return out;
}
size_t boxStart(size_t box){return box<6?0x4000+box*BOXSIZE:0x6000+(box-6)*BOXSIZE;}
void emptyList(std::vector<uint8_t>& d,size_t o,size_t cap){
    d[o]=0;std::fill(d.begin()+o+1,d.begin()+o+cap+2,0);d[o+1]=0xFF;
}
void body(std::vector<uint8_t>& d,size_t o,size_t n,uint8_t species,uint8_t level){
    std::fill(d.begin()+o,d.begin()+o+n,0);
    d[o]=species;be16(d,o+1,45);d[o+3]=level;d[o+4]=0x40;d[o+5]=0x14;d[o+6]=0x14;
    d[o+7]=45;d[o+8]=33;d[o+9]=45;be16(d,o+0x0C,0x1234);be24(d,o+0x0E,12345);
    be16(d,o+0x11,111);be16(d,o+0x13,222);be16(d,o+0x15,333);be16(d,o+0x17,444);be16(d,o+0x19,555);
    be16(d,o+0x1B,0xA987);d[o+0x1D]=0xC0|20;d[o+0x1E]=15;
    if(n==44){d[o+0x21]=level;be16(d,o+0x22,52);be16(d,o+0x24,31);be16(d,o+0x26,28);be16(d,o+0x28,35);be16(d,o+0x2A,30);}
}
void setEntry(std::vector<uint8_t>& d,size_t o,size_t cap,size_t n,size_t index,uint8_t species,uint8_t level,const std::string& nick){
    assert(index<cap);size_t bs=o+cap+2,os=bs+cap*n,ns=os+cap*STR;
    d[o]=static_cast<uint8_t>(std::max<size_t>(d[o],index+1));d[o+1+index]=species;d[o+1+d[o]]=0xFF;
    body(d,bs+index*n,n,species,level);auto ot=text("RED"),nk=text(nick);
    std::copy(ot.begin(),ot.end(),d.begin()+os+index*STR);std::copy(nk.begin(),nk.end(),d.begin()+ns+index*STR);
}
void checksums(std::vector<uint8_t>& d){
    for(int bank=0;bank<2;++bank){size_t start=bank?0x6000:0x4000,sum=start+BANKSIZE;
        for(size_t i=0;i<6;++i)d[sum+1+i]=diff8(std::span<const uint8_t>(d).subspan(start+i*BOXSIZE,BOXSIZE));
        d[sum]=diff8(std::span<const uint8_t>(d).subspan(start,BANKSIZE));}
    d[SUM]=diff8(std::span<const uint8_t>(d).subspan(MAIN,MAINLEN));
}
std::vector<uint8_t> fixture(){
    std::vector<uint8_t>d(0x8000,0);auto will=text("WILL");std::copy(will.begin(),will.end(),d.begin()+MAIN);
    d[MONEY]=0x12;d[MONEY+1]=0x34;d[MONEY+2]=0x56;be16(d,TID,0x1234);d[0x29C3]=0x99;d[CURIDX]=0x80|2;
    emptyList(d,PARTY,6);setEntry(d,PARTY,6,44,0,0x54,15,"SPARKY");
    emptyList(d,CURBOX,CAP);setEntry(d,CURBOX,CAP,33,0,0x99,8,"BULBA");
    for(size_t b=0;b<BOXES;++b)emptyList(d,boxStart(b),CAP);
    setEntry(d,boxStart(0),CAP,33,0,0xB0,12,"CHAR");
    for(size_t s=0;s<CAP;++s)setEntry(d,boxStart(BOXES-1),CAP,33,s,0x15,5+(s%20),"MEW");
    d[0x2ED5]=0xFF;checksums(d);return d;
}
}

int main(){
    auto raw=fixture();const auto before=raw;auto parsed=parse(raw,SourceGame::Red);assert(parsed);assert(raw==before);
    std::string error;auto trainer=RBYReadOnlyTrainer::create(*parsed.save,error);assert(trainer&&error.empty());
    assert(trainer->sourceGameId()=="red_gb");assert(!trainer->japaneseLayout());assert(trainer->getBoxCount()==12);assert(trainer->getSlotsPerBox()==20);
    assert(trainer->trainerName=="WILL");assert(trainer->money==123456);assert(trainer->TID16==0x1234);assert(trainer->SID16==0);assert(trainer->currentBox==2);
    assert(trainer->party.size()==1);auto* party=static_cast<Pokemon::Pokemon1ReadOnly*>(trainer->party[0].get());
    assert(party->speciesID()==25);assert(Utils::utf16ToUtf8(party->nickname())=="SPARKY");assert(Utils::utf16ToUtf8(party->otName())=="RED");
    assert(party->tid16()==0x1234);assert(party->level()==15);assert(party->exp()==12345);assert(party->move(0)==33);assert(party->movePP(0)==20);assert(party->movePPUps(0)==3);
    assert(party->dvATK()==10&&party->dvDEF()==9&&party->dvSPE()==8&&party->dvSpecial()==7);
    // DV16 0xA987 => low bits A/9/8/7 = 0/1/0/1, so Gen I's synthesized HP DV is 0b0101 = 5.
    assert(party->dvHP()==5);
    assert(party->statExpHP()==111&&party->statExpSpecial()==555);assert(party->statusByte()==0x40);assert(party->isPartyRecord());
    assert(party->statHPCurrent()==45&&party->statHPMax()==52&&party->statATK()==31&&party->statDEF()==28&&party->statSPE()==35&&party->gen1Special()==30);
    assert(party->heldItem()==0&&party->ability()==0&&party->nature()==0&&party->sid16()==0);
    assert(trainer->boxes[0][0]&&trainer->boxes[0][0]->speciesID()==4);
    assert(trainer->boxes[2][0]&&trainer->boxes[2][0]->speciesID()==1);
    assert(trainer->boxes[11][19]&&trainer->boxes[11][19]->speciesID()==151);
    assert(!trainer->boxes[1][0]);
    auto* boxed=static_cast<Pokemon::Pokemon1ReadOnly*>(trainer->boxes[2][0].get());assert(!boxed->isPartyRecord());assert(boxed->statHPMax()==0);
    auto cloned=party->clone();assert(cloned&&cloned->speciesID()==25&&Utils::utf16ToUtf8(cloned->nickname())=="SPARKY");
    std::cout<<"RBY Trainer/Party/Boxes/Pokemon read-only bridge tests passed\n";return 0;
}
