#include "Legacy/GSCReadOnlyTrainer.h"

#include "Integration/Gen2/Gen2PersonalData.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault;

namespace {
struct L {
    Integration::Gen2::RegionLayout region;
    Integration::Gen2::VersionFamily family;
    std::size_t money, idx, names, party, current, end, sum, gender;
    uint8_t cap, boxes, str, split;
};
constexpr std::size_t N = static_cast<std::size_t>(-1);
constexpr L IGS{Integration::Gen2::RegionLayout::International,Integration::Gen2::VersionFamily::GoldSilver,
    0x23DB,0x2724,0x2727,0x288A,0x2D6C,0x2D68,0x2D69,N,20,14,11,7};
constexpr L IC{Integration::Gen2::RegionLayout::International,Integration::Gen2::VersionFamily::Crystal,
    0x23DC,0x2700,0x2703,0x2865,0x2D10,0x2B82,0x2D0D,0x3E3D,20,14,11,7};
constexpr L JGS{Integration::Gen2::RegionLayout::Japanese,Integration::Gen2::VersionFamily::GoldSilver,
    0x23BC,0x2705,0x2708,0x283E,0x2D10,0x2C8B,0x2D0D,N,30,9,6,6};

std::size_t listLen(std::size_t cap,std::size_t body,std::size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
std::size_t stride(const L& l){return listLen(l.cap,32,l.str)+2;}
std::size_t boxStart(const L& l,std::size_t b){return b<l.split?0x4000+b*stride(l):0x6000+(b-l.split)*stride(l);}
void emptyList(std::vector<uint8_t>& d,std::size_t ofs){d[ofs]=0;d[ofs+1]=0xFF;}
void text(std::vector<uint8_t>& d,std::size_t ofs,std::size_t len,char c){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(ofs),static_cast<std::ptrdiff_t>(len),0x50);
    if(c>='A'&&c<='Z')d[ofs]=static_cast<uint8_t>(0x80+c-'A');
}
void pokemonList(std::vector<uint8_t>& d,std::size_t ofs,std::size_t cap,std::size_t bodySize,
                 std::size_t str,uint8_t species,bool party){
    d[ofs]=1;d[ofs+1]=species;d[ofs+2]=0xFF;
    const std::size_t body=ofs+1+(cap+1);
    d[body]=species;d[body+1]=1;
    d[body+2]=33;d[body+3]=45;d[body+4]=0;d[body+5]=0;
    d[body+6]=0x12;d[body+7]=0x34;
    d[body+8]=0;d[body+9]=0x10;d[body+10]=0;
    for(int i=0;i<5;++i){d[body+11+2*i]=0;d[body+12+2*i]=static_cast<uint8_t>(i+1);}
    d[body+21]=0x7A;d[body+22]=0xAA; // shiny: Atk 7, Def/Spe/Special 10
    d[body+23]=10|0x40;d[body+24]=20|0x80;d[body+25]=0;d[body+26]=0;
    d[body+27]=123;d[body+28]=0x21;d[body+29]=0x12;d[body+30]=0x34;d[body+31]=15;
    if(party){
        d[body+32]=0x08;d[body+33]=0;
        const auto be=[&](std::size_t o,uint16_t v){d[body+o]=static_cast<uint8_t>(v>>8);d[body+o+1]=static_cast<uint8_t>(v);};
        be(34,35);be(36,40);be(38,30);be(40,20);be(42,25);be(44,26);be(46,27);
    }
    const std::size_t ot=body+cap*bodySize;
    const std::size_t nick=ot+cap*str;
    text(d,ot,str,'O');text(d,nick,str,'M');
}
void checksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(std::size_t i=0x2009;i<=l.end;++i)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=static_cast<uint8_t>(s);d[l.sum+1]=static_cast<uint8_t>(s>>8);
}
std::vector<uint8_t> fixture(const L& l,uint8_t storedSpecies=1,uint8_t currentCopySpecies=25,uint8_t partySpecies=25){
    const std::size_t size=l.gender==0x8000?0x10000:0x8000;
    std::vector<uint8_t>d(size);
    d[0x2009]=0x12;d[0x200A]=0x34;text(d,0x200B,l.str,'A');
    d[l.money]=0x01;d[l.money+1]=0xE2;d[l.money+2]=0x40;d[l.idx]=0;
    if(l.gender!=N)d[l.gender]=1;
    for(std::size_t b=0;b<l.boxes;++b){
        emptyList(d,boxStart(l,b));
        text(d,l.names+b*9,9,static_cast<char>('A'+(b%26)));
    }
    pokemonList(d,boxStart(l,0),l.cap,32,l.str,storedSpecies,false);
    // Deliberately disagree with the canonical stored bank. The bridge must not duplicate or
    // substitute this current-box working copy for box 0.
    pokemonList(d,l.current,l.cap,32,l.str,currentCopySpecies,false);
    pokemonList(d,l.party,6,48,l.str,partySpecies,true);
    checksum(d,l);
    return d;
}

