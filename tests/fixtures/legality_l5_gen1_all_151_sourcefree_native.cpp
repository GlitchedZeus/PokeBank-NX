// L5 source-free Gen I absence/format controls for ALL 151 species.
// Requires REAL coordinator Gen1ReadOnlySave.cpp, Pokemon1ReadOnly,
// and Legality::analyze linkage; original game histories are absent.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Legality/Legality.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
#ifdef NDEBUG
#error "L5 native Gen I all-species QA requires assertions"
#endif
namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}
namespace {
bool has(const Legality::Report& r,Legality::Severity level,std::string_view snippet){
    for(const auto& x:r.issues)
        if(x.severity==level && x.text.find(snippet)!=std::string::npos)return true;
    return false;
}
}
int main(){
    using PokeVault::Integration::Gen1::PokemonRecord;
    using PokeVault::Integration::Gen1::gen1InternalToNational;
    constexpr std::array<std::string_view,6> sources{
        "", "red_gb", "blue_gb", "yellow_gb", "platinum_nds", "unknown_source"
    };
    unsigned checked=0,identified=0;
    for(uint16_t species=1;species<=151;++species){
        unsigned matches=0;
        uint8_t nativeInternal=0;
        for(unsigned raw=1;raw<=255;++raw){
            if(gen1InternalToNational(static_cast<uint8_t>(raw))==species){
                ++matches;nativeInternal=static_cast<uint8_t>(raw);
            }
        }
        assert(matches==1 && nativeInternal!=0);
        ++identified;
        PokemonRecord record{};
        record.species=species;
        record.rawSpecies=nativeInternal;
        record.level=5;
        record.experience=125; // synthetic; EXP/level history NOT validated here
        record.trainerId=1234;
        record.originalTrainer="L5";
        record.nickname="L5";
        record.catchRate=0; // unresolved original capture/trade history
        record.dvs={15,15,15,15,15};
        record.statExperience={65535u,0u,1u,32768u,65534u};
        record.rawBodySize=record.rawBody.size();
        for(const auto& source:sources){
            for(uint16_t move:{uint16_t{33},uint16_t{200}}){
                record.moves={static_cast<uint8_t>(move),0,0,0};
                record.pp={35,0,0,0};
                const Pokemon::Pokemon1ReadOnly pk(record);
                assert(pk.speciesID()==species && pk.ability()==0 && pk.gender()==2);
                assert(pk.statExpHP()==65535u && pk.statExpATK()==0 &&
                       pk.statExpDEF()==1 && pk.statExpSPE()==32768 &&
                       pk.statExpSpecial()==65534);
                const auto report=Legality::analyze(pk,pk.getGameGroup(),source);
                // Stat experience 65535/0/1/32768/65534 needs battle gains with
                // one stat still at zero. Pinned PKHeX 6501f0ab calls that
                // EVs:EffortShouldBeZero, but original pret/pokered
                // engine/battle/experience.asm (DivideExpDataByNumMonsGainingExp,
                // and the Exp. All halving in engine/battle/core.asm) floors each
                // enemy base stat by the recipient count before it is added, so a
                // stat can receive 0 while the others grow. The engine therefore
                // keeps this as a non-definitive warning: never "No problems
                // found", never a false Invalid from this rule alone.
                assert(report.verdict()!=Legality::Verdict::NoProblemsFound);
                assert(!has(report,Legality::Severity::Invalid,
                            "Stat experience from battles"));
                assert(has(report,Legality::Severity::Warning,
                           "Stat experience from battles"));
                assert(!has(report,Legality::Severity::Invalid,
                            "Ability not legal for this species"));
                assert(!has(report,Legality::Severity::Invalid,
                            "Gendered species marked genderless"));
                assert(!has(report,Legality::Severity::Invalid,
                            "EV total over 510"));
                assert(!has(report,Legality::Severity::Invalid,
                            "EV over 252"));
                assert(has(report,Legality::Severity::Invalid,
                           "cannot exist in a Generation")== (move==200));
                assert(has(report,Legality::Severity::Warning,
                           "source-game label conflicts")==
                       (source=="platinum_nds"));
                ++checked;
            }
        }
    }
    assert(identified==151 && checked==1812);
    std::cout<<"L5 actual coordinator PK1 native all 151 internal species x 6 sources x 2 move-domain cases: "
             <<checked<<" PASS (synthetic; original RBY source Unknown)\n";
}
