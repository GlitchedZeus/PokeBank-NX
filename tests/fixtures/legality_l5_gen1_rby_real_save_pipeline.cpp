// L5 independent end-to-end Gen I RBY QA over synthetic checksum-valid saves.
// Input bytes are freshly generated. No retail provenance is authenticated.
// L5_FIXTURE_ONLY compiles the EXACT same generator without production linkage;
// the ordinary CI build links the real Gen1ReadOnlySave and Legality::analyze.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>
#ifndef L5_FIXTURE_ONLY
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
namespace Trainer {
const char* getSpeciesName(uint16_t id){return Names::getSpeciesName(id);}
const char* getItemName(uint16_t id){return Names::getItemName(id);}
}
#endif
#ifdef NDEBUG
#error "L5 RBY parsed-save QA must never disable assertions"
#endif
namespace {
unsigned long long checks=0;
#define CHECK(x) do { ++checks; if(!(x)) { \
  std::cerr<<"L5 RBY parsed-save invariant failed at line "<<__LINE__<<'\n'; \
  std::abort(); }} while(false)
struct Layout {
    unsigned money,tid,currentBoxIndex,party,currentBox,checksum,mainLength;
    unsigned starter,friendship,boxCapacity,stringLength;
};
// Source pinned to PKHeX SAV1Offsets and PKSM-Core Sav1 and independently
// confirmed against production Gen1ReadOnlySave.cpp; not copied via header.
constexpr Layout intl{0x25F3,0x2605,0x284C,0x2F2C,0x30C0,0x3523,0x0F8B,
                      0x29C3,0x271C,20,11};
constexpr Layout jpn {0x25EE,0x25FB,0x2842,0x2ED5,0x302D,0x3594,0x0FFC,
                      0x29B9,0x2712,30,6};
constexpr unsigned saveSize=0x8000,mainStart=0x2598,partyCapacity=6;
constexpr std::array<unsigned,5> stats{0,1,32768,65534,65535};
// ORIGINAL pinned kwsch/PKHeX@6501f0ab PKHeX.Core/PKM/Util/Conversion/SpeciesConverter.cs
// National->Gen I internal ID table, index 0..151; a source snapshot, not a fabricated encounter.
constexpr std::array<uint8_t,152> pkhexRawSpecies{{
    0x00, 0x99, 0x09, 0x9a, 0xb0, 0xb2, 0xb4, 0xb1,
    0xb3, 0x1c, 0x7b, 0x7c, 0x7d, 0x70, 0x71, 0x72,
    0x24, 0x96, 0x97, 0xa5, 0xa6, 0x05, 0x23, 0x6c,
    0x2d, 0x54, 0x55, 0x60, 0x61, 0x0f, 0xa8, 0x10,
    0x03, 0xa7, 0x07, 0x04, 0x8e, 0x52, 0x53, 0x64,
    0x65, 0x6b, 0x82, 0xb9, 0xba, 0xbb, 0x6d, 0x2e,
    0x41, 0x77, 0x3b, 0x76, 0x4d, 0x90, 0x2f, 0x80,
    0x39, 0x75, 0x21, 0x14, 0x47, 0x6e, 0x6f, 0x94,
    0x26, 0x95, 0x6a, 0x29, 0x7e, 0xbc, 0xbd, 0xbe,
    0x18, 0x9b, 0xa9, 0x27, 0x31, 0xa3, 0xa4, 0x25,
    0x08, 0xad, 0x36, 0x40, 0x46, 0x74, 0x3a, 0x78,
    0x0d, 0x88, 0x17, 0x8b, 0x19, 0x93, 0x0e, 0x22,
    0x30, 0x81, 0x4e, 0x8a, 0x06, 0x8d, 0x0c, 0x0a,
    0x11, 0x91, 0x2b, 0x2c, 0x0b, 0x37, 0x8f, 0x12,
    0x01, 0x28, 0x1e, 0x02, 0x5c, 0x5d, 0x9d, 0x9e,
    0x1b, 0x98, 0x2a, 0x1a, 0x48, 0x35, 0x33, 0x1d,
    0x3c, 0x85, 0x16, 0x13, 0x4c, 0x66, 0x69, 0x68,
    0x67, 0xaa, 0x62, 0x63, 0x5a, 0x5b, 0xab, 0x84,
    0x4a, 0x4b, 0x49, 0x58, 0x59, 0x42, 0x83, 0x15
}};

