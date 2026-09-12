#include "Integration/Gen2/Gen2StagedEditor.h"

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

size_t listLen(size_t cap,size_t body,size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
size_t stride(){return listLen(20,32,11)+2;}
size_t boxStart(size_t b){return b<7?0x4000+b*stride():0x6000+(b-7)*stride();}
void emptyList(std::vector<uint8_t>& d,size_t ofs){d[ofs]=0;d[ofs+1]=0xFF;}
void name(std::vector<uint8_t>& d,size_t ofs,size_t len,char c){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(ofs),len,0x50);
    d[ofs]=static_cast<uint8_t>(0x80+c-'A');
}
void pokemonList(std::vector<uint8_t>& d,size_t ofs,size_t cap,size_t bodySize,size_t str,bool party){
    constexpr uint8_t species=25;
    d[ofs]=1;d[ofs+1]=species;d[ofs+2]=0xFF;
    const size_t body=ofs+1+(cap+1);
    d[body]=species;d[body+1]=1;d[body+2]=33;d[body+3]=45;
    d[body+6]=0x12;d[body+7]=0x34;d[body+9]=0x10;
    for(int i=0;i<5;i++){d[body+11+2*i]=0;d[body+12+2*i]=static_cast<uint8_t>(i+1);}
    d[body+21]=0x7A;d[body+22]=0xAA;d[body+23]=10;d[body+24]=20;
    d[body+27]=123;d[body+28]=0x21;d[body+31]=15;
    if(party){
        auto be=[&](size_t o,uint16_t v){d[body+o]=v>>8;d[body+o+1]=v&0xFF;};
        d[body+32]=0x08;be(34,35);be(36,40);be(38,30);be(40,20);be(42,25);be(44,26);be(46,27);
    }
    const size_t ot=body+cap*bodySize;
    const size_t nick=ot+cap*str;
    name(d,ot,str,'O');name(d,nick,str,'M');
}
void pairPocket(std::vector<uint8_t>& d,size_t ofs,std::initializer_list<std::pair<uint8_t,uint8_t>> entries){
    d[ofs]=static_cast<uint8_t>(entries.size());
    size_t p=ofs+1;
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
void fillItemsPocket(std::vector<uint8_t>& d,size_t ofs){
    constexpr std::array<uint8_t,20> ids{
        3,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,26,27
    };
    d[ofs]=static_cast<uint8_t>(ids.size());
    size_t p=ofs+1;
    for(uint8_t id:ids){d[p++]=id;d[p++]=1;}
    d[p]=0xFF;
}
void checksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(size_t i=0x2009;i<=l.end;i++)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=s&0xFF;d[l.sum+1]=s>>8;
}
std::vector<uint8_t> fixture(const L& l,bool rtc=false){
    std::vector<uint8_t>d(0x8000+(rtc?7:0));
    d[0x2009]=0x12;d[0x200A]=0x34;name(d,0x200B,11,'A');
    d[l.money]=0x01;d[l.money+1]=0xE2;d[l.money+2]=0x40;
    d[l.idx]=0;if(l.gender!=N)d[l.gender]=1;
    for(size_t b=0;b<14;b++){emptyList(d,boxStart(b));name(d,l.names+b*9,9,static_cast<char>('A'+b));}
    pokemonList(d,boxStart(0),20,32,11,false);
    emptyList(d,l.current);pokemonList(d,l.party,6,48,11,true);
    inventory(d,l);checksum(d,l);
    if(rtc){for(size_t i=0;i<7;i++)d[0x8000+i]=static_cast<uint8_t>(0xA0+i);}
    return d;
}
uint16_t le16(const std::vector<uint8_t>& d,size_t o){return static_cast<uint16_t>(d[o]|(d[o+1]<<8));}
uint8_t quantity(const InventoryRecord& inv,InventoryPocket pocket,uint8_t id){
    const std::vector<InventoryItem>* list=nullptr;
    if(pocket==InventoryPocket::Items)list=&inv.items;
    else if(pocket==InventoryPocket::Balls)list=&inv.balls;
    assert(list);
    for(const auto& item:*list)if(item.itemId==id)return item.quantity;
    return 0;
}
bool inRange(size_t p,size_t a,size_t b){return p>=a&&p<b;}
bool allowedGoldDiff(size_t p){
    return inRange(p,0x200B,0x2016)||inRange(p,0x23DB,0x23DE)||
           inRange(p,0x241F,0x2449)||inRange(p,0x2464,0x247E)||
           inRange(p,0x2D69,0x2D6B)||inRange(p,0x7E6D,0x7E6F)||
           inRange(p,0x15C7,0x17ED)||inRange(p,0x3D69,0x3F13)||
           inRange(p,0x0C6B,0x10E8)||inRange(p,0x7E39,0x7E6D)||
           inRange(p,0x10E8,0x15C7);
}
}

