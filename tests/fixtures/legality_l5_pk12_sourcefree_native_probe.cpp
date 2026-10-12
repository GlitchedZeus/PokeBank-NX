// L5 independent *actual-production* PK1/PK2 source-free false-Invalid probe.
// Synthetic native-format wrappers, not authentic retail Pokémon. 12 contexts.
// Use --baseline to reproduce current defects; --after-fix after coordinator patch.
// Does NOT edit source saves, shared CI/Makefiles or Legality.cpp.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <string>
namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}
namespace {
bool invalid(const Legality::Report& report, std::string_view needle) {
    for(const auto& i:report.issues)
        if(i.severity==Legality::Severity::Invalid &&
           i.text.find(needle)!=std::string::npos) return true;
    return false;
}
bool warning(const Legality::Report& report,std::string_view needle) {
    for(const auto& i:report.issues)
        if(i.severity==Legality::Severity::Warning &&
           i.text.find(needle)!=std::string::npos) return true;
    return false;
}
PokeVault::Integration::Gen1::PokemonRecord pk1Record(uint16_t species) {
    using PokeVault::Integration::Gen1::PokemonRecord;
    PokemonRecord r{};
    r.species=species;
    r.rawSpecies=species==1?153:173; // Native Gen I internal species IDs.
    r.level=5;
    r.experience=135;
    r.trainerId=1234;
    r.catchRate=species==1?45:190;
    r.moves={33,0,0,0};
    r.pp={35,0,0,0};
    r.originalTrainer="L5";
    r.nickname=species==1?"BULBASAUR":"MAGNEMITE";
    r.rawBodySize=r.rawBody.size();
    return r;
}
PokeVault::Integration::Gen2::PokemonRecord pk2Record(uint16_t species) {
    using PokeVault::Integration::Gen2::PokemonRecord;
    PokemonRecord r{};
    r.species=species;
    r.level=5;
    r.experience=135;
    r.trainerId=1234;
    r.moves={33,0,0,0};
    r.pp={35,0,0,0};
    r.dvs[1]=15; // Bulbasaur DV-derived Male; Magnemite is genderless.
    r.originalTrainer="L5";
    r.nickname=species==1?"BULBASAUR":"MAGNEMITE";
    r.rawBodySize=r.rawBody.size();
    return r;
}
}
int main(int argc,char** argv) {
    if(argc!=2 || (std::string_view(argv[1])!="--baseline" &&
                   std::string_view(argv[1])!="--after-fix")) {
        std::cerr<<"usage: l5_pk12_probe --baseline | --after-fix\n";
        return 2;
    }
    const bool baseline=std::string_view(argv[1])=="--baseline";
    int checks=0;
    for(int gen:{1,2}) for(uint16_t species:{1u,81u}) {
        for(int label=0;label<3;++label) {
            const char* source=label==0?"":(label==1?(gen==1?"red_gb":"gold_gbc"):
                                                (gen==1?"platinum_nds":"firered_gba"));
            const bool unknownSource=label!=1;
            Legality::Report report;
            if(gen==1) {
                const Pokemon::Pokemon1ReadOnly pk(pk1Record(species));
                assert(pk.ability()==0 && pk.gender()==2);
                report=Legality::analyze(pk,pk.getGameGroup(),source);
            } else {
                const Pokemon::Pokemon2ReadOnly pk(pk2Record(species));
                assert(pk.ability()==0);
                report=Legality::analyze(pk,pk.getGameGroup(),source);
            }
            const bool ability=invalid(report,"Ability not legal for this species");
            const bool gender=invalid(report,"Gendered species marked genderless");
            assert(ability==(baseline && unknownSource));
            assert(gender==(baseline && unknownSource && gen==1 && species==1));
            if(label==2)
                assert(warning(report,"source-game label conflicts"));
            ++checks;
        }
    }
    assert(checks==12);
    std::cout<<"L5 actual PK1/PK2 source-free native analyzer 12 cases: "
             <<(baseline?"known false-Invalid baseline":"corrected guard")
             <<" PASS (synthetic records, no retail history)\n";
}