unsigned be16(const std::vector<uint8_t>& b,unsigned o) {
    return (static_cast<unsigned>(b[o])<<8)|b[o+1];
}
unsigned be24(const std::vector<uint8_t>& b,unsigned o) {
    return (static_cast<unsigned>(b[o])<<16)|(static_cast<unsigned>(b[o+1])<<8)|b[o+2];
}
void write16(std::vector<uint8_t>& b,unsigned o,unsigned v){
    b[o]=static_cast<uint8_t>(v>>8);b[o+1]=static_cast<uint8_t>(v);
}
void write24(std::vector<uint8_t>& b,unsigned o,unsigned v){
    b[o]=static_cast<uint8_t>(v>>16);b[o+1]=static_cast<uint8_t>(v>>8);
    b[o+2]=static_cast<uint8_t>(v);
}
uint8_t diff8(std::span<const uint8_t> b){
    unsigned value=0xff;
    for(auto ch:b)value=(value-ch)&0xffu;
    return static_cast<uint8_t>(value);
}
void checksum(std::vector<uint8_t>& b,const Layout& l){
    b[l.checksum]=diff8(std::span<const uint8_t>(b).subspan(mainStart,l.mainLength));
}
std::vector<uint8_t> generate(const Layout& l,int game,uint8_t species,
                              uint16_t dv,uint8_t move,unsigned rotation){
    std::vector<uint8_t> b(saveSize,0);
    // Valid packed-BCD money and minimal international/Japanese name glyphs.
    b[mainStart]=0x80;b[mainStart+1]=0x50;
    b[l.money]=0x12;b[l.money+1]=0x34;b[l.money+2]=0x56;
    write16(b,l.tid,1234);
    b[l.starter]=game==2?0x54:0x99; // Yellow vs Red/Blue family evidence
    b[l.friendship]=0;
    b[l.currentBoxIndex]=0; // uninitialized bank; only current box checked
    b[l.currentBox]=0;b[l.currentBox+1]=0xff;
    b[l.party]=1;b[l.party+1]=species;b[l.party+2]=0xff;
    const unsigned o=l.party+partyCapacity+2;
    b[o]=species;write16(b,o+1,30);b[o+3]=5;
    b[o+5]=0x14;b[o+6]=0x14;b[o+7]=45;
    b[o+8]=move;write16(b,o+0x0c,1234);
    write24(b,o+0x0e,135);
    for(unsigned i=0;i<5;++i)write16(b,o+0x11+i*2,stats[(rotation+i)%5]);
    write16(b,o+0x1b,dv);
    b[o+0x1d]=static_cast<uint8_t>(0xc0|35);
    b[o+0x21]=5;
    for(unsigned i=0;i<5;++i)write16(b,o+0x22+i*2,40+i);
    const unsigned ot=o+partyCapacity*44;
    const unsigned nickname=ot+partyCapacity*l.stringLength;
    b[ot]=0x80;b[ot+1]=0x50;
    b[nickname]=0x81;b[nickname+1]=0x50;
    checksum(b,l);
    return b;
}
void inspectRaw(const std::vector<uint8_t>& b,const Layout& l,int game,
                uint8_t rawSpecies,uint16_t dv,uint8_t move,unsigned rotation){
    CHECK(b.size()==saveSize);
    CHECK(b[l.checksum]==diff8(std::span<const uint8_t>(b).subspan(mainStart,l.mainLength)));
    CHECK(b[l.starter]==(game==2?0x54:0x99));
    CHECK(b[l.party]==1 && b[l.party+1]==rawSpecies && b[l.party+2]==0xff);
    CHECK(b[l.currentBox]==0 && b[l.currentBox+1]==0xff);
    const unsigned o=l.party+8;
    CHECK(b[o]==rawSpecies && b[o+8]==move);
    CHECK(be16(b,o+0x0c)==1234 && be24(b,o+0x0e)==135);
    CHECK(be16(b,o+0x1b)==dv);
    for(unsigned i=0;i<5;++i)CHECK(be16(b,o+0x11+i*2)==stats[(rotation+i)%5]);
    CHECK((b[o+0x1d]&0x3f)==35 && (b[o+0x1d]>>6)==3);
    CHECK(b[o+0x21]==5);
}
#ifndef L5_FIXTURE_ONLY
bool issue(const Legality::Report& r,Legality::Severity severity,std::string_view needle){
    for(const auto& item:r.issues)
        if(item.severity==severity && item.text.find(needle)!=std::string::npos)return true;
    return false;
}
void runReal(const std::vector<uint8_t>& b,const Layout& l,int game,
             unsigned national,uint16_t dv,uint8_t move,unsigned rotation){
    using namespace PokeVault::Integration::Gen1;
    const SourceGame label=game==0?SourceGame::Red:game==1?SourceGame::Blue:SourceGame::Yellow;
    const auto parsed=parse(b,label);
    CHECK(parsed && parsed.error==SaveError::None && parsed.save);
    CHECK(parsed.save->sourceBytes().size()==b.size());
    CHECK(std::equal(parsed.save->sourceBytes().begin(),parsed.save->sourceBytes().end(),b.begin()));
    CHECK(parsed.save->party().size()==1);
    const auto& r=parsed.save->party().front();
    CHECK(r.species==national && r.level==5 && r.experience==135);
    CHECK(r.partyRecord && r.rawBodySize==44 && r.rawBody[8]==move);
    CHECK(r.moves[0]==move && r.pp[0]==35 && r.ppUps[0]==3);
    CHECK(r.rawBody[0]==b[l.party+8]);
    const unsigned attack=(dv>>12)&15, defense=(dv>>8)&15,
                   speed=(dv>>4)&15,special=dv&15;
    const unsigned hp=((attack&1)<<3)|((defense&1)<<2)|((speed&1)<<1)|(special&1);
    CHECK((r.dvs==std::array<uint8_t,5>{static_cast<uint8_t>(hp),static_cast<uint8_t>(attack),
                                       static_cast<uint8_t>(defense),static_cast<uint8_t>(speed),
                                       static_cast<uint8_t>(special)}));
    for(unsigned i=0;i<5;++i)CHECK(r.statExperience[i]==stats[(rotation+i)%5]);
    const Pokemon::Pokemon1ReadOnly pk(r);
    CHECK(pk.ability()==0 && pk.gender()==2 && pk.heldItem()==0);
    CHECK(pk.statExpHP()==r.statExperience[0] && pk.statExpSpecial()==r.statExperience[4]);
    // Four independently observed contexts. A native PK1 has no original
    // Red/Blue/Yellow game label; even a checksum-valid synthetic save
    // supplies a candidate source, not proof of an authentic retail history.
    // Missing or conflicting source must not convert absent Gen I ability
    // and gender into false Invalid. A genuine out-of-gen move stays Invalid.
    const std::array<std::string_view,4> sources{
        sourceGameId(label), "", "platinum_nds", "unknown_source"};
    for(const auto source:sources){
        const auto report=Legality::analyze(pk,pk.getGameGroup(),source);
        CHECK(!issue(report,Legality::Severity::Invalid,"Ability not legal for this species"));
        CHECK(!issue(report,Legality::Severity::Invalid,"Gendered species marked genderless"));
        CHECK(!issue(report,Legality::Severity::Invalid,"EV over 252"));
        CHECK(!issue(report,Legality::Severity::Invalid,"EV total over 510"));
        CHECK(issue(report,Legality::Severity::Invalid,"cannot exist in a Generation")== (move==200));
        CHECK(issue(report,Legality::Severity::Warning,"source-game label conflicts")==
              (source=="platinum_nds"));
    }
}
void negatives(const std::vector<uint8_t>& original,const Layout& l,int game){
    using namespace PokeVault::Integration::Gen1;
    const SourceGame label=game==0?SourceGame::Red:game==1?SourceGame::Blue:SourceGame::Yellow;
    auto fail=original;fail[l.party+8+0x21]=101;checksum(fail,l);
    const auto badLevel=parse(fail,label);
    CHECK(!badLevel && badLevel.error==SaveError::InvalidParty);
    fail=original;fail[l.party+8]^=1;checksum(fail,l);
    const auto badMarker=parse(fail,label);
    CHECK(!badMarker && badMarker.error==SaveError::InvalidParty);
    fail=original;fail[l.party+8]^=1;
    const auto badChecksum=parse(fail,label);
    CHECK(!badChecksum && badChecksum.error==SaveError::ChecksumMismatch);
    const SourceGame opposite=game==2?SourceGame::Red:SourceGame::Yellow;
    const auto conflict=parse(original,opposite);
    CHECK(!conflict && conflict.error==SaveError::GameHintMismatch);
    // All malformed/contradictory saves above must leave their caller bytes unchanged.
    CHECK(original[l.checksum]==diff8(std::span<const uint8_t>(original).subspan(mainStart,l.mainLength)));
}
#endif
}
int main(){
#ifndef L5_FIXTURE_ONLY
    using PokeVault::Integration::Gen1::gen1InternalToNational;
#endif
    unsigned cases=0,speciesTested=0;
#ifndef L5_FIXTURE_ONLY
    unsigned negativesTested=0;
#endif
    constexpr std::array<uint16_t,3> dvs{0,0x5555,0xffff};
    for(unsigned national=1;national<=151;++national){
        const unsigned raw=pkhexRawSpecies[national];
#ifndef L5_FIXTURE_ONLY
        unsigned matches=0;
        for(unsigned id=1;id<=255;++id){
            if(gen1InternalToNational(static_cast<uint8_t>(id))==national){
                CHECK(id==raw);++matches;
            }
        }
        CHECK(matches==1 && gen1InternalToNational(static_cast<uint8_t>(raw))==national);
#else
        // The standalone generator now varies all 151 independent pinned original IDs.
        CHECK(raw>0);
#endif
        ++speciesTested;
        for(const auto& l:{intl,jpn})for(int game=0;game<3;++game)
          for(auto dv:dvs)for(uint8_t move:{uint8_t{33},uint8_t{200}}){
            const unsigned rotation=cases%5;
            const auto b=generate(l,game,static_cast<uint8_t>(raw),dv,move,rotation);
            inspectRaw(b,l,game,static_cast<uint8_t>(raw),dv,move,rotation);
#ifndef L5_FIXTURE_ONLY
            runReal(b,l,game,national,dv,move,rotation);
            if(national==1 && dv==0 && move==33){negatives(b,l,game);++negativesTested;}
#endif
            ++cases;
          }
    }
    CHECK(speciesTested==151 && cases==5436);
#ifndef L5_FIXTURE_ONLY
    CHECK(negativesTested==6);
    std::cout<<"L5 Gen I actual saved-byte parser -> PK1 -> Legality analyzer: "<<cases
             <<" saved-byte cases x 4 source labels; "<<negativesTested
             <<" negative groups PASS (synthetic provenance Unknown)\n";
#else
    std::cout<<"L5 Gen I independent checksum-valid save generator: "<<cases
             <<" cases and "<<checks<<" byte assertions PASS (NOT production analyzer)\n";
#endif
}
