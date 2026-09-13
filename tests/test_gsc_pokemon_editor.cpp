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

size_t listLen(size_t cap,size_t body,size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
size_t stride(){return listLen(20,32,11)+2;}
size_t boxStart(size_t b){return b<7?0x4000+b*stride():0x6000+(b-7)*stride();}
size_t boxBody(size_t b){return boxStart(b)+1+21;}
size_t boxOT(size_t b){return boxBody(b)+20*32;}
size_t boxNick(size_t b){return boxOT(b)+20*11;}

uint8_t encodeChar(char c){
    if(c>='A'&&c<='Z')return static_cast<uint8_t>(0x80+c-'A');
    if(c>='a'&&c<='z')return static_cast<uint8_t>(0xA0+c-'a');
    if(c>='0'&&c<='9')return static_cast<uint8_t>(0xF6+c-'0');
    if(c==' ')return 0x7F;
    if(c=='-')return 0xE3;
    if(c=='!')return 0xE7;
    return 0xE6;
}
void text(std::vector<uint8_t>& d,size_t ofs,size_t len,const std::string& s){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(ofs),len,0x50);
    for(size_t i=0;i<std::min(len,s.size());++i)d[ofs+i]=encodeChar(s[i]);
}
void name(std::vector<uint8_t>& d,size_t ofs,size_t len,char c){text(d,ofs,len,std::string(1,c));}
void emptyList(std::vector<uint8_t>& d,size_t ofs){d[ofs]=0;d[ofs+1]=0xFF;}
void be16(std::vector<uint8_t>& d,size_t o,uint16_t v){d[o]=static_cast<uint8_t>(v>>8);d[o+1]=static_cast<uint8_t>(v);}
void be24(std::vector<uint8_t>& d,size_t o,uint32_t v){d[o]=static_cast<uint8_t>(v>>16);d[o+1]=static_cast<uint8_t>(v>>8);d[o+2]=static_cast<uint8_t>(v);}

