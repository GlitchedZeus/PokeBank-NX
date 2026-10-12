// Coordinator L3 production integration: REAL encrypted PK4 -> read-only
// native view -> Legality::analyze. No actual trainer decisions are inferred,
// source saves or Pokémon bytes are modified, and no parity is certified.
#include "Encryption/Encryption4.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}
namespace {
void wr16(std::vector<std::byte>& d, size_t at, uint16_t value) {
    d.at(at)=static_cast<std::byte>(value&0xFFu);
    d.at(at+1)=static_cast<std::byte>(value>>8);
}
void wr32(std::vector<std::byte>& d, size_t at, uint32_t value) {
    for(size_t i=0;i<4;++i)
        d.at(at+i)=static_cast<std::byte>((value>>(i*8))&0xFFu);
}
bool info(const Legality::Report& r, std::string_view needle) {
    for(const auto& i:r.issues)
        if(i.severity==Legality::Severity::Info &&
           i.text.find(needle)!=std::string::npos)return true;
    return false;
}
Legality::Report probe(uint16_t species,uint8_t origin,
    const std::array<uint16_t,4>& moves,uint32_t experience,
    Enums::GameVersion saveGroup=Enums::GameVersion::PT,
    std::string_view sourceLabel="platinum_nds",
    bool egg=false,uint8_t form=0,bool palPark=false) {
    std::vector<std::byte> plain(Encryption::SIZE_STORED4,std::byte{0});
    wr32(plain,0x00,0x12345678u);
    wr16(plain,0x08,species);
    wr16(plain,0x0C,12345);
    wr16(plain,0x0E,54321);
    wr32(plain,0x10,experience);
    plain[0x15]=std::byte{65}; // Bulbasaur Overgrow in native Platinum
    plain[0x17]=std::byte{2}; // English
    for(size_t i=0;i<moves.size();++i) {
        wr16(plain,0x28+i*2,moves[i]);
        plain[0x30+i]=moves[i]?std::byte{10}:std::byte{0};
    }
    wr32(plain,0x38,egg?0x40000000u:0u);
    plain[0x40]=static_cast<std::byte>(form<<3);
    plain[0x5F]=static_cast<std::byte>(origin);
    if(palPark) {
        wr16(plain,0x46,0x37); // Pt/HGSS Pal Park split-field marker
        wr16(plain,0x80,0x37);
    }
    plain[0x83]=std::byte{4};
    plain[0x84]=std::byte{5};
    const auto ciphertext=Encryption::encryptArray4(plain);
    const auto original=ciphertext;
    Pokemon::Pokemon4ReadOnly native(ciphertext,saveGroup);
    assert(native.valid() && native.checksumValid() && !native.empty());
    assert(native.originVersion()==origin);
    assert(native.species()==species);
    assert(native.moves()==moves);
    assert(native.isEgg()==egg);
    Pokemon::Pokemon4ReadOnlyView view(native);
    const auto report=Legality::analyze(view,saveGroup,sourceLabel);
    assert(ciphertext==original); // analyzer never touches the source
    return report;
}
}
int main() {
    const std::array<uint16_t,4> three{73,402,92,0}; // level, tutor, TM06
    const auto platinum=probe(1,12,three,10000);
    assert(info(platinum,"Original Platinum native level-up method:"));
    assert(info(platinum,"Original Platinum native tutor method:"));
    assert(info(platinum,"Original Platinum native TM/HM method:"));

    // Current save is Diamond, ORIGINAL native PK4 still identifies Platinum.
    const auto traded=probe(1,12,three,10000,
        Enums::GameVersion::DP,"diamond_nds");
    assert(info(traded,"Original Platinum native level-up method:"));
    assert(info(traded,"Original Platinum native tutor method:"));
    assert(info(traded,"Original Platinum native TM/HM method:"));

    // Do not infer original level-up teaching before its offer level.
    const auto lowLevel=probe(1,12,{73,0,0,0},0);
    assert(!info(lowLevel,"Original Platinum native level-up method:"));

    // Stored Diamond or Gen III origin must NEVER inherit Pt-only evidence.
    const auto diamond=probe(1,10,three,10000,
        Enums::GameVersion::PT,"platinum_nds");
    assert(!info(diamond,"Original Platinum native"));
    const auto palPark=probe(1,2,three,10000);
    assert(!info(palPark,"Original Platinum native"));

    // A Gen IV unhatched egg cannot supply direct historical teaching proof.
    const auto unhatched=probe(1,12,three,10000,
        Enums::GameVersion::PT,"platinum_nds",true);
    assert(!info(unhatched,"Original Platinum native"));

    // Unpinned species are Unknown, never fabricated source compatibility.
    const auto unmodeled=probe(152,12,three,10000);
    assert(!info(unmodeled,"Original Platinum native"));

    // The only source-pinned Deoxys row is normal form. All others unresolved.
    const auto formUnknown=probe(386,12,{33,0,0,0},10000,
        Enums::GameVersion::PT,"platinum_nds",false,1);
    assert(!info(formUnknown,"Original Platinum native"));
    // L3 original GBA tables now drive a separate native PK4 Pal Park
    // informational path only when BOTH fields and stored Gen III Ball agree.
    const auto migratedRuby=probe(1,2,{73,0,0,0},10000,
        Enums::GameVersion::PT,"platinum_nds",false,0,true);
    assert(info(migratedRuby,"Pal Park original ruby_gba level-up source opportunity:"));
    assert(!info(migratedRuby,"Original Platinum native"));

    // Later-gen moves can never be presented as originally learnable in GBA.
    const auto laterMove=probe(1,2,{412,0,0,0},10000,
        Enums::GameVersion::PT,"platinum_nds",false,0,true);
    assert(!info(laterMove,"Pal Park original ruby_gba level-up source opportunity:"));

    // Pal Park metadata, native source generation and stored origin all
    // matter. The same plausible move without a migration pattern proves
    // no retained original-era method.
    const auto noMarker=probe(1,2,{73,0,0,0},10000);
    assert(!info(noMarker,"Pal Park original"));
    const auto cubeOrigin=probe(1,15,{73,0,0,0},10000,
        Enums::GameVersion::PT,"platinum_nds",false,0,true);
    assert(!info(cubeOrigin,"Pal Park original"));
    const auto migratedUnmodeled=probe(152,2,{73,0,0,0},10000,
        Enums::GameVersion::PT,"platinum_nds",false,0,true);
    assert(!info(migratedUnmodeled,"Pal Park original ruby_gba level-up source opportunity:"));

    std::cout<<"L3 production Platinum and Pal Park native encrypted PK4 13-context INFO gate PASS\\n";
}
