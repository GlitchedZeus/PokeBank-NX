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

    // Once the coordinator has applied the native-format guards, exercise
    // source-identity invariance over different DVs/levels, additional
    // native Gen II gender classes, and deliberately contradictory labels.
    // These are synthetic native wrappers: no source save is edited.
    if (!baseline) {
        const std::array<std::string_view,5> gen1Sources{
            "", "red_gb", "blue_gb", "platinum_nds", "unrecognized_source"
        };
        const std::array<std::string_view,5> gen2Sources{
            "", "gold_gbc", "crystal_gbc", "firered_gba", "unrecognized_source"
        };
        for (uint16_t species : {1u,81u}) {
            for (uint8_t level : {5u,20u,50u,100u}) {
                auto rec = pk1Record(species);
                rec.level = level;
                const Pokemon::Pokemon1ReadOnly pk(rec);
                assert(pk.ability()==0 && pk.gender()==2);
                for (size_t label=0;label<gen1Sources.size();++label) {
                    const auto report = Legality::analyze(
                        pk,pk.getGameGroup(),gen1Sources[label]);
                    assert(!invalid(report,"Ability not legal for this species"));
                    assert(!invalid(report,"Gendered species marked genderless"));
                    if (label==3)
                        assert(warning(report,"source-game label conflicts"));
                    else
                        assert(!warning(report,"source-game label conflicts"));
                    ++checks;
                }
            }
        }
        // Bulbasaur (mixed), Nidoran F/M and Chansey/Tauros (single-gender),
        // Magnemite/Unown (genderless). The Gen II wrapper's DV-derived
        // gender must not be confused with Gen I's absent gender field.
        for (uint16_t species : {1u,29u,32u,81u,113u,128u,201u}) {
            for (uint8_t attackDv : {0u,7u,8u,15u}) {
                auto rec = pk2Record(species);
                rec.dvs[1] = attackDv;
                const Pokemon::Pokemon2ReadOnly pk(rec);
                assert(pk.ability()==0);
                if (species==29 || species==113) assert(pk.gender()==1);
                if (species==32 || species==128) assert(pk.gender()==0);
                if (species==81 || species==201) assert(pk.gender()==2);
                for (size_t label=0;label<gen2Sources.size();++label) {
                    const auto report = Legality::analyze(
                        pk,pk.getGameGroup(),gen2Sources[label]);
                    assert(!invalid(report,"Ability not legal for this species"));
                    assert(!invalid(report,"Gendered species marked genderless"));
                    assert(!invalid(report,"Female-only species set to male"));
                    assert(!invalid(report,"Male-only species set to female"));
                    assert(!invalid(report,"Genderless species but a gender is set"));
                    if (label==3)
                        assert(warning(report,"source-game label conflicts"));
                    else
                        assert(!warning(report,"source-game label conflicts"));
                    ++checks;
                }
            }
        }
        // Negative controls: real native Gen I/II move-domain contradictions
        // must remain Invalid even with absent or wrong-generation labels.
        for (int gen : {1,2}) for (int label : {0,1,2}) {
            const std::string_view source = label==0 ? std::string_view{} :
                label==1 ? (gen==1 ? "red_gb" : "gold_gbc") :
                           (gen==1 ? "platinum_nds" : "firered_gba");
            Legality::Report report;
            if (gen==1) {
                auto rec=pk1Record(1);
                rec.moves[0]=200; // Gen II move, above native Gen I 165.
                const Pokemon::Pokemon1ReadOnly pk(rec);
                report=Legality::analyze(pk,pk.getGameGroup(),source);
            } else {
                auto rec=pk2Record(1);
                rec.moves[0]=252; // Above native Gen II 251.
                const Pokemon::Pokemon2ReadOnly pk(rec);
                report=Legality::analyze(pk,pk.getGameGroup(),source);
            }
            assert(invalid(report,"cannot exist in a Generation"));
            assert(!invalid(report,"Ability not legal for this species"));
            assert(!invalid(report,"Gendered species marked genderless"));
            if(label==2) assert(warning(report,"source-game label conflicts"));
            ++checks;
        }
        // A zero-species native slot is empty, not an invalid Pokémon.
        {
            auto rec=pk1Record(1);
            rec.species=0;
            const Pokemon::Pokemon1ReadOnly pk(rec);
            assert(Legality::analyze(pk,pk.getGameGroup(),"").issues.empty());
            ++checks;
        }
        {
            auto rec=pk2Record(1);
            rec.species=0;
            const Pokemon::Pokemon2ReadOnly pk(rec);
            assert(Legality::analyze(pk,pk.getGameGroup(),"").issues.empty());
            ++checks;
        }
    }
    assert(checks==(baseline?12:200));
    std::cout<<"L5 actual PK1/PK2 source-free native analyzer 12 cases: "
             <<(baseline?"known false-Invalid baseline":"corrected guard")
             <<" PASS (synthetic records, no retail history)\n";
}