void writeStoredPokemon(std::vector<uint8_t>& d,size_t box,size_t slot,uint8_t species,
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
void setBoxCount(std::vector<uint8_t>& d,size_t box,size_t count,uint8_t species=25){
    const size_t s=boxStart(box);d[s]=static_cast<uint8_t>(count);
    for(size_t i=0;i<20;i++)d[s+1+i]=(i<count)?species:0;
    d[s+1+count]=0xFF;
}
void onePokemonBox(std::vector<uint8_t>& d,size_t box,const std::string& nick="MON"){
    setBoxCount(d,box,1,25);writeStoredPokemon(d,box,0,25,"OT",nick);
}
void fullBox(std::vector<uint8_t>& d,size_t box){
    setBoxCount(d,box,20,25);
    for(size_t i=0;i<20;i++)writeStoredPokemon(d,box,i,25,"OT","FULL");
}
void partyList(std::vector<uint8_t>& d,size_t ofs){
    const size_t cap=6,str=11,bodySize=48;
    d[ofs]=1;d[ofs+1]=25;d[ofs+2]=0xFF;
    const size_t body=ofs+1+(cap+1);d[body]=25;d[body+1]=1;d[body+2]=33;
    be16(d,body+6,0x1234);be24(d,body+8,3375);
    d[body+21]=0x7A;d[body+22]=0xAA;d[body+23]=35;d[body+27]=123;d[body+31]=15;
    const size_t ot=body+cap*bodySize,nick=ot+cap*str;text(d,ot,str,"OT");text(d,nick,str,"PARTY");
}
void pairPocket(std::vector<uint8_t>& d,size_t ofs,std::initializer_list<std::pair<uint8_t,uint8_t>> entries){
    d[ofs]=static_cast<uint8_t>(entries.size());size_t p=ofs+1;
    for(const auto& [id,qty]:entries){d[p++]=id;d[p++]=qty;}d[p]=0xFF;
}
void inventory(std::vector<uint8_t>& d,const L& l){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(l.tm),57,0);
    pairPocket(d,l.items,{{kPotionItemId,2}});d[l.key]=0;d[l.key+1]=0xFF;
    pairPocket(d,l.balls,{{kPokeBallItemId,5}});d[l.pc]=0;d[l.pc+1]=0xFF;
}
void checksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(size_t i=0x2009;i<=l.end;i++)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=static_cast<uint8_t>(s);d[l.sum+1]=static_cast<uint8_t>(s>>8);
}
std::vector<uint8_t> fixture(const L& l,bool rtc=false){
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
uint16_t le16(const std::vector<uint8_t>& d,size_t o){return static_cast<uint16_t>(d[o]|(d[o+1]<<8));}
bool differsOnlyInRange(const std::vector<uint8_t>& a,const std::vector<uint8_t>& b,size_t begin,size_t end){
    assert(a.size()==b.size());for(size_t i=0;i<a.size();++i)if(a[i]!=b[i]&&(i<begin||i>=end))return false;return true;
}

void runFamily(const L& l,SourceGame game){
    auto raw=fixture(l,true);const auto source=raw;
    auto parsed=parse(raw,game);assert(parsed);
    // The accepted read parser intentionally keeps the raw PokemonRecord gender unresolved.
    // The staged semantic layer must derive gender from species + Attack DV without changing it.
    assert(parsed.save->boxes()[0].slots[0]);
    assert(parsed.save->boxes()[0].slots[0]->gender==2);
    std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor&&error.empty());
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::BoxPokemon));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonEditing));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonCreation));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonClone));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonShinyToggle));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::MachineMoveNames));
    assert(!editor->capabilities().supports(PokeVault::SaveEdit::Capability::PartyPokemon));

    auto p=editor->boxedPokemon(0,0,error);assert(p&&p->species==25&&p->nickname=="PIKA");
    assert(p->gender==static_cast<uint8_t>(genderFromAttackDV(p->species,p->dvs[1])));
    assert(p->shiny);assert(StagedEditor::isShinyDVs({7,10,10,10}));
    assert(StagedEditor::derivedHPDV({7,10,10,10})==8);
    assert(raw==source);assert(std::equal(parsed.save->sourceBytes().begin(),parsed.save->sourceBytes().end(),source.begin()));

    BoxPokemonEdit edit;edit.species=26;edit.nickname="SPARKY";edit.level=20;edit.heldItem=1;
    edit.moves=std::array<uint8_t,4>{84,85,0,0};
    edit.dvs=std::array<uint8_t,4>{9,8,7,6};
    edit.statExperience=std::array<uint16_t,5>{100,200,300,400,500};
    edit.otName="RED";edit.trainerId=4321;edit.friendship=200;edit.pokerus=0x21;edit.caughtData=0x1234;
    assert(editor->stageBoxPokemonEdit(0,0,edit,error));
    auto changed=editor->boxedPokemon(0,0,error);assert(changed);
    assert(changed->species==26&&changed->nickname=="SPARKY"&&changed->level==20&&changed->heldItem==1);
    assert((changed->moves==std::array<uint8_t,4>{84,85,0,0}));
    assert(changed->pp[0]==StagedEditor::gen2MoveBasePP(84));
    assert(changed->pp[1]==StagedEditor::gen2MoveBasePP(85));
    assert(changed->dvs[0]==StagedEditor::derivedHPDV({9,8,7,6}));
    assert(changed->gender==static_cast<uint8_t>(genderFromAttackDV(changed->species,changed->dvs[1])));
    assert(changed->originalTrainer=="RED"&&changed->trainerId==4321&&changed->friendship==200);
    const auto* personal=personalRecord(26);assert(personal);
    assert(changed->experience==Pokemon::getExpForLevel(20,personal->experienceGrowth));
    assert(Pokemon::getLevelFromExp(changed->experience,personal->experienceGrowth)==20);

    // Invalid/malformed edits reject atomically.
    const auto preReject=std::vector<uint8_t>(editor->stagedBytes().begin(),editor->stagedBytes().end());
    BoxPokemonEdit bad;bad.nickname="ABCDEFGHIJK";assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.species=252;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.nickname="BAD@";assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.level=0;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.level=101;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.heldItem=6;assert(!editor->stageBoxPokemonEdit(0,0,bad,error)); // TERU-SAMA
    bad={};bad.moves=std::array<uint8_t,4>{252,0,0,0};assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.dvs=std::array<uint8_t,4>{16,1,1,1};assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),preReject.begin()));

    // Exact 10-character international nickname is accepted.
    BoxPokemonEdit maxName;maxName.nickname="ABCDEFGHIJ";assert(editor->stageBoxPokemonEdit(0,0,maxName,error));
    // Explicit PP/PP-Up validation.
    BoxPokemonEdit ppGood;ppGood.ppUps=std::array<uint8_t,4>{1,0,0,0};
    ppGood.pp=std::array<uint8_t,4>{static_cast<uint8_t>(StagedEditor::gen2MoveBasePP(84)*6/5),
                                     StagedEditor::gen2MoveBasePP(85),0,0};
    assert(editor->stageBoxPokemonEdit(0,0,ppGood,error));
    BoxPokemonEdit ppBad;ppBad.pp=std::array<uint8_t,4>{63,63,0,0};assert(!editor->stageBoxPokemonEdit(0,0,ppBad,error));

    // Shiny -> non-shiny preserves Attack DV/gender; non-shiny -> shiny uses real DV rule.
    auto beforeToggle=editor->boxedPokemon(0,0,error);assert(beforeToggle);
    assert(editor->stageBoxPokemonShiny(0,0,false,error));
    auto nonShiny=editor->boxedPokemon(0,0,error);assert(nonShiny&&!nonShiny->shiny);
    const auto genderBefore=nonShiny->gender;
    assert(editor->stageBoxPokemonShiny(0,0,true,error));
    auto shinyAgain=editor->boxedPokemon(0,0,error);assert(shinyAgain&&shinyAgain->shiny&&shinyAgain->gender==genderBefore);

    // A female 12.5%-female species with Attack DV 1 cannot preserve female while becoming shiny.
    BoxPokemonEdit female;female.species=1;female.dvs=std::array<uint8_t,4>{1,8,8,8};
    assert(editor->stageBoxPokemonEdit(1,0,female,error));
    auto femaleMon=editor->boxedPokemon(1,0,error);assert(femaleMon&&femaleMon->gender==1);
    const auto femaleBytes=std::vector<uint8_t>(editor->stagedBytes().begin(),editor->stagedBytes().end());
    assert(!editor->stageBoxPokemonShiny(1,0,true,error));assert(error.find("gender")!=std::string::npos);
    assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),femaleBytes.begin()));

    // Clone appends without touching source.
    auto sourceBeforeClone=editor->boxedPokemon(0,0,error);assert(sourceBeforeClone);
    size_t cloneSlot=99;assert(editor->stageCloneBoxPokemon(0,0,2,cloneSlot,error));assert(cloneSlot==0);
    auto clone=editor->boxedPokemon(2,0,error);assert(clone&&clone->nickname==sourceBeforeClone->nickname);
    auto sourceAfterClone=editor->boxedPokemon(0,0,error);assert(sourceAfterClone&&sourceAfterClone->nickname==sourceBeforeClone->nickname);

    // New boxed Pokemon is complete, level/EXP consistent, default OT/TID comes from save.
    BoxPokemonCreate created;created.species=158;created.level=5;created.nickname="TOTODILE";
    created.moves={10,43,0,0};created.dvs={8,8,8,8};created.friendship=70;
    size_t addSlot=99;assert(editor->stageAddBoxPokemon(3,created,addSlot,error));assert(addSlot==0);
    auto added=editor->boxedPokemon(3,0,error);assert(added&&added->species==158&&added->nickname=="TOTODILE");
    assert(added->originalTrainer=="ASH"&&added->trainerId==0x1234&&!added->shiny);
    const auto* createdPersonal=personalRecord(158);assert(createdPersonal);
    assert(added->experience==Pokemon::getExpForLevel(5,createdPersonal->experienceGrowth));
    assert((added->statExperience==std::array<uint16_t,5>{0,0,0,0,0}));

    BoxPokemonCreate defaultNamed=created;defaultNamed.species=25;defaultNamed.nickname.clear();
    size_t defaultSlot=99;assert(editor->stageAddBoxPokemon(5,defaultNamed,defaultSlot,error));
    auto defaultMon=editor->boxedPokemon(5,0,error);assert(defaultMon&&defaultMon->nickname=="Pikachu");
    BoxPokemonEdit genderGlyph;genderGlyph.nickname="NIDORAN\xE2\x99\x80";
    assert(editor->stageBoxPokemonEdit(5,0,genderGlyph,error));
    auto glyphMon=editor->boxedPokemon(5,0,error);assert(glyphMon&&glyphMon->nickname=="NIDORAN\xE2\x99\x80");
    genderGlyph.nickname="NIDORAN\xE2\x99\x82";
    assert(editor->stageBoxPokemonEdit(5,0,genderGlyph,error));
    glyphMon=editor->boxedPokemon(5,0,error);assert(glyphMon&&glyphMon->nickname=="NIDORAN\xE2\x99\x82");

    // Full box refuses add/clone and never overwrites.
    auto fullRaw=fixture(l);fullBox(fullRaw,4);checksum(fullRaw,l);
    auto fullParsed=parse(fullRaw,game);assert(fullParsed);auto fullEditor=StagedEditor::create(*fullParsed.save,error);assert(fullEditor);
    const auto fullBefore=std::vector<uint8_t>(fullEditor->stagedBytes().begin(),fullEditor->stagedBytes().end());
    size_t noSlot=99;assert(!fullEditor->stageAddBoxPokemon(4,created,noSlot,error));assert(error.find("full")!=std::string::npos);
    assert(!fullEditor->stageCloneBoxPokemon(0,0,4,noSlot,error));
    assert(std::equal(fullEditor->stagedBytes().begin(),fullEditor->stagedBytes().end(),fullBefore.begin()));

    // Final export: checksum pair, RTC preservation, strict reload, semantic values, counts/terminators.
    assert(!editor->pendingChanges().empty());
    auto out=editor->finalizedBytes(error);assert(!out.empty()&&error.empty());
    assert(out.size()==source.size());assert(le16(out,l.sum)==le16(out,l.sum2));
    assert(std::equal(out.begin()+0x8000,out.end(),source.begin()+0x8000));
    auto reload=parse(out,game);assert(reload);
    assert(reload.save->boxes()[2].slots[0]&&reload.save->boxes()[3].slots[0]);
    assert(out[boxStart(2)]==1&&out[boxStart(2)+2]==0xFF);
    assert(out[boxStart(3)]==1&&out[boxStart(3)+2]==0xFF);
    assert(source==raw); // caller source remains sacred throughout staged editing/export.

    // Isolated non-current boxed edit changes only that stored box after comparing normalized exports.
    auto baseRaw=fixture(l);auto baseParsed=parse(baseRaw,game);assert(baseParsed);
    auto baseEditor=StagedEditor::create(*baseParsed.save,error);assert(baseEditor);
    auto normalized=baseEditor->finalizedBytes(error);assert(!normalized.empty());
    BoxPokemonEdit isolated;isolated.nickname="ONLYBOX";assert(baseEditor->stageBoxPokemonEdit(1,0,isolated,error));
    auto isolatedOut=baseEditor->finalizedBytes(error);assert(!isolatedOut.empty());
    assert(differsOnlyInRange(normalized,isolatedOut,boxStart(1),boxStart(1)+listLen(20,32,11)));

    editor->discard();assert(!editor->hasPendingChanges());
    assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),source.begin()));
}
}

int main(){
    runFamily(GS,SourceGame::Gold);
    runFamily(GS,SourceGame::Silver);
    runFamily(C,SourceGame::Crystal);
    std::cout<<"Generation II boxed Pokemon staged editor tests: PASS\n";
}