std::size_t occupiedBoxSlots(const Legacy::GSCReadOnlyTrainer& trainer){
    std::size_t count=0;
    for(std::size_t box=0;box<trainer.getBoxCount();++box)
        for(std::size_t slot=0;slot<trainer.getSlotsPerBox();++slot)
            if(trainer.boxes[box][slot])++count;
    return count;
}
}

int main(){
    {
        auto bytes=fixture(IGS,1,25,25);
        const auto before=bytes;
        auto parsed=Integration::Gen2::parse(bytes,Integration::Gen2::SourceGame::Gold);
        assert(parsed);
        std::string error;
        auto trainer=Legacy::GSCReadOnlyTrainer::create(*parsed.save,error);
        assert(trainer&&error.empty());
        assert(trainer->getGameGroup()==Enums::GameVersion::GSC);
        assert(trainer->sourceGameId()=="gold_gbc");
        assert(!trainer->japaneseLayout()&&!trainer->crystalFamily()&&!trainer->hasTrainerGender());
        assert(trainer->trainerName=="A"&&trainer->money==123456&&trainer->TID16==0x1234);
        assert(trainer->SID16==0&&trainer->SID==0&&trainer->ID32==0x1234);
        assert(trainer->getBoxCount()==14&&trainer->getSlotsPerBox()==20&&trainer->getCurrentBox()==0);
        assert(trainer->boxNames.size()==14&&trainer->boxNames[0]=="A");
        assert(trainer->getPartySize()==1&&trainer->party[0]);

        auto* party=static_cast<Pokemon::Pokemon2ReadOnly*>(trainer->party[0].get());
        assert(party->speciesID()==25&&party->heldItem()==1&&party->friendship()==123);
        assert(party->dvATK()==7&&party->dvDEF()==10&&party->dvSPE()==10&&party->dvSpecial()==10);
        assert(party->statExpHP()==1&&party->statExpSpecial()==5);
        assert(party->move(0)==33&&party->move(1)==45&&party->movePP(0)==10&&party->movePPUps(0)==1);
        assert(party->isShiny(0,{})&&party->gender()==1); // Pikachu: Attack DV 7 is female at the 50% boundary.
        assert(party->statusByte()==0x08&&party->statHPCurrent()==35&&party->statHPMax()==40);
        assert(party->statATK()==30&&party->statDEF()==20&&party->statSPE()==25);
        assert(party->statSPA()==26&&party->statSPD()==27);
        const auto* personal=Integration::Gen2::personalRecord(25);assert(personal);
        assert(party->baseHP()==personal->hp&&party->baseATK()==personal->attack);
        assert(party->ivATK()==0&&party->evATK()==0&&party->ability()==0&&party->nature()==0);
        auto clone=party->clone();assert(clone&&clone->getGameGroup()==Enums::GameVersion::GSC&&clone->speciesID()==25);

        // Stored box bank is canonical. The current-box copy contains Pikachu, but box 0 contains
        // exactly one Bulbasaur and no duplicated working-copy Pokemon.
        assert(trainer->boxes[0][0]&&trainer->boxes[0][0]->speciesID()==1);
        assert(occupiedBoxSlots(*trainer)==1);
        assert(bytes==before);
        assert(parsed.save->sourceBytes().size()==before.size());
        assert(std::equal(parsed.save->sourceBytes().begin(),parsed.save->sourceBytes().end(),before.begin()));
    }
    {
        auto bytes=fixture(IC);
        auto parsed=Integration::Gen2::parse(bytes,Integration::Gen2::SourceGame::Crystal);assert(parsed);
        std::string error;auto trainer=Legacy::GSCReadOnlyTrainer::create(*parsed.save,error);assert(trainer);
        assert(trainer->sourceGameId()=="crystal_gbc"&&trainer->crystalFamily());
        assert(trainer->hasTrainerGender()&&trainer->trainerGender==1);
    }
    {
        auto bytes=fixture(JGS);
        auto parsed=Integration::Gen2::parse(bytes,Integration::Gen2::SourceGame::Silver);assert(parsed);
        std::string error;auto trainer=Legacy::GSCReadOnlyTrainer::create(*parsed.save,error);assert(trainer);
        assert(trainer->sourceGameId()=="silver_gbc"&&trainer->japaneseLayout());
        assert(trainer->getBoxCount()==9&&trainer->getSlotsPerBox()==30&&trainer->boxNames.size()==9);
    }
    {
        auto bytes=fixture(IGS,1,25,201);
        auto parsed=Integration::Gen2::parse(bytes,Integration::Gen2::SourceGame::Gold);assert(parsed);
        std::string error;auto trainer=Legacy::GSCReadOnlyTrainer::create(*parsed.save,error);assert(trainer);
        auto* unown=static_cast<Pokemon::Pokemon2ReadOnly*>(trainer->party[0].get());
        assert(unown->speciesID()==201&&unown->form()==21); // Gen II DV-derived Unown letter index.
    }

    std::cout<<"Generation II readonly Trainer/Party/Boxes + PK2 bridge: PASS\n";
}
