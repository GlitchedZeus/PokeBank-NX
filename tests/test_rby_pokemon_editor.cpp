#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Pokemon/Experience.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

using namespace PokeVault::Integration::Gen1;
namespace {
uint8_t sum(const std::vector<uint8_t>& v,size_t start,size_t n) { unsigned s=0;for(size_t i=start;i<start+n;++i)s+=v[i];return (~s)&255; }
size_t bank(size_t b) {return (b<6?0x4000:0x6000)+(b%6)*0x462;}
void checksums(std::vector<uint8_t>& v) {
    v[0x3523]=sum(v,0x2598,0xF8B);
    if(v[0x284C]&0x80) for(size_t b:{size_t(0x4000),size_t(0x6000)}) {
        v[b+0x1A4C]=sum(v,b,0x1A4C);
        for(size_t i=0;i<6;++i)v[b+0x1A4D+i]=sum(v,b+i*0x462,0x462);
    }
}
std::vector<uint8_t> fixture(SourceGame game,bool initialized=true) {
    std::vector<uint8_t> v(0x8000,0);
    v[0x2598]=0x91;v[0x2599]=0x84;v[0x259A]=0x83;v[0x259B]=0x50; // RED
    v[0x2605]=0x12;v[0x2606]=0x34;
    v[0x25CA]=v[0x27E7]=v[0x2F2D]=0xFF;
    v[0x284C]=initialized?0x82:2;
    v[0x29C3]=game==SourceGame::Yellow?0x54:0x99;
    for(size_t b=0;b<12;++b)v[bank(b)+1]=0xFF;
    const size_t box=0x30C0,body=box+22;
    v[box]=1;v[box+1]=0x54;v[box+2]=0xFF; // Pikachu, internal 0x54
    v[body]=0x54;v[body+1]=0;v[body+2]=20;v[body+3]=5;
    v[body+5]=v[body+6]=23;v[body+7]=190;v[body+8]=33;
    v[body+12]=0x12;v[body+13]=0x34;v[body+16]=125;
    v[body+27]=0x88;v[body+28]=0x88;v[body+29]=35;
    std::fill_n(v.begin()+box+682,11,0x50);v[box+682]=0x91;v[box+683]=0x84;v[box+684]=0x83;
    std::fill_n(v.begin()+box+902,11,0x50);v[box+902]=0x8F;v[box+903]=0x88;v[box+904]=0x8A;v[box+905]=0x80; // PIKA
    if(initialized) std::copy_n(v.begin()+box,0x462,v.begin()+bank(2));
    checksums(v);return v;
}
std::vector<uint8_t> bytes(const StagedPokemonEditor& e) {return {e.stagedBytes().begin(),e.stagedBytes().end()};}
void currentBoxAdd(SourceGame game) {
    const auto raw = fixture(game);
    const auto source = parse(raw, game); assert(source);
    std::string error; auto editor = StagedPokemonEditor::create(*source.save, error); assert(editor);
    BoxPokemonCreate draft; draft.species = 29; draft.nickname = "Nidoran F";
    // Visual cell 19 in partially filled box 2 maps to packed position 1 in box 2.
    // Empty box 0 cannot steal that destination.
    const size_t currentBox = 2;
    auto slot = editor->appendSlot(currentBox, error); assert(slot == 1);
    assert(editor->stageAdd(currentBox, *slot, draft, error));
    assert(editor->boxedPokemon(currentBox, 1, error)->species == 29);
    assert(!editor->boxedPokemon(0, 0, error));
    auto output = editor->finalizedBytes(error); assert(parse(output, game));
    // An entirely empty selected box starts at zero within that selected box.
    slot = editor->appendSlot(7, error); assert(slot == 0);
    assert(editor->stageAdd(7, *slot, draft, error));
    for (size_t i = 2; i < 20; ++i) {
        slot = editor->appendSlot(currentBox, error); assert(slot == i);
        assert(editor->stageAdd(currentBox, *slot, draft, error));
    }
    const auto before = bytes(*editor);
    assert(!editor->appendSlot(currentBox, error) && error == "This Generation I box is full");
    assert(bytes(*editor) == before && !editor->boxedPokemon(0, 0, error));
    assert(!editor->appendSlot(12, error));
    assert(parse(editor->finalizedBytes(error), game));
    assert(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()));
    assert(std::equal(raw.begin(), raw.end(), source.save->sourceBytes().begin()));
}
void run(SourceGame game) {
    const auto raw=fixture(game);const auto source=parse(raw,game);assert(source);
    std::string error;auto e=StagedPokemonEditor::create(*source.save,error);assert(e);
    assert(e->metadata().sourceGame==game && e->metadata().sourceGameId==sourceGameId(game));
    auto p=[&](){auto p=e->boxedPokemon(2,0,error);assert(p);return *p;};
    assert(p().nickname=="PIKA" && p().originalTrainer=="RED");
    BoxPokemonEdit edit;edit.nickname="Sparky";assert(e->stageEdit(2,0,edit,error));assert(p().nickname=="Sparky");
    auto changed=bytes(*e);
    // Nickname only: current/cache name range and their documented checksum bytes.
    for(size_t i=0;i<raw.size();++i) if(raw[i]!=changed[i])
        assert((i>=0x30C0+902 && i<0x30C0+913)||(i>=bank(2)+902 && i<bank(2)+913)||i==0x3523||i==0x5A4C||i==0x5A4F);
    edit.nickname="PIKA";assert(e->stageEdit(2,0,edit,error));assert(!e->hasPendingChanges());assert(bytes(*e)==raw);
    edit={};edit.species=1;assert(e->stageEdit(2,0,edit,error));assert(p().species==1 && p().experience==135 && p().level==5);
    edit={};edit.level=25;assert(e->stageEdit(2,0,edit,error));assert(p().level==25 && p().experience==11735);
    edit={};edit.experience=Pokemon::getExpForLevel(26,3)+5;assert(e->stageEdit(2,0,edit,error));assert(p().level==26);
    edit.level=27;auto before=bytes(*e);assert(!e->stageEdit(2,0,edit,error));assert(bytes(*e)==before);
    edit={};edit.moves=std::array<uint8_t,4>{1,14,74,165};edit.ppUps=std::array<uint8_t,4>{1,2,3,0};edit.pp=std::array<uint8_t,4>{42,42,61,10};
    assert(e->stageEdit(2,0,edit,error));assert(p().moves==*edit.moves && p().pp==*edit.pp && p().ppUps==*edit.ppUps);
    edit.pp=std::array<uint8_t,4>{42,42,62,10};before=bytes(*e);assert(!e->stageEdit(2,0,edit,error));assert(bytes(*e)==before);
    edit={};edit.moves=std::array<uint8_t,4>{166,0,0,0};assert(!e->stageEdit(2,0,edit,error));
    edit={};edit.dvs=std::array<uint8_t,4>{15,2,3,4};assert(e->stageEdit(2,0,edit,error));assert((p().dvs==std::array<uint8_t,5>{10,15,2,3,4}));
    edit.dvs=std::array<uint8_t,4>{16,0,0,0};assert(!e->stageEdit(2,0,edit,error));
    edit={};edit.statExperience=std::array<uint16_t,5>{0,1,255,256,65535};assert(e->stageEdit(2,0,edit,error));assert(p().statExperience==*edit.statExperience);
    edit={};edit.otName="Will";edit.trainerId=65535;assert(e->stageEdit(2,0,edit,error));assert(p().originalTrainer=="Will" && p().trainerId==65535);
    edit={};edit.nickname="ABCDEFGHIJK";assert(!e->stageEdit(2,0,edit,error));edit.nickname="Poké";assert(!e->stageEdit(2,0,edit,error));
    auto output=e->finalizedBytes(error);assert(!output.empty() && parse(output,game));
    assert(output[0x3523]==sum(output,0x2598,0xF8B));
    for(size_t b:{size_t(0x4000),size_t(0x6000)}) {
        assert(output[b+0x1A4C]==sum(output,b,0x1A4C));
        for(size_t i=0;i<6;++i)assert(output[b+0x1A4D+i]==sum(output,b+i*0x462,0x462));
    }
    assert(std::equal(source.save->sourceBytes().begin(),source.save->sourceBytes().end(),raw.begin()));
    assert(std::equal(e->originalBytes().begin(),e->originalBytes().end(),raw.begin()));
    e->discard();assert(bytes(*e)==raw);
    BoxPokemonCreate create;create.species=133;create.nickname="Eevee";create.otName="Will";
    before=bytes(*e);assert(!e->stageAdd(2,0,create,error));assert(bytes(*e)==before);
    assert(!e->stageAdd(2,2,create,error));
    assert(e->stageAdd(2,1,create,error));assert(e->boxedPokemon(2,1,error)->species==133);
    assert(e->stageClone(2,0,2,2,error));assert(e->boxedPokemon(2,2,error)->rawBody==p().rawBody);
    assert(e->stageRemove(2,0,error));assert(e->boxedPokemon(2,0,error)->nickname=="Eevee");assert(e->boxedPokemon(2,1,error)->nickname=="PIKA");
    assert(!e->boxedPokemon(2,2,error));assert(parse(e->finalizedBytes(error),game));
    assert(e->stagedBytes()[0x30C0]==2 && e->stagedBytes()[0x30C3]==0xFF);
    e->discard();assert(e->stageAdd(2,1,create,error));assert(e->stageRemove(2,1,error));assert(!e->hasPendingChanges() && bytes(*e)==raw);
    for(size_t s=1;s<20;++s)assert(e->stageAdd(2,s,create,error));
    before=bytes(*e);assert(!e->stageAdd(2,20,create,error));assert(!e->stageClone(2,0,2,19,error));assert(bytes(*e)==before);
    e->discard();assert(e->stageClone(2,0,11,0,error));assert(e->boxedPokemon(11,0,error)->nickname=="PIKA");assert(parse(e->finalizedBytes(error),game));
    auto inventory=StagedInventoryEditor::create(*source.save,error);assert(inventory);
    assert(inventory->stageSetQuantity(PokeVault::Inventory::ClassicPocket::Balls,4,10,error));
    auto inv=inventory->finalize(error);assert(!inv.empty());auto merged=e->finalizedBytes(error,inv);assert(!merged.empty());
    assert(merged[0x25CA]==4 && merged[0x25CB]==10 && parse(merged,game).save->boxes()[11].slots[0]);
    inv[0x2606]^=1;checksums(inv);assert(e->finalizedBytes(error,inv).empty());
    // Every species can be created without introducing modern fields.
    e->discard();for(uint16_t species=1;species<=151;++species) {
        BoxPokemonCreate c;c.species=species;assert(e->stageAdd(2,1,c,error));assert(e->boxedPokemon(2,1,error)->species==species);assert(e->stageRemove(2,1,error));
    }
}
}
int main() {
    for(auto game:{SourceGame::Red,SourceGame::Blue,SourceGame::Yellow}) { run(game); currentBoxAdd(game); }
    assert(StagedPokemonEditor::moveMaxPP(74,3)==61);assert(StagedPokemonEditor::moveBasePP(105)==20);
    assert(StagedPokemonEditor::moveBasePP(166)==0);
    for(uint8_t a=0;a<16;++a) for(uint8_t d=0;d<16;++d) for(uint8_t s=0;s<16;++s) for(uint8_t c=0;c<16;++c)
        assert(StagedPokemonEditor::derivedHPDV({a,d,s,c})==(8*(a%2)+4*(d%2)+2*(s%2)+c%2));
    std::array<uint8_t,11> encoded{};std::string error;
    for(const auto text:{"Abc 019-?!","Will","1234567890"}) {
        assert(StagedPokemonEditor::encodeName(text,10,encoded,error));assert(decodeGen1String(encoded,RegionLayout::International)==text);
    }
    assert(!StagedPokemonEditor::encodeName("12345678",7,encoded,error));
    auto raw=fixture(SourceGame::Red,false);auto p=parse(raw,SourceGame::Red);assert(p);
    auto e=StagedPokemonEditor::create(*p.save,error);BoxPokemonCreate create;
    assert(e->stageAdd(11,0,create,error));auto output=e->finalizedBytes(error);assert(!output.empty());
    assert(parse(output,SourceGame::Red).save->metadata().boxesInitialized);
    assert(parse(output,SourceGame::Red).save->boxes()[2].slots[0]->nickname=="PIKA");
    assert(e->stageRemove(11,0,error));assert(bytes(*e)==raw);
    // Japanese layout remains browsable but is capability-gated for mutation.
    std::vector<uint8_t> jp(0x8000);jp[0x2ED6]=jp[0x302E]=0xFF;jp[0x3594]=sum(jp,0x2598,0xFFC);
    auto j=parse(jp,SourceGame::Red);assert(j);assert(!StagedPokemonEditor::create(*j.save,error));
    std::cout<<"Gen I staged boxed Pokemon editor: PASS (R/B/Y fields, lists, encoding, immutable export)\n";
}
