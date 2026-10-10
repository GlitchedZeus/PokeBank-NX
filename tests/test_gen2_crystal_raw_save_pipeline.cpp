#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t s) { return Names::getSpeciesName(s); }
const char* getItemName(uint16_t i) { return Names::getItemName(i); }
}

namespace {
using namespace PokeVault::Integration::Gen2;
constexpr size_t kSize=0x8000, kParty=0x2865, kEnd=0x2B82;
constexpr size_t kChecksum=0x2D0D, kBoxCapacity=20, kBoxCount=14;
constexpr size_t kBoxesPerBank=7, kText=11, kBoxNameLength=9;
constexpr size_t kCurrentBoxCopy=0x2D10;
constexpr size_t kBoxListLength=1+(kBoxCapacity+1)+kBoxCapacity*32+
                                2*kBoxCapacity*kText;
constexpr size_t kBoxStride=kBoxListLength+2; // Retail padding bytes.
static_assert(kBoxesPerBank*kBoxStride<=0x2000);
void putName(std::vector<uint8_t>& b,size_t at,size_t len,char c) {
    std::fill_n(b.begin()+at,len,0x50);
    if(c>='A'&&c<='Z')b[at]=static_cast<uint8_t>(0x80+c-'A');
}
void checksum(std::vector<uint8_t>& data) {
    uint16_t v=0;
    for(size_t i=0x2009;i<=kEnd;++i)v=static_cast<uint16_t>(v+data[i]);
    data[kChecksum]=static_cast<uint8_t>(v&0xFF);
    data[kChecksum+1]=static_cast<uint8_t>(v>>8);
}
void be16(std::vector<uint8_t>& d,size_t at,uint16_t v) {
    d[at]=static_cast<uint8_t>(v>>8);
    d[at+1]=static_cast<uint8_t>(v);
}
std::vector<uint8_t> crystalSave(uint16_t species,uint8_t level,
                                 uint16_t caught,bool egg=false,
                                 bool shiny=true) {
    // Pinned read-only parser: international Crystal, 32-KiB bank,
    // 6-party slots, 20 slots per box, 14 boxes, 11-byte names.
    std::vector<uint8_t> raw(kSize,0);
    be16(raw,0x2009,0x1234);
    putName(raw,0x200B,11,'A');
    raw[0x2700]=0; // current box
    raw[0x3E3D]=1; // Crystal trainer gender
    raw[kCurrentBoxCopy]=0; // Empty current-box backup list
    raw[kCurrentBoxCopy+1]=0xFF;
    for(size_t b=0;b<kBoxCount;++b) {
        const size_t at=b<kBoxesPerBank ? 0x4000+b*kBoxStride :
                             0x6000+(b-kBoxesPerBank)*kBoxStride;
        assert(at+kBoxListLength<=raw.size());
        raw[at]=0;
        raw[at+1]=0xFF;
        putName(raw,0x2703+b*kBoxNameLength,kBoxNameLength,
                static_cast<char>('A'+b%26));
    }
    // One party entry; the stored party body must remain 48 bytes.
    raw[kParty]=1;
    raw[kParty+1]=egg ? 0xFD : static_cast<uint8_t>(species);
    raw[kParty+2]=0xFF;
    const size_t body=kParty+1+7;
    raw[body]=static_cast<uint8_t>(species);
    raw[body+1]=1;      // held item
    raw[body+2]=33;     // Tackle
    be16(raw,body+6,0x1234);
    const uint32_t xp=static_cast<uint32_t>(level)*level*level;
    raw[body+8]=static_cast<uint8_t>((xp>>16)&0xFF);
    raw[body+9]=static_cast<uint8_t>((xp>>8)&0xFF);
    raw[body+10]=static_cast<uint8_t>(xp&0xFF);
    // Gen II shiny is represented by DVs, not a separate flag:
    // attack 7, defense/speed/special 10. Set defense 0 as non-shiny.
    raw[body+21]=shiny ? 0x7A : 0x70;
    raw[body+22]=0xAA;
    raw[body+23]=35;
    raw[body+27]=70;
    be16(raw,body+29,caught);
    raw[body+31]=level;
    raw[body+32]=0; // status
    for(int at=34;at<=46;at+=2)
        be16(raw,body+static_cast<size_t>(at),20);
    putName(raw,body+6*48,11,'A');
    putName(raw,body+6*48+6*11,11,'B');
    checksum(raw);
    return raw;
}
bool hasInfo(const Legality::Report& r,const std::string& fragment) {
    for(const auto& issue:r.issues)
        if(issue.severity==Legality::Severity::Info &&
           issue.text.find(fragment)!=std::string::npos)return true;
    return false;
}
Legality::Report analyzeRawSave(uint16_t species,uint8_t level,
                                uint16_t caught,bool egg=false,
                                bool shiny=true) {
    const auto data=crystalSave(species,level,caught,egg,shiny);
    const auto original=data;
    const auto parsed=parse(data,SourceGame::Crystal);
    assert(parsed && parsed.save);
    assert(parsed.save->metadata().sourceGameId=="crystal_gbc");
    assert(parsed.save->metadata().boxCount==kBoxCount);
    assert(parsed.save->metadata().boxCapacity==kBoxCapacity);
    assert(parsed.save->boxes().size()==kBoxCount);
    assert(parsed.save->party().size()==1);
    assert(parsed.save->sourceBytes().size()==original.size());
    assert(std::equal(parsed.save->sourceBytes().begin(),
                      parsed.save->sourceBytes().end(),original.begin()));
    const Pokemon::Pokemon2ReadOnly pk(parsed.save->party().front());
    assert(pk.speciesID()==species&&pk.level()==level);
    assert(pk.caughtData()==caught&&pk.isEgg()==egg);
    assert(pk.isShiny(pk.id32(), {}) == shiny);
    assert(pk.isPartyRecord());
    auto report=Legality::analyze(
        pk,Enums::GameVersion::GSC,parsed.save->metadata().sourceGameId);
    // Strict read-only parsing and the full legality report must NEVER
    // mutate either its input or the retained original source copy.
    assert(data==original);
    assert(std::equal(parsed.save->sourceBytes().begin(),
                      parsed.save->sourceBytes().end(),original.begin()));
    return report;
}
}

