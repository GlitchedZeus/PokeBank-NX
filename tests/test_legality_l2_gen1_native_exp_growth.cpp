#include "Legality/Gen1NativeExperienceEvidence.h"
// Reuse the actual packed PK1 regression specimen, not a modern PKM.
#define main l2_gen1_dv_stat_fixture_main
#include "test_legality_l2_gen1_native_dv_stat.cpp"
#undef main
#include "Pokemon/Experience.h"
#include <cassert>
#include <cstdint>
#include <iostream>

namespace X=Legality::Gen1NativeExperience;
static void setOriginalEXP(G::PokemonRecord& r,std::uint32_t exp){
    r.experience=exp;
    r.rawBody[0x0E]=static_cast<std::uint8_t>(exp>>16u);
    r.rawBody[0x0F]=static_cast<std::uint8_t>(exp>>8u);
    r.rawBody[0x10]=static_cast<std::uint8_t>(exp);
}
int main(){
    const X::Source verifiedSource{true,true,true};
    const L::SourceContext verifiedJoint{true,false,false,true,true,true};
    std::uint64_t checks=0;
    unsigned nativeSpecies=0;
    for(unsigned raw=1;raw<=255;++raw){
        const auto species=G::gen1InternalToNational(static_cast<std::uint8_t>(raw));
        if(species<1||species>151)continue;
        ++nativeSpecies;
        const auto growth=G::StagedPokemonEditor::growthRate(species);
        assert(growth==0||growth==3||growth==4||growth==5);++checks;
        for(unsigned lvl=1;lvl<=100;++lvl){
            const auto level=static_cast<std::uint8_t>(lvl);
            auto p=specimen(false,level,{15,0,9,2},{65535,1,65025,65026,0});
            p.rawSpecies=p.rawBody[0]=static_cast<std::uint8_t>(raw);
            p.species=species;
            const auto lower=Pokemon::getExpForLevel(level,growth);
            const auto upper=lvl<100?
                Pokemon::getExpForLevel(static_cast<std::uint8_t>(lvl+1),growth):
                static_cast<std::uint32_t>(0x1000000u);
            assert(lower<upper);++checks;
            setOriginalEXP(p,lower);
            const auto base=X::evaluate(p,verifiedSource);
            assert(base.status==X::Status::ConditionalLevelExperienceMatch);
            assert(base.minimumExperienceAtLevel==lower);
            assert(base.exclusiveExperienceAtNextLevel==upper);
            assert(base.levelFromExperience==level);
            checks+=4;
            const auto joined=L::evaluate(p,verifiedJoint);
            assert(joined.status==L::Status::ConditionalNativeFields);
            assert(joined.experienceEvidence.status==X::Status::ConditionalLevelExperienceMatch);
            checks+=2;
            auto unknown=p;
            assert(X::evaluate(unknown,{true,false,true}).status==
                   X::Status::UnknownOriginalProgression);
            assert(X::evaluate(unknown,{true,true,false}).status==
                   X::Status::UnknownOriginalProgression);
            assert(X::evaluate(unknown,{}).status==X::Status::UnknownSource);
            checks+=3;
            assert(L::evaluate(unknown,{true,false,false,true}).status==
                   L::Status::ConditionalNativeFields);
            ++checks;
            // The last attainable value below the next threshold still
            // has this level, except level 100 over-cap historical ambiguity.
            if(lvl<100){
                setOriginalEXP(p,upper-1);
                assert(X::evaluate(p,verifiedSource).status==
                       X::Status::ConditionalLevelExperienceMatch);++checks;
                setOriginalEXP(p,upper);
                assert(X::evaluate(p,verifiedSource).status==
                       X::Status::ContradictedAttestedOriginalProgression);
                assert(L::evaluate(p,verifiedJoint).status==
                       L::Status::ContradictedAttestedLevelExperience);
                checks+=2;
            }else{
                setOriginalEXP(p,0xFFFFFFu);
                assert(X::evaluate(p,verifiedSource).status==
                       X::Status::UnknownOriginalProgression);
                assert(L::evaluate(p,verifiedJoint).status==
                       L::Status::UnknownLevelContext);
                checks+=2;
            }
            if(lvl>1) {
                setOriginalEXP(p,lower-1);
                assert(X::evaluate(p,verifiedSource).status==
                       X::Status::ContradictedAttestedOriginalProgression);
                ++checks;
            }
            // A forged decoded wrapper cannot be rescued by a claimed
            // original gameplay witness. Corrupt raw EXP is local invalid.
            setOriginalEXP(p,lower);
            p.rawBody[0x10]^=0x01u;
            assert(X::evaluate(p,verifiedSource).status==
                   X::Status::InvalidDecodedSourceField);++checks;
        }
    }
    assert(nativeSpecies==151);++checks;
    // Native level 0, which may appear in unusual/glitch records,
    // cannot be rejected by the ordinary level table.
    auto zero=specimen(false,0);
    setOriginalEXP(zero,0);
    assert(X::evaluate(zero,verifiedSource).status==
           X::Status::UnknownOriginalProgression);++checks;
    // No Gen I modern ability, held item, nature, gender or breeding fields
    // participate in the original EXP/level check.
    std::cout<<"L2 Gen I EXP thresholds for all 151 species: "<<checks
             <<" PASS\n";
    return 0;
}
