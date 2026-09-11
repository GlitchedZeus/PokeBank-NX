#include "Integration/Gen2/Gen2ReadOnlySave.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace PokeVault::Integration::Gen2;

namespace {
struct L {
    RegionLayout region; VersionFamily family; size_t money, idx, names, party, current, end, sum, gender;
    uint8_t cap, boxes, str, split;
};
constexpr size_t N = static_cast<size_t>(-1);
constexpr L IGS{RegionLayout::International,VersionFamily::GoldSilver,0x23DB,0x2724,0x2727,0x288A,0x2D6C,0x2D68,0x2D69,N,20,14,11,7};
constexpr L IC {RegionLayout::International,VersionFamily::Crystal,0x23DC,0x2700,0x2703,0x2865,0x2D10,0x2B82,0x2D0D,0x3E3D,20,14,11,7};
constexpr L JGS{RegionLayout::Japanese,VersionFamily::GoldSilver,0x23BC,0x2705,0x2708,0x283E,0x2D10,0x2C8B,0x2D0D,N,30,9,6,6};
constexpr L JC {RegionLayout::Japanese,VersionFamily::Crystal,0x23BE,0x26E2,0x26E5,0x281A,0x2D10,0x2AE2,0x2D0D,0x8000,30,9,6,6};

size_t listLen(size_t cap,size_t body,size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
size_t stride(const L& l){return listLen(l.cap,32,l.str)+2;}
size_t boxStart(const L& l,size_t b){return b<l.split?0x4000+b*stride(l):0x6000+(b-l.split)*stride(l);}

void emptyList(std::vector<uint8_t>& d,size_t ofs,size_t){d[ofs]=0;d[ofs+1]=0xFF;}
void name(std::vector<uint8_t>& d,size_t ofs,size_t len,char c){
    std::fill_n(d.begin()+ofs,len,0x50);
    if(c>='A'&&c<='Z')d[ofs]=static_cast<uint8_t>(0x80+c-'A');
}
void pokemonList(std::vector<uint8_t>& d,size_t ofs,size_t cap,size_t bodySize,size_t str,uint8_t species,bool shiny,bool party){
    d[ofs]=1;d[ofs+1]=species;d[ofs+2]=0xFF;
    size_t body=ofs+1+(cap+1);
    d[body+0]=species;d[body+1]=0x01;
    d[body+2]=33;d[body+3]=45;d[body+4]=0;d[body+5]=0;
    d[body+6]=0x12;d[body+7]=0x34;
    d[body+8]=0;d[body+9]=0x10;d[body+10]=0;
    for(int i=0;i<5;i++){d[body+11+2*i]=0;d[body+12+2*i]=static_cast<uint8_t>(i+1);}
    if(shiny){d[body+21]=0x7A;d[body+22]=0xAA;}else{d[body+21]=0x98;d[body+22]=0x76;}
    d[body+23]=10|0x40;d[body+24]=20|0x80;d[body+25]=0;d[body+26]=0;
    d[body+27]=123;d[body+28]=0x21;d[body+29]=0;d[body+30]=0;d[body+31]=15;
    if(party){
        d[body+32]=0x08;d[body+33]=0;
        auto be=[&](size_t o,uint16_t v){d[body+o]=v>>8;d[body+o+1]=v&0xFF;};
        be(34,35);be(36,40);be(38,30);be(40,20);be(42,25);be(44,26);be(46,27);
    }
    size_t ot=body+cap*bodySize;
    size_t nick=ot+cap*str;
    name(d,ot,str,'O');name(d,nick,str,'M');
}
void putChecksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(size_t i=0x2009;i<=l.end;i++)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=s&0xFF;d[l.sum+1]=s>>8;
}
std::vector<uint8_t> fixture(const L& l,uint8_t species=25,bool shiny=true){
    size_t sz=(l.gender==0x8000)?0x10000:0x8000;
    std::vector<uint8_t>d(sz);
    d[0x2009]=0x12;d[0x200A]=0x34;name(d,0x200B,l.str,'A');
    d[l.money]=0x01;d[l.money+1]=0xE2;d[l.money+2]=0x40;
    d[l.idx]=0;if(l.gender!=N)d[l.gender]=1;
    for(size_t b=0;b<l.boxes;b++){emptyList(d,boxStart(l,b),l.cap);name(d,l.names+b*9,9,static_cast<char>('A'+(b%26)));}
    pokemonList(d,boxStart(l,0),l.cap,32,l.str,species,shiny,false);
    emptyList(d,l.current,l.cap); // deliberately stale current-box copy; stored bank is authoritative.
    pokemonList(d,l.party,6,48,l.str,species,shiny,true);
    putChecksum(d,l);return d;
}
}

