#include "Integration/Gen2/Gen2ReadOnlySave.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace PokeVault::Integration::Gen2;

namespace {
struct L { RegionLayout r; VersionFamily f; size_t size,trainer,money,boxIndex,boxNames,party,checksumEnd,checksum,checksum2,gender; int boxes,cap,str; size_t spacing; int first; };
constexpr L IGS{RegionLayout::International,VersionFamily::GoldSilver,0x8000,0x2009,0x23DB,0x2724,0x2727,0x288A,0x2D68,0x2D69,0x7E6D,0,14,20,11,0x450,7};
constexpr L IC {RegionLayout::International,VersionFamily::Crystal,0x8000,0x2009,0x23DC,0x2700,0x2703,0x2865,0x2B82,0x2D0D,0x1F0D,0x3E3D,14,20,11,0x450,7};
constexpr L JGS{RegionLayout::Japanese,VersionFamily::GoldSilver,0x10000,0x2009,0x23BC,0x2705,0x2708,0x283E,0x2C8B,0x2D0D,0x7F0D,0,9,30,6,0x54A,6};
constexpr L JC {RegionLayout::Japanese,VersionFamily::Crystal,0x10000,0x2009,0x23BE,0x26E2,0x26E5,0x281A,0x2AE2,0x2D0D,0x7F0D,0x8000,9,30,6,0x54A,6};
void be16(std::vector<uint8_t>& b,size_t o,uint16_t v){b[o]=v>>8;b[o+1]=v&0xFF;}
void be24(std::vector<uint8_t>& b,size_t o,uint32_t v){b[o]=v>>16;b[o+1]=(v>>8)&0xFF;b[o+2]=v&0xFF;}
void initList(std::vector<uint8_t>& b,size_t o,int cap){b[o]=0;for(int i=0;i<cap;i++)b[o+1+i]=0xFF;b[o+1+cap]=0xFF;}
void putText(std::vector<uint8_t>& b,size_t o,const char* s,size_t width){size_t i=0;for(;s[i]&&i<width;i++){char c=s[i];b[o+i]=(c>='A'&&c<='Z')?uint8_t(0x80+c-'A'):uint8_t(0xA0+c-'a');}if(i<width)b[o+i]=0x50;}
void finalize(std::vector<uint8_t>& b,const L& l){uint16_t sum=0;for(size_t i=0x2009;i<=l.checksumEnd;i++)sum=uint16_t(sum+b[i]);for(size_t o:{l.checksum,l.checksum2}){b[o]=sum&0xFF;b[o+1]=sum>>8;}}
std::vector<uint8_t> fixture(const L& l,bool partyMon=true){
 std::vector<uint8_t>b(l.size);putText(b,l.trainer+2,l.r==RegionLayout::Japanese?"A":"GOLD",l.r==RegionLayout::Japanese?5:7);be16(b,l.trainer,12345);be24(b,l.money,54321);b[l.boxIndex]=2;if(l.gender)b[l.gender]=1;
 initList(b,l.party,6);
 for(int box=0;box<l.boxes;box++){size_t base=box<l.first?0x4000:0x6000;int idx=box<l.first?box:box-l.first;initList(b,base+idx*l.spacing,l.cap);}
 if(partyMon){size_t o=l.party;b[o]=1;b[o+1]=25;for(int i=1;i<6;i++)b[o+1+i]=0xFF;b[o+7]=0xFF;size_t body=o+8;b[body]=25;b[body+1]=173;b[body+2]=33;be16(b,body+6,12345);b[body+8]=0;b[body+9]=0x01;b[body+10]=0x00;be16(b,body+0x0B,100);be16(b,body+0x0D,200);be16(b,body+0x0F,300);be16(b,body+0x11,400);be16(b,body+0x13,500);be16(b,body+0x15,0x2AAA);b[body+0x17]=10;b[body+0x1B]=70;b[body+0x1F]=5;be16(b,body+0x22,20);be16(b,body+0x24,20);be16(b,body+0x26,12);be16(b,body+0x28,10);be16(b,body+0x2A,15);be16(b,body+0x2C,11);be16(b,body+0x2E,11);size_t ot=body+48*6;size_t nick=ot+l.str*6;putText(b,ot,"OT",l.str);putText(b,nick,"PIKA",l.str);}
 finalize(b,l);return b;
}
void check(const L& l,SourceGame game,const char* id){auto raw=fixture(l);auto before=raw;auto p=parse(raw,game);assert(p);assert(p.save->metadata().sourceGameId==id);assert(p.save->metadata().region==l.r);assert(p.save->metadata().family==l.f);assert(p.save->metadata().payloadSize==l.size);assert(p.save->metadata().boxCount==l.boxes);assert(p.save->metadata().boxCapacity==l.cap);assert(p.save->metadata().currentBox==2);assert(p.save->trainer().trainerId==12345);assert(p.save->trainer().money==54321);assert(p.save->party().size()==1);auto&m=p.save->party()[0];assert(m.species==25&&m.heldItem==173&&m.trainerId==12345&&m.level==5);assert(m.dvs[1]==2&&m.dvs[2]==10&&m.dvs[3]==10&&m.dvs[4]==10&&m.shiny);assert(m.gender==1);assert(m.friendship==70);assert(m.sourceGame==game&&m.region==l.r);assert(p.save->boxes().size()==size_t(l.boxes));assert(raw==before);}
}
int main(){
 check(IGS,SourceGame::Gold,"gold_gbc");check(IGS,SourceGame::Silver,"silver_gbc");check(IC,SourceGame::Crystal,"crystal_gbc");check(JGS,SourceGame::Gold,"gold_gbc");check(JGS,SourceGame::Silver,"silver_gbc");check(JC,SourceGame::Crystal,"crystal_gbc");
 {auto b=fixture(IGS);auto p=parse(b,SourceGame::Crystal);assert(!p&&p.error==SaveError::GameHintMismatch);}
 {auto b=fixture(IC);auto p=parse(b,SourceGame::Gold);assert(!p&&p.error==SaveError::GameHintMismatch);}
 {auto b=fixture(IGS);b[0x2100]^=1;auto p=parse(b,SourceGame::Gold);assert(!p&&p.error==SaveError::ChecksumMismatch);}
 {auto b=fixture(IGS);b[IGS.checksum2]^=1;auto p=parse(b,SourceGame::Gold);assert(!p&&p.error==SaveError::ChecksumMismatch);}
 {auto b=fixture(IGS);b.resize(0x7000);auto p=parse(b,SourceGame::Gold);assert(!p&&p.error==SaveError::WrongSize);}
 {auto b=fixture(JGS);b.resize(0x8000);auto p=parse(b,SourceGame::Gold);assert(!p);}
 {auto b=fixture(JC);b.insert(b.end(),12,0xCC);auto p=parse(b,SourceGame::Crystal);assert(p&&p.save->metadata().rtcFooterSize==12&&p.save->sourceBytes().size()==0x1000C);}
 {auto b=fixture(JC);b.insert(b.end(),9,0);auto p=parse(b,SourceGame::Crystal);assert(!p&&p.error==SaveError::WrongSize);}
 {auto b=fixture(IGS);b[0x4000]=21;finalize(b,IGS);auto p=parse(b,SourceGame::Gold);assert(!p);}
 std::cout<<"GSC strict read-only adapter/oracle tests passed\n";
}
