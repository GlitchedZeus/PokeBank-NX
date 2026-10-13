// L5 live native test: links the actual PK1 read-only adapter and Legality::analyze.
// Synthetic data and version labels are NOT original game history evidence.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Legality/Legality.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
#include "legality_l5_gen1_rby_oracle_cases.inc"
#ifdef NDEBUG
#error "L5 Gen I actual production differential requires active assertions"
#endif
namespace Trainer {
const char* getSpeciesName(uint16_t id){return Names::getSpeciesName(id);}
const char* getItemName(uint16_t id){return Names::getItemName(id);}
}
static bool invalid(const Legality::Report& report,std::string_view text){
    for(const auto& x:report.issues)
        if(x.severity==Legality::Severity::Invalid && x.text.find(text)!=std::string::npos)
            return true;
    return false;
}
static bool warning(const Legality::Report& report,std::string_view text){
    for(const auto& x:report.issues)
        if(x.severity==Legality::Severity::Warning && x.text.find(text)!=std::string::npos)
            return true;
    return false;
}
int main(int argc,char** argv){
    if(argc!=2 || (std::string_view(argv[1])!="--baseline" &&
                    std::string_view(argv[1])!="--coordinator")){
        std::cerr<<"usage: L5Gen1NX --baseline | --coordinator\n";
        return 2;
    }
    const bool baseline=std::string_view(argv[1])=="--baseline";
    unsigned checked=0;
    for(const auto& c:kL5Gen1Cases){
        using PokeVault::Integration::Gen1::PokemonRecord;
        PokemonRecord record{};
        record.species=static_cast<uint16_t>(c.species);
        record.rawSpecies=c.species==1?153:173;
        record.level=static_cast<uint8_t>(c.level);
        record.experience=c.experience;
        record.statExperience={static_cast<uint16_t>(c.stat_exp_hp),
            static_cast<uint16_t>(c.stat_exp_attack),
            static_cast<uint16_t>(c.stat_exp_defense),
            static_cast<uint16_t>(c.stat_exp_speed),
            static_cast<uint16_t>(c.stat_exp_special)};
        record.trainerId=1234;
        record.catchRate=c.species==1?45:190;
        record.moves={static_cast<uint8_t>(c.move),0,0,0};
        record.pp={static_cast<uint8_t>(c.move?35:0),0,0,0};
        const unsigned attack=(c.dv16>>12)&15u;
        const unsigned defense=(c.dv16>>8)&15u;
        const unsigned speed=(c.dv16>>4)&15u;
        const unsigned special=c.dv16&15u;
        const unsigned hp=((attack&1u)<<3)|((defense&1u)<<2)|
                          ((speed&1u)<<1)|(special&1u);
        record.dvs={static_cast<uint8_t>(hp),
                    static_cast<uint8_t>(attack),static_cast<uint8_t>(defense),
                    static_cast<uint8_t>(speed),static_cast<uint8_t>(special)};
        record.originalTrainer="L5";
        record.nickname=c.species==1?"BULBASAUR":"MAGNEMITE";
        record.rawBodySize=record.rawBody.size();
        const Pokemon::Pokemon1ReadOnly pk(record);
        assert(pk.speciesID()==c.species && pk.dvHP()==hp);
        assert(pk.dvATK()==attack && pk.dvDEF()==defense &&
               pk.dvSPE()==speed && pk.dvSpecial()==special);
        assert(pk.ability()==0 && pk.gender()==2);
        assert(pk.ivHP()==0 && pk.evHP()==0); // neutral *modern* compatibility getters
        assert(pk.statExpHP()==c.stat_exp_hp &&
               pk.statExpATK()==c.stat_exp_attack &&
               pk.statExpDEF()==c.stat_exp_defense &&
               pk.statExpSPE()==c.stat_exp_speed &&
               pk.statExpSpecial()==c.stat_exp_special);
        assert(pk.exp()==c.experience);
        auto result=Legality::analyze(pk,pk.getGameGroup(),c.source);
        const bool moveDomain=invalid(result,"cannot exist in a Generation");
        assert(moveDomain==(c.move==200));
        const bool modernEvInvalid=invalid(result,"EV total over 510") ||
            invalid(result,"EV over 252");
        assert(!modernEvInvalid); // Gen I has Stat Experience, NOT modern EV fields.
        const bool ability=invalid(result,"Ability not legal for this species");
        const bool gender=invalid(result,"Gendered species marked genderless");
        // The source-free baseline reproduces F3 for any nonempty PK1 and
        // F4 for Bulbasaur; the coordinator version has format guards.
        if(std::string_view(c.source).empty()){
            assert(ability==baseline);
            assert(gender==(baseline && c.species==1));
        }
        if(std::string_view(c.source)=="red_gb"){
            assert(!ability && !gender);
        }
        if(std::string_view(c.source)=="platinum_nds")
            assert(warning(result,"source-game label conflicts"));
        ++checked;
        std::cout<<"{\"schema\":\"L5_GEN1_LIVE_RUNTIME_RESULT_V1\","
          "\"engine\":\"POKEBANK_NX_ANALYZER\","
          "\"execution_mode\":\"actual_production_pk1\","
          "\"nx_variant\":\""<<(baseline?"baseline":"coordinator")<<"\","
          "\"id\":\""<<c.id<<"\","
          "\"source\":\""<<c.source<<"\","
          "\"level\":"<<c.level<<",\"experience\":"<<c.experience<<","
          "\"species\":"<<c.species<<",\"move\":"<<c.move<<","
          "\"dv16\":"<<c.dv16<<","
          "\"dv_hp\":"<<hp<<",\"dv_attack\":"<<attack<<","
          "\"dv_defense\":"<<defense<<",\"dv_speed\":"<<speed<<","
          "\"dv_special\":"<<special<<","
          "\"stat_exp_hp\":"<<pk.statExpHP()<<","
          "\"stat_exp_attack\":"<<pk.statExpATK()<<","
          "\"stat_exp_defense\":"<<pk.statExpDEF()<<","
          "\"stat_exp_speed\":"<<pk.statExpSPE()<<","
          "\"stat_exp_special\":"<<pk.statExpSpecial()<<","
          "\"modern_ev_invalid\":"<<(modernEvInvalid?"true":"false")<<","
          "\"move_domain_invalid\":"<<(moveDomain?"true":"false")<<","
          "\"ability_invalid\":"<<(ability?"true":"false")<<","
          "\"gender_invalid\":"<<(gender?"true":"false")<<","
          "\"source_conflict_warning\":"
          <<(warning(result,"source-game label conflicts")?"true":"false")<<","
          "\"original_retail_history_authenticated\":false}\n";
    }
    assert(checked==72);
    std::cerr<<"L5 Gen I actual PK1 analyzer: "<<checked
             <<" source-context vectors "<<(baseline?"baseline":"coordinator")
             <<" PASS (synthetic, not original retail history)\n";
}
