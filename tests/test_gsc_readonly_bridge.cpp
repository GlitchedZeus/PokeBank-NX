#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "Utils/StringHelpers.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen2;

namespace {
struct L {
    VersionFamily family; size_t trainer,money,boxIndex,boxNames,party,checksumEnd,checksum,checksum2,gender;
    size_t tm,item,key,ball,pc;
};
constexpr L GS{VersionFamily::GoldSilver,0x2009,0x23DB,0x2724,0x2727,0x288A,0x2D68,0x2D69,0x7E6D,0,
               0x23E6,0x241F,0x2449,0x2464,0x247E};
constexpr L C {VersionFamily::Crystal,0x2009,0x23DC,0x2700,0x2703,0x2865,0x2B82,0x2D0D,0x1F0D,0x3E3D,
               0x23E7,0x2420,0x244A,0x2465,0x247F};
void be16(std::vector<uint8_t>& b,size_t o,uint16_t v){b[o]=v>>8;b[o+1]=v&0xFF;}
void be24(std::vector<uint8_t>& b,size_t o,uint32_t v){b[o]=v>>16;b[o+1]=(v>>8)&0xFF;b[o+2]=v&0xFF;}
void text(std::vector<uint8_t>& b,size_t o,const char* s,size_t width){size_t i=0;for(;s[i]&&i<width;i++){char c=s[i];b[o+i]=(c>='A'&&c<='Z')?uint8_t(0x80+c-'A'):uint8_t(0xA0+c-'a');}if(i<width)b[o+i]=0x50;}
void emptyList(std::vector<uint8_t>&b,size_t o,size_t cap){b[o]=0;for(size_t i=0;i<cap;i++)b[o+1+i]=0xFF;b[o+1+cap]=0xFF;}
void emptyInventory(std::vector<uint8_t>&b,const L&l){b[l.item]=0;b[l.item+1]=0xFF;b[l.key]=0;b[l.key+1]=0xFF;b[l.ball]=0;b[l.ball+1]=0xFF;b[l.pc]=0;b[l.pc+1]=0xFF;}
void putBody(std::vector<uint8_t>&b,size_t body,bool party,uint8_t species,const char* nick,size_t ot,size_t no){
 b[body]=species;b[body+1]=173;b[body+2]=33;b[body+3]=45;be16(b,body+6,12345);be24(b,body+8,125);
 be16(b,body+0x0B,100);be16(b,body+0x0D,200);be16(b,body+0x0F,300);be16(b,body+0x11,400);be16(b,body+0x13,500);
 be16(b,body+0x15,0x2AAA);b[body+0x17]=10;b[body+0x18]=0x45;b[body+0x1B]=70;b[body+0x1C]=0x12;b[body+0x1F]=5;
 if(party){b[body+0x20]=0x08;be16(b,body+0x22,20);be16(b,body+0x24,20);be16(b,body+0x26,12);be16(b,body+0x28,10);be16(b,body+0x2A,15);be16(b,body+0x2C,11);be16(b,body+0x2E,11);}
 text(b,ot,"OT",11);text(b,no,nick,11);
}
void finalize(std::vector<uint8_t>&b,const L&l){uint16_t sum=0;for(size_t i=0x2009;i<=l.checksumEnd;i++)sum=uint16_t(sum+b[i]);for(size_t o:{l.checksum,l.checksum2}){b[o]=sum&0xFF;b[o+1]=sum>>8;}}
std::vector<uint8_t> fixture(const L&l,bool crystal){
 std::vector<uint8_t>b(0x8000);be16(b,l.trainer,12345);text(b,l.trainer+2,crystal?"KRIS":"GOLD",7);be24(b,l.money,54321);b[l.boxIndex]=0;if(l.gender)b[l.gender]=1;
 emptyInventory(b,l);b[l.tm]=1;b[l.item]=1;b[l.item+1]=173;b[l.item+2]=2;b[l.item+3]=0xFF;b[l.ball]=1;b[l.ball+1]=1;b[l.ball+2]=9;b[l.ball+3]=0xFF;
 emptyList(b,l.party,6);b[l.party]=1;b[l.party+1]=25;size_t pbody=l.party+8;size_t pot=pbody+48*6;size_t pnick=pot+11*6;putBody(b,pbody,true,25,"PIKA",pot,pnick);
 for(size_t box=0;box<14;++box){size_t base=box<7?0x4000:0x6000;size_t idx=box<7?box:box-7;emptyList(b,base+idx*0x450,20);text(b,l.boxNames+box*9,"BOX",9);}
 size_t bo=0x4000;b[bo]=1;b[bo+1]=133;size_t bbody=bo+22;size_t bot=bbody+32*20;size_t bnick=bot+11*20;putBody(b,bbody,false,133,"EEVEE",bot,bnick);
 finalize(b,l);return b;
}
void check(SourceGame game,const L&l,bool crystal){
 auto raw=fixture(l,crystal);auto before=raw;auto parsed=parse(raw,game);assert(parsed);std::string error;auto trainer=PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save,error);assert(trainer&&error.empty());
 assert(trainer->getGameGroup()==Enums::GameVersion::GSC);assert(trainer->sourceGameId()==sourceGameId(game));assert(trainer->trainerName==(crystal?"KRIS":"GOLD"));assert(trainer->money==54321);assert(trainer->TID16==12345&&trainer->SID16==0);assert(trainer->getBoxCount()==14&&trainer->getSlotsPerBox()==20&&trainer->getPartySize()==1);assert(trainer->getCurrentBox()==0);
 assert(trainer->items.size()==5);assert(trainer->items[0].size()==1);assert(trainer->items[1].size()==1&&trainer->items[1][0].itemId==173&&trainer->items[1][0].quantity==2);assert(trainer->items[3].size()==1);
 auto* p=dynamic_cast<Pokemon::Pokemon2ReadOnly*>(trainer->party[0].get());assert(p);assert(p->speciesID()==25&&p->heldItem()==173&&p->level()==5);assert(p->isShiny(0,""));assert(p->gender()==1);assert(p->friendship()==70);assert(p->dvATK()==2&&p->dvSpecial()==10);assert(p->statExpSpecial()==500);assert(p->move(0)==33&&p->movePP(1)==5&&p->movePPUps(1)==1);assert(p->isPokerusInfected()&&!p->isPokerusCured());assert(Utils::utf16ToUtf8(p->nickname())=="PIKA");assert(p->rawOriginVersion()==(game==SourceGame::Gold?39:game==SourceGame::Silver?40:41));
 auto* boxed=dynamic_cast<Pokemon::Pokemon2ReadOnly*>(trainer->boxes[0][0].get());assert(boxed&&boxed->speciesID()==133&&!boxed->isPartyRecord());assert(trainer->boxNames[0]=="BOX");
 assert(trainer->hasStoredTrainerGender()==crystal);if(crystal)assert(trainer->trainerGender==1);assert(raw==before);
}
}

int main(){check(SourceGame::Gold,GS,false);check(SourceGame::Silver,GS,false);check(SourceGame::Crystal,C,true);std::cout<<"GSC read-only Trainer/Party/Boxes bridge tests passed\n";}