int main() {
    constexpr auto evolved="PK2 caught-data is compatible with a pinned Crystal wild pre-evolution capture";
    constexpr auto direct="PK2 caught-data location/level/time matches a pinned Crystal wild encounter slot";
    // Actual pinned Crystal Pidgey #16 at location2, level2, daytime.
    constexpr uint16_t pidgey=(1u<<14)|(2u<<8)|2u;
    assert(hasInfo(analyzeRawSave(16,2,pidgey),direct));
    assert(hasInfo(analyzeRawSave(17,18,pidgey),evolved));
    assert(hasInfo(analyzeRawSave(18,36,pidgey),evolved));
    assert(!hasInfo(analyzeRawSave(18,35,pidgey),evolved));
    assert(!hasInfo(analyzeRawSave(18,36,(3u<<14)|(2u<<8)|2u),evolved));
    assert(!hasInfo(analyzeRawSave(18,36,(1u<<14)|(2u<<8)|127u),evolved));
    assert(!hasInfo(analyzeRawSave(16,2,pidgey,true),direct));
    assert(!hasInfo(analyzeRawSave(18,36,pidgey,true),evolved));
    // Actual pinned Crystal Caterpie #10, location4, level3/daytime.
    // Butterfree needs two separate level ups after that capture.
    constexpr uint16_t caterpie=(1u<<14)|(3u<<8)|4u;
    assert(hasInfo(analyzeRawSave(11,7,caterpie),evolved));
    assert(hasInfo(analyzeRawSave(12,10,caterpie),evolved));
    assert(!hasInfo(analyzeRawSave(12,9,caterpie),evolved));
    // Pinned Crystal Eevee gift #133 location16 level20, later evolved.
    constexpr uint16_t eeveeGift=(1u<<14)|(20u<<8)|16u;
    constexpr auto eeveeText="PK2 caught-data is compatible with Crystal's pinned level-20 Eevee gift ancestor";
    assert(hasInfo(analyzeRawSave(134,20,eeveeGift),eeveeText));
    assert(hasInfo(analyzeRawSave(135,20,eeveeGift),eeveeText));
    assert(hasInfo(analyzeRawSave(136,20,eeveeGift),eeveeText));
    assert(hasInfo(analyzeRawSave(196,21,eeveeGift),eeveeText));
    assert(hasInfo(analyzeRawSave(197,21,eeveeGift),eeveeText));
    assert(!hasInfo(analyzeRawSave(196,20,eeveeGift),eeveeText));
    assert(!hasInfo(analyzeRawSave(196,21,eeveeGift,true),eeveeText));
    assert(!hasInfo(analyzeRawSave(196,21,(1u<<14)|(20u<<8)|17u),eeveeText));

    // Crystal's three pinned level-5 Johto starter gifts at location 1:
    // intact met-data may remain compatible with an evolved starter.
    constexpr uint16_t starterGift = (1u << 14) | (5u << 8) | 1u;
    constexpr auto starterText = "PK2 caught-data is compatible with a pinned Crystal level-5 starter gift ancestor";
    assert(hasInfo(analyzeRawSave(153, 16, starterGift), starterText));
    assert(hasInfo(analyzeRawSave(154, 32, starterGift), starterText));
    assert(hasInfo(analyzeRawSave(156, 14, starterGift), starterText));
    assert(hasInfo(analyzeRawSave(157, 36, starterGift), starterText));
    assert(hasInfo(analyzeRawSave(159, 18, starterGift), starterText));
    assert(hasInfo(analyzeRawSave(160, 30, starterGift), starterText));
    assert(!hasInfo(analyzeRawSave(154, 31, starterGift), starterText));
    assert(!hasInfo(analyzeRawSave(157, 36, starterGift, true), starterText));
    assert(!hasInfo(analyzeRawSave(160, 30, (1u << 14) | (5u << 8) | 2u), starterText));

    // Source-pinned Crystal Dragon's Den Dratini gift (#147): level 15,
    // location 42; valid Dragonair and Dragonite retain that caught-data.
    constexpr uint16_t dratiniGift = (1u << 14) | (15u << 8) | 42u;
    constexpr auto dratiniText = "PK2 caught-data is compatible with Crystal's pinned level-15 Dratini gift ancestor";
    assert(hasInfo(analyzeRawSave(148,30,dratiniGift),dratiniText));
    assert(hasInfo(analyzeRawSave(149,55,dratiniGift),dratiniText));
    assert(!hasInfo(analyzeRawSave(148,29,dratiniGift),dratiniText));
    assert(!hasInfo(analyzeRawSave(149,54,dratiniGift),dratiniText));
    assert(!hasInfo(analyzeRawSave(149,55,dratiniGift,true),dratiniText));
    assert(!hasInfo(analyzeRawSave(149,55,(1u << 14)|(15u << 8)|41u),dratiniText));
    assert(!hasInfo(analyzeRawSave(149,55,(1u << 14)|(16u << 8)|42u),dratiniText));

    // Pinned Crystal Lake of Rage fixed Red Gyarados #130 at level 30,
    // location 38: the static source mandates Gen II shiny DVs.
    constexpr uint16_t redGyarados = (1u << 14) | (30u << 8) | 38u;
    constexpr auto staticText = "PK2 data is compatible with a pinned Generation II static/gift encounter";
    assert(hasInfo(analyzeRawSave(130,30,redGyarados,false,true),staticText));
    assert(!hasInfo(analyzeRawSave(130,30,redGyarados,false,false),staticText));
    assert(!hasInfo(analyzeRawSave(130,29,redGyarados,false,true),staticText));
    assert(!hasInfo(analyzeRawSave(130,30,redGyarados,true,true),staticText));
    assert(!hasInfo(analyzeRawSave(130,30,(1u<<14)|(29u<<8)|38u,false,true),staticText));
    assert(!hasInfo(analyzeRawSave(130,30,(1u<<14)|(30u<<8)|39u,false,true),staticText));

    // Pinned Crystal level-10 Tyrogue gift, location 35; evolved
    // Hitmonlee, Hitmonchan and Hitmontop remain possible histories.
    constexpr uint16_t tyrogueGift = (1u<<14)|(10u<<8)|35u;
    constexpr auto tyrogueText = "PK2 caught-data is compatible with Crystal's pinned level-10 Tyrogue gift ancestor";
    assert(hasInfo(analyzeRawSave(106,20,tyrogueGift),tyrogueText));
    assert(hasInfo(analyzeRawSave(107,20,tyrogueGift),tyrogueText));
    assert(hasInfo(analyzeRawSave(237,20,tyrogueGift),tyrogueText));
    assert(hasInfo(analyzeRawSave(237,45,tyrogueGift,false,false),tyrogueText));
    assert(!hasInfo(analyzeRawSave(106,19,tyrogueGift),tyrogueText));
    assert(!hasInfo(analyzeRawSave(107,20,tyrogueGift,true),tyrogueText));
    assert(!hasInfo(analyzeRawSave(237,20,(1u<<14)|(9u<<8)|35u),tyrogueText));
    assert(!hasInfo(analyzeRawSave(237,20,(1u<<14)|(10u<<8)|34u),tyrogueText));
    assert(!hasInfo(analyzeRawSave(236,20,tyrogueGift),tyrogueText));

    // Actual pinned Crystal Sentret #161, location2, level2/daytime.
    constexpr uint16_t sentret=(1u<<14)|(2u<<8)|2u;
    assert(hasInfo(analyzeRawSave(162,15,sentret),evolved));
    assert(!hasInfo(analyzeRawSave(162,14,sentret),evolved));
}