int main(){
    {
        auto d=fixture(IGS);const auto before=d;auto r=parse(d,SourceGame::Gold);assert(r);
        assert(r.save->metadata().family==VersionFamily::GoldSilver);
        assert(r.save->metadata().region==RegionLayout::International);
        assert(r.save->metadata().sourceGameId=="gold_gbc");
        assert(r.save->trainer().trainerId==0x1234);assert(r.save->trainer().money==123456);
        assert(r.save->party().size()==1);const auto&p=r.save->party()[0];
        assert(p.species==25&&p.heldItem==1&&p.friendship==123);
        assert(p.dvs[1]==7&&p.dvs[2]==10&&p.dvs[3]==10&&p.dvs[4]==10&&p.shiny);
        assert(p.pp[0]==10&&p.ppUps[0]==1&&p.pp[1]==20&&p.ppUps[1]==2);
        assert(p.partyRecord&&p.currentHP==35&&p.maxHP==40&&p.specialDefense==27);
        assert(r.save->boxes().size()==14);
        assert(r.save->boxes()[0].slots[0]&&r.save->boxes()[0].slots[0]->species==25);
        assert(d==before);
        auto silver=parse(d,SourceGame::Silver);assert(silver&&silver.save->metadata().sourceGameId=="silver_gbc");
        auto wrong=parse(d,SourceGame::Crystal);assert(!wrong&&wrong.error==SaveError::GameHintMismatch);
    }
    {
        auto d=fixture(IC);auto r=parse(d,SourceGame::Crystal);assert(r&&r.save->metadata().family==VersionFamily::Crystal);
        assert(r.save->trainer().gender&&*r.save->trainer().gender==1);assert(r.save->metadata().payloadSize==0x8000);
    }
    {
        auto d=fixture(JGS);auto r=parse(d,SourceGame::Gold);assert(r&&r.save->metadata().region==RegionLayout::Japanese);
        assert(r.save->metadata().boxCount==9&&r.save->metadata().boxCapacity==30);
    }
    {
        auto d=fixture(JC,201,false);auto r=parse(d,SourceGame::Crystal);assert(r&&r.save->metadata().payloadSize==0x10000);
        assert(r.save->party()[0].species==201);
    }
    {
        auto d=fixture(IGS);for(int i=0;i<7;i++)d.push_back(0);auto r=parse(d,SourceGame::Gold);
        assert(r&&r.save->metadata().rtcFooterSize==7&&r.save->sourceBytes().size()==0x8007);
    }
    {
        auto d=fixture(IGS);d[0x2100]^=1;auto r=parse(d,SourceGame::Gold);assert(!r&&r.error==SaveError::ChecksumMismatch);
    }
    {
        auto d=fixture(IGS);d[IGS.party]=7;putChecksum(d,IGS);auto r=parse(d,SourceGame::Gold);
        assert(!r&&r.error==SaveError::InvalidStructure);
    }
    {
        std::vector<uint8_t>d(123);auto r=parse(d,SourceGame::Gold);assert(!r&&r.error==SaveError::WrongSize);
    }
    std::cout<<"Gen II strict read-only parser focused tests: PASS\n";
}
