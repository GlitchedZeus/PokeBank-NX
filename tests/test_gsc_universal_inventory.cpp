#include "Integration/Gen2/Gen2StagedEditor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen2;

namespace {
struct L {
    VersionFamily family;
    std::size_t money, idx, names, party, current, end, sum, sum2, gender;
    std::size_t tm, items, key, balls, pc;
};
constexpr std::size_t N = static_cast<std::size_t>(-1);
constexpr L GS{VersionFamily::GoldSilver,0x23DB,0x2724,0x2727,0x288A,0x2D6C,0x2D68,0x2D69,0x7E6D,N,
               0x23E6,0x241F,0x2449,0x2464,0x247E};
constexpr L C {VersionFamily::Crystal,0x23DC,0x2700,0x2703,0x2865,0x2D10,0x2B82,0x2D0D,0x1F0D,0x3E3D,
               0x23E7,0x2420,0x244A,0x2465,0x247F};

std::size_t listLen(std::size_t cap,std::size_t body,std::size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
std::size_t stride(){return listLen(20,32,11)+2;}
std::size_t boxStart(std::size_t b){return b<7?0x4000+b*stride():0x6000+(b-7)*stride();}
void emptyList(std::vector<uint8_t>& d,std::size_t ofs){d[ofs]=0;d[ofs+1]=0xFF;}
void name(std::vector<uint8_t>& d,std::size_t ofs,std::size_t len,char c){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(ofs),len,0x50);
    d[ofs]=static_cast<uint8_t>(0x80+c-'A');
}
void pokemonList(std::vector<uint8_t>& d,std::size_t ofs,std::size_t cap,std::size_t bodySize,std::size_t str,bool party){
    constexpr uint8_t species=25;
    d[ofs]=1;d[ofs+1]=species;d[ofs+2]=0xFF;
    const std::size_t body=ofs+1+(cap+1);
    d[body]=species;d[body+1]=1;d[body+2]=33;d[body+3]=45;
    d[body+6]=0x12;d[body+7]=0x34;d[body+9]=0x10;
    for(int i=0;i<5;i++){d[body+11+2*i]=0;d[body+12+2*i]=static_cast<uint8_t>(i+1);}
    d[body+21]=0x7A;d[body+22]=0xAA;d[body+23]=10;d[body+24]=20;
    d[body+27]=123;d[body+28]=0x21;d[body+31]=15;
    if(party){
        auto be=[&](std::size_t o,uint16_t v){d[body+o]=static_cast<uint8_t>(v>>8);d[body+o+1]=static_cast<uint8_t>(v);};
        d[body+32]=0x08;be(34,35);be(36,40);be(38,30);be(40,20);be(42,25);be(44,26);be(46,27);
    }
    const std::size_t ot=body+cap*bodySize;
    const std::size_t nick=ot+cap*str;
    name(d,ot,str,'O');name(d,nick,str,'M');
}
void pairPocket(std::vector<uint8_t>& d,std::size_t ofs,std::initializer_list<std::pair<uint8_t,uint8_t>> entries){
    d[ofs]=static_cast<uint8_t>(entries.size());
    std::size_t p=ofs+1;
    for(const auto& [id,qty]:entries){d[p++]=id;d[p++]=qty;}
    d[p]=0xFF;
}
void inventory(std::vector<uint8_t>& d,const L& l){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(l.tm),57,0);
    pairPocket(d,l.items,{{kPotionItemId,2}});
    d[l.key]=0;d[l.key+1]=0xFF;
    pairPocket(d,l.balls,{{kPokeBallItemId,5}});
    d[l.pc]=0;d[l.pc+1]=0xFF;
}
void checksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(std::size_t i=0x2009;i<=l.end;i++)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=static_cast<uint8_t>(s);d[l.sum+1]=static_cast<uint8_t>(s>>8);
}
std::vector<uint8_t> fixture(const L& l){
    std::vector<uint8_t>d(0x8000);
    d[0x2009]=0x12;d[0x200A]=0x34;name(d,0x200B,11,'A');
    d[l.money]=0x01;d[l.money+1]=0xE2;d[l.money+2]=0x40;
    d[l.idx]=0;if(l.gender!=N)d[l.gender]=1;
    for(std::size_t b=0;b<14;b++){emptyList(d,boxStart(b));name(d,l.names+b*9,9,static_cast<char>('A'+b));}
    pokemonList(d,boxStart(0),20,32,11,false);
    emptyList(d,l.current);pokemonList(d,l.party,6,48,11,true);
    inventory(d,l);checksum(d,l);
    return d;
}
uint8_t q(const InventoryRecord& inv, InventoryPocket pocket, uint8_t id){
    const std::vector<InventoryItem>* list=nullptr;
    switch(pocket){
        case InventoryPocket::TMHM:list=&inv.tmhm;break;
        case InventoryPocket::Items:list=&inv.items;break;
        case InventoryPocket::KeyItems:list=&inv.keyItems;break;
        case InventoryPocket::Balls:list=&inv.balls;break;
        case InventoryPocket::PCItems:list=&inv.pcItems;break;
    }
    const auto it=std::find_if(list->begin(),list->end(),[=](const auto& item){return item.itemId==id;});
    return it==list->end()?0:it->quantity;
}
}

