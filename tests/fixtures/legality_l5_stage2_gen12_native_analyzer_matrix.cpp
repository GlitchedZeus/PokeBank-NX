// L5 Stage 2: real Legality::analyze regressions on native read-only PK1/PK2.
// Synthetic specimens only; no authenticated retail histories or save writes.
// --l5-baseline expects the frozen L5 pre-F3/F4 source predicates and pre-F5.
// --post-f3f4-pre-f5 tests the intermediate coordinator state.
// --after-f5 requires the independently reviewed native PK2 egg stat-exp fix.
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
#ifdef NDEBUG
#error "L5 Stage2 native analyzer checks require enabled assertions"
#endif

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}
namespace {
bool contains(const Legality::Report& report, Legality::Severity severity,
              std::string_view phrase) {
    for(const auto& issue:report.issues)
        if(issue.severity==severity &&
           issue.text.find(phrase)!=std::string::npos) return true;
    return false;
}
PokeVault::Integration::Gen2::PokemonRecord specimen2(uint16_t species) {
    PokeVault::Integration::Gen2::PokemonRecord x{};
    x.species=species; x.level=5; x.experience=135; x.trainerId=1234;
    x.moves={33,0,0,0}; x.pp={35,0,0,0};
    x.originalTrainer="L5"; x.nickname="STAGE2";
    x.dvs[1]=15; x.rawBodySize=x.rawBody.size();
    return x;
}
PokeVault::Integration::Gen1::PokemonRecord specimen1(uint16_t species) {
    PokeVault::Integration::Gen1::PokemonRecord x{};
    x.species=species; x.rawSpecies=species==1?153:173;
    x.level=5; x.experience=135; x.trainerId=1234;
    x.catchRate=species==1?45:190;
    x.moves={33,0,0,0}; x.pp={35,0,0,0};
    x.originalTrainer="L5"; x.nickname="STAGE2";
    x.rawBodySize=x.rawBody.size(); return x;
}
uint16_t statExp(const Pokemon::Pokemon2ReadOnly& pk,unsigned index) {
    switch(index) {
        case 0:return pk.statExpHP();
        case 1:return pk.statExpATK();
        case 2:return pk.statExpDEF();
        case 3:return pk.statExpSPE();
        default:return pk.statExpSpecial();
    }
}
}
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    const std::string_view mode=argv[1];
    if(mode!="--l5-baseline" && mode!="--post-f3f4-pre-f5" && mode!="--after-f5")return 2;
    const bool preGuard=mode=="--l5-baseline";
    const bool afterF5=mode=="--after-f5";
    const std::array<std::string_view,5> sources{
        "", "gold_gbc", "crystal_gbc", "firered_gba", "unrecognized_source"};
    unsigned checks=0, eggNonzero=0;
    for(uint16_t species : {1u,29u,81u,201u,196u,242u}) {
        for(unsigned field=0;field<5;++field) {
            for(uint16_t amount : {0u,1u,2560u,25600u,65535u}) {
                for(bool egg : {false,true}) {
                    for(unsigned label=0;label<sources.size();++label) {
                        auto record=specimen2(species);
                        record.statExperience[field]=amount;
                        record.isEgg=egg;
                        const auto original=record.rawBody;
                        Pokemon::Pokemon2ReadOnly pk(record);
                        const Pokemon::Pokemon& nativeBase=pk;
                        assert(nativeBase.nativePk2ReadOnly()==&pk);
                        assert(nativeBase.nativePk1ReadOnly()==nullptr);
                        assert(pk.isEgg()==egg && statExp(pk,field)==amount);
                        assert(pk.evHP()==0 && pk.evATK()==0 && pk.evDEF()==0 &&
                               pk.evSPE()==0 && pk.evSPA()==0 && pk.evSPD()==0);
                        const auto report=Legality::analyze(pk,pk.getGameGroup(),sources[label]);
                        const bool expectedEggStat=afterF5 && egg && amount>0;
                        assert(contains(report,Legality::Severity::Invalid,"stat experience")==expectedEggStat);
                        const bool unknownSource=label==0 || label==3 || label==4;
                        assert(contains(report,Legality::Severity::Invalid,"Ability not legal for this species") ==
                               (preGuard && unknownSource));
                        assert(contains(report,Legality::Severity::Warning,"source-game label conflicts") ==
                               (label==3));
                        assert(pk.strictRecord().rawBody==original);
                        if(egg && amount)++eggNonzero;
                        ++checks;
                    }
                }
            }
        }
    }
    assert(checks==1500 && eggNonzero==600);
    // A duplicate move proves a current structural contradiction; a missing
    // origin does not invent parent, encounter, or transfer history.
    for(unsigned gen : {1u,2u}) for(unsigned variant=0;variant<4;++variant) {
        for(unsigned label=0;label<3;++label) {
            const std::string_view source=label==0?std::string_view{}:
                label==1?(gen==1?"red_gb":"gold_gbc"):
                         (gen==1?"platinum_nds":"firered_gba");
            Legality::Report report;
            if(gen==1) {
                auto rec=specimen1(1);
                if(variant==0) rec.species=0; // Empty slot
                if(variant==1) rec.moves={33,33,0,0}; // Duplicate
                if(variant==2) rec.moves={166,0,0,0}; // Non-native move
                if(variant==3) rec.statExperience={65535,0,0,0,0}; // Native Gen I stat exp
                const Pokemon::Pokemon1ReadOnly pk(rec);
                const Pokemon::Pokemon& generic=pk;
                assert(generic.nativePk1ReadOnly()==&pk);
                assert(generic.nativePk2ReadOnly()==nullptr);
                report=Legality::analyze(pk,pk.getGameGroup(),source);
            } else {
                auto rec=specimen2(1);
                if(variant==0) rec.species=0;
                if(variant==1) rec.moves={33,33,0,0};
                if(variant==2) rec.moves={252,0,0,0};
                if(variant==3) rec.statExperience={65535,0,0,0,0};
                const Pokemon::Pokemon2ReadOnly pk(rec);
                const Pokemon::Pokemon& generic=pk;
                assert(generic.nativePk2ReadOnly()==&pk);
                assert(generic.nativePk1ReadOnly()==nullptr);
                report=Legality::analyze(pk,pk.getGameGroup(),source);
            }
            if(variant==0) assert(report.issues.empty());
            if(variant==1) assert(contains(report,Legality::Severity::Invalid,"Duplicate move"));
            if(variant==2) assert(contains(report,Legality::Severity::Invalid,"cannot exist in a Generation"));
            if(variant==3) assert(!contains(report,Legality::Severity::Invalid,"stat experience"));
            if(variant!=0 && label==2)
                assert(contains(report,Legality::Severity::Warning,"source-game label conflicts"));
            ++checks;
        }
    }
    // Gen I stores neither ability nor gender: only source-label availability
    // may NOT turn absent native fields into Invalid after coordinator F3/F4.
    for(uint16_t species : {1u,81u})for(unsigned label=0;label<3;++label){
        const std::string_view source=label==0?"":label==1?"red_gb":"platinum_nds";
        Pokemon::Pokemon1ReadOnly pk(specimen1(species));
        assert(pk.ability()==0 && pk.gender()==2);
        const auto report=Legality::analyze(pk,pk.getGameGroup(),source);
        const bool unknown=label!=1;
        assert(contains(report,Legality::Severity::Invalid,"Ability not legal for this species") ==
               (preGuard&&unknown));
        if(species==1)
            assert(contains(report,Legality::Severity::Invalid,"Gendered species marked genderless") ==
                   (preGuard&&unknown));
        ++checks;
    }
    assert(checks==1530);
    std::cout<<"L5 Stage2 real PK1/PK2 Legality::analyze matrix: "<<checks
             <<" PASS; egg-nonzero stat-exp candidates="<<eggNonzero
             <<"; mode="<<mode<<" (synthetic only)\n";
}