int main(){
    {
        auto raw=fixture(GS,true);const auto original=raw;
        auto parsed=parse(raw,SourceGame::Gold);assert(parsed);
        std::string error;
        auto editor=StagedEditor::create(*parsed.save,error);assert(editor&&error.empty());
        assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::TrainerIdentity));
        assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::Money));
        assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::Inventory));
        assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::Balls));
        assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::Medicine));
        assert(!editor->capabilities().supports(PokeVault::SaveEdit::Capability::KeyItems));
        assert(!editor->capabilities().supports(PokeVault::SaveEdit::Capability::PlayerPosition));
        assert(editor->trainerName()=="A"&&editor->money()==123456);
        assert(editor->itemQuantity(InventoryPocket::Items,kPotionItemId)==2);
        assert(editor->itemQuantity(InventoryPocket::Balls,kPokeBallItemId)==5);
        assert(editor->itemQuantity(InventoryPocket::Balls,kMasterBallItemId)==0);

        // Field limits and encoding are validated before staged bytes are accepted.
        assert(editor->stageTrainerName("ABCDEFG",error)); // exact international maximum length
        assert(editor->trainerName()=="ABCDEFG");
        assert(editor->stageTrainerName("WILL",error));
        assert(!editor->stageTrainerName("TOO-LONG",error));
        assert(!editor->stageTrainerName("A@",error));
        assert(editor->stageMoney(999999,error));
        assert(!editor->stageMoney(1000000,error));

        // Exercise update, remove, re-add, valid pocket routing, invalid IDs and the device-target trio.
        assert(editor->stageItemQuantity(InventoryPocket::Items,kPotionItemId,0,error));
        assert(editor->itemQuantity(InventoryPocket::Items,kPotionItemId)==0);
        assert(editor->stageItemQuantity(InventoryPocket::Items,kPotionItemId,20,error));
        assert(editor->stageItemQuantity(InventoryPocket::Balls,kPokeBallItemId,30,error));
        assert(editor->stageItemQuantity(InventoryPocket::Balls,kMasterBallItemId,1,error));
        assert(!editor->stageItemQuantity(InventoryPocket::Balls,kPotionItemId,1,error));
        assert(!editor->stageItemQuantity(InventoryPocket::Items,0xFF,1,error));
        assert(!editor->stageItemQuantity(InventoryPocket::Items,kPotionItemId,100,error));
        assert(raw==original); // caller/source vector remains sacred
        assert(editor->pendingChanges().size()==5);

        auto out=editor->finalizedBytes(error);assert(!out.empty()&&error.empty());
        assert(out.size()==original.size());
        assert(std::equal(out.begin()+0x8000,out.end(),original.begin()+0x8000));
        assert(le16(out,GS.sum)==le16(out,GS.sum2));
        auto reload=parse(out,SourceGame::Gold);assert(reload);
        assert(reload.save->trainer().name=="WILL");
        assert(reload.save->trainer().money==999999);
        auto inv=decodeInventory(reload.save->payloadBytes(),RegionLayout::International,VersionFamily::GoldSilver);
        assert(inv.available);
        assert(quantity(inv,InventoryPocket::Items,kPotionItemId)==20);
        assert(quantity(inv,InventoryPocket::Balls,kPokeBallItemId)==30);
        assert(quantity(inv,InventoryPocket::Balls,kMasterBallItemId)==1);
        for(size_t i=0;i<0x8000;i++)if(out[i]!=original[i])assert(allowedGoldDiff(i));

        editor->discard();assert(!editor->hasPendingChanges());
        assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),original.begin()));
    }
    {
        auto raw=fixture(C);auto parsed=parse(raw,SourceGame::Crystal);assert(parsed);
        std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor);
        assert(editor->stageTrainerName("CRYS",error));assert(editor->stageMoney(543210,error));
        auto out=editor->finalizedBytes(error);assert(!out.empty());
        assert(le16(out,C.sum)==le16(out,C.sum2));
        auto reload=parse(out,SourceGame::Crystal);assert(reload);
        assert(reload.save->trainer().name=="CRYS"&&reload.save->trainer().money==543210);
        assert(reload.save->trainer().gender&&*reload.save->trainer().gender==1);
    }
    {
        // A full Items pocket must reject another entry rather than overwrite or overflow adjacent data.
        auto raw=fixture(GS);
        fillItemsPocket(raw,GS.items);
        checksum(raw,GS);
        auto parsed=parse(raw,SourceGame::Gold);assert(parsed);
        std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor);
        assert(!editor->stageItemQuantity(InventoryPocket::Items,28,1,error));
        assert(error.find("full")!=std::string::npos);
        assert(editor->stagedBytes().size()==raw.size());
        assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),raw.begin()));
    }
    std::cout<<"Generation II staged editor round-trip tests: PASS\n";
}