int main(){
    {
        auto raw=fixture(GS); const auto source=raw;
        auto parsed=parse(raw,SourceGame::Gold); assert(parsed);
        std::string error; auto editor=StagedEditor::create(*parsed.save,error); assert(editor&&error.empty());
        assert(editor->inventoryEntries(InventoryPocket::KeyItems,error).empty()&&error.empty());
        assert(editor->inventoryEntries(InventoryPocket::TMHM,error).empty()&&error.empty());
        assert(editor->inventoryEntries(InventoryPocket::PCItems,error).empty()&&error.empty());

        assert(editor->stageInventoryQuantity(InventoryPocket::KeyItems,54,1,error));
        assert(!editor->stageInventoryQuantity(InventoryPocket::KeyItems,54,2,error));
        assert(!editor->stageInventoryQuantity(InventoryPocket::KeyItems,70,1,error)); // Crystal-only.
        assert(editor->stageInventoryQuantity(InventoryPocket::TMHM,191,3,error)); // TM01.
        assert(editor->stageInventoryQuantity(InventoryPocket::TMHM,249,1,error)); // HM07.
        assert(!editor->stageInventoryQuantity(InventoryPocket::TMHM,249,2,error));
        assert(editor->stageInventoryQuantity(InventoryPocket::PCItems,54,1,error));
        assert(!editor->stageInventoryQuantity(InventoryPocket::PCItems,54,2,error));
        assert(editor->stageInventoryQuantity(InventoryPocket::PCItems,5,20,error));
        assert(editor->stageInventoryQuantity(InventoryPocket::Balls,5,30,error));
        assert(editor->stageInventoryQuantity(InventoryPocket::Balls,5,31,error)); // update, not duplicate.
        assert(editor->stagedInventoryQuantity(InventoryPocket::Balls,5)==31);
        assert(raw==source);

        bool sawMachine=false;
        for(const auto& change:editor->pendingChanges())
            if(change.label.find("TM01")!=std::string::npos) sawMachine=true;
        assert(sawMachine);

        auto out=editor->finalizedBytes(error); assert(!out.empty()&&error.empty());
        auto reload=parse(out,SourceGame::Gold); assert(reload);
        auto inv=decodeInventory(reload.save->payloadBytes(),RegionLayout::International,VersionFamily::GoldSilver);
        assert(inv.available);
        assert(q(inv,InventoryPocket::KeyItems,54)==1);
        assert(q(inv,InventoryPocket::TMHM,191)==3);
        assert(q(inv,InventoryPocket::TMHM,249)==1);
        assert(q(inv,InventoryPocket::PCItems,54)==1);
        assert(q(inv,InventoryPocket::PCItems,5)==20);
        assert(q(inv,InventoryPocket::Balls,5)==31);
        assert(std::equal(editor->originalBytes().begin(),editor->originalBytes().end(),source.begin()));

        assert(editor->stageInventoryQuantity(InventoryPocket::KeyItems,54,0,error));
        assert(editor->stageInventoryQuantity(InventoryPocket::TMHM,191,0,error));
        assert(editor->stagedInventoryQuantity(InventoryPocket::KeyItems,54)==0);
        assert(editor->stagedInventoryQuantity(InventoryPocket::TMHM,191)==0);
    }
    {
        auto raw=fixture(C); auto parsed=parse(raw,SourceGame::Crystal); assert(parsed);
        std::string error; auto editor=StagedEditor::create(*parsed.save,error); assert(editor);
        assert(editor->stageInventoryQuantity(InventoryPocket::KeyItems,70,1,error));
        auto out=editor->finalizedBytes(error); assert(!out.empty());
        auto reload=parse(out,SourceGame::Crystal); assert(reload);
        auto inv=decodeInventory(reload.save->payloadBytes(),RegionLayout::International,VersionFamily::Crystal);
        assert(inv.available&&q(inv,InventoryPocket::KeyItems,70)==1);
    }
    std::cout<<"GSC universal staged inventory: PASS\n";
}
