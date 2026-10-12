// L5 Q12: exact native synthetic PK3/PK4 differential acceptance probe.
// This uses the production Legality::analyze and real NX native decoders;
// requires coordinator's existing host legality link graph to compile/run.
// NOT a user save, not a retail provenance claim, and NOT auto-wired to shared Makefiles.
// Usage: ./l5_ev_probe --baseline       // expected current accepted F1/F2 behavior
//        ./l5_ev_probe --after-fix      // required acceptance once production is corrected
#include "Encryption/Encryption4.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {
void wr16(std::vector<std::byte>& b,size_t at,uint16_t v) {
    b[at]=static_cast<std::byte>(v);
    b[at+1]=static_cast<std::byte>(v>>8);
}
void wr32(std::vector<std::byte>& b,size_t at,uint32_t v) {
    b[at]=static_cast<std::byte>(v);
    b[at+1]=static_cast<std::byte>(v>>8);
    b[at+2]=static_cast<std::byte>(v>>16);
    b[at+3]=static_cast<std::byte>(v>>24);
}
bool hasInvalidWith(const Legality::Report& r,std::string_view term1,
                    std::string_view term2={}) {
    for(const auto& issue:r.issues) {
        if(issue.severity!=Legality::Severity::Invalid)continue;
        std::string str=issue.text;
        for(char& ch:str)ch=static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch)));
        if(str.find(term1)!=std::string::npos &&
           (term2.empty()||str.find(term2)!=std::string::npos))return true;
    }
    return false;
}
Legality::Report pk3Probe(uint8_t value,bool egg,bool exactSource) {
    std::array<std::byte,80> encrypted{};
    Pokemon::Pokemon3FRLG pk{std::span<const std::byte>(encrypted)};
    pk.setPID(0x12345678u);
    pk.setTID16(12345);
    pk.setSID16(54321);
    pk.setLanguage(2);
    pk.setSpecies(1);
    pk.setOriginGame(4); // FRLG version field (held constant; source context independent).
    pk.setBall(4);
    pk.setLevel(5);
    pk.setOTName(u"RED");
    pk.setNickname(u"BULBASAUR");
    pk.setEgg(egg);
    if(egg)pk.setMetLevel(0);
    pk.setEV(0,value);
    pk.refreshChecksum();
    assert(pk.evHP()==value && pk.isEgg()==egg && pk.checksumValid());
    return exactSource
      ? Legality::analyze(pk,Enums::GameVersion::FRLG,"firered_gba")
      : Legality::analyze(pk,Enums::GameVersion::FRLG);
}
Legality::Report pk4Probe(uint8_t value,bool egg,bool exactSource) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,1); // Bulbasaur
    wr16(raw,0x0c,12345);
    wr16(raw,0x0e,54321);
    wr32(raw,0x10,135);
    raw[0x15]=std::byte{65}; // Overgrow
    raw[0x17]=std::byte{2};
    raw[0x18]=static_cast<std::byte>(value);
    raw[0x5f]=std::byte{12}; // Platinum
    raw[0x83]=std::byte{4};
    if(egg) {
        wr32(raw,0x38,0x40000000u);
        wr16(raw,0x7e,300); // synthetic egg-origin location
        raw[0x84]=std::byte{0};
    } else raw[0x84]=std::byte{5};
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly source(encrypted,Enums::GameVersion::PT);
    assert(source.valid() && source.checksumValid() && !source.empty());
    assert(source.evs()[0]==value && source.isEgg()==egg);
    Pokemon::Pokemon4ReadOnlyView view(source);
    return exactSource
      ? Legality::analyze(view,Enums::GameVersion::PT,"platinum_nds")
      : Legality::analyze(view,Enums::GameVersion::PT);
}
}

int main(int argc,char** argv) {
    if(argc!=2 || (std::string_view(argv[1])!="--baseline" &&
                   std::string_view(argv[1])!="--after-fix")) {
        std::cerr<<"usage: l5_ev_probe --baseline | --after-fix\n";
        return 2;
    }
    const bool baseline=std::string_view(argv[1])=="--baseline";
    int assertions=0;
    for(bool exactSource:{false,true})for(int gen:{3,4}) {
        for(int ev:{0,1,252,253,254,255}) {
            const Legality::Report r=gen==3
              ? pk3Probe(static_cast<uint8_t>(ev),false,exactSource)
              : pk4Probe(static_cast<uint8_t>(ev),false,exactSource);
            const bool badCap=hasInvalidWith(r,"ev over 252");
            // Baseline faithfully reproduces the source bug. After-fix must
            // pass 253-255 without THIS spurious Invalid (other findings not masked).
            assert(badCap==(baseline && ev>252));
            ++assertions;
        }
        for(int ev:{0,1,100}) {
            const Legality::Report r=gen==3
              ? pk3Probe(static_cast<uint8_t>(ev),true,exactSource)
              : pk4Probe(static_cast<uint8_t>(ev),true,exactSource);
            const bool eggEV = hasInvalidWith(r,"egg","ev") ||
                               hasInvalidWith(r,"egg","effort") ||
                               hasInvalidWith(r,"ev","egg");
            assert(eggEV==(!baseline && ev>0));
            ++assertions;
        }
    }
    assert(assertions==36);
    std::cout<<"L5 Q12 strict native PK3/PK4 analyzer EV probe: "
             <<assertions<<" checks, "
             <<(baseline?"baseline expected mismatch":"corrected acceptance")
             <<" PASS (synthetic specimens)\n";
}
