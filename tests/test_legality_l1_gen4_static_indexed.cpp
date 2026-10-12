#include "Legality/Gen4StaticIndexedOriginalHistory.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main(){
    namespace H=Legality::Gen4StaticIndexedOriginalHistory;
    namespace S=Legality::Gen4Static;
    using Status=H::Status;
    constexpr std::array<const char*,5> games{
        "diamond_nds","pearl_nds","platinum_nds",
        "heartgold_nds","soulsilver_nds"
    };
    // Four distinct source classes per exact game:
    // an unrestricted ordinary static, a source gift egg, a roaming
    // static and a fixed-ball static. Original packed indices are pinned.
    constexpr std::array<std::array<std::size_t,4>,5> indexes{{
        {{11,12,17,0}},{{35,36,41,24}},{{59,56,53,48}},
        {{89,114,120,83}},{{146,171,177,140}}
    }};
    std::size_t conditional=0,giftUnknown=0;
    for(std::size_t g=0;g<games.size();++g){
        for(std::size_t kind=0;kind<4;++kind){
            const auto row=S::kPackedGen4StaticEncounters[indexes[g][kind]];
            assert(S::game(row)==static_cast<Legality::Gen4Wild::Game>(g));
            H::Candidate c{};
            c.exactOriginalGame=games[g];c.originalRowIndex=indexes[g][kind];
            c.originalSpecies=S::species(row);c.originalForm=S::form(row);
            c.originalLocation=S::location(row);
            c.originalEggLocation=S::eggLocation(row);
            c.originalLevel=c.originalEggLocation?0:S::level(row);
            c.originalIsEgg=c.originalEggLocation!=0;
            c.originalBall=S::fixedBall(row);
            c.originalGender=0;c.originalNature=0;
            c.originalGameIndependentlyAttested=true;
            c.originalRowIndependentlyAttested=true;
            c.originalCaptureFieldsIndependentlyAttested=true;
            c.originalEggStageIndependentlyAttested=true;
            c.originalRoamingRouteIndependentlyAttested=true;
            c.originalBallIndependentlyAttested=true;
            c.originalPidFamilyIndependentlyAttested=true;
            c.historicalPidOrigin=H::PidOrigin::OrdinaryMethod1;
            if(S::roaming(row)){
                uint16_t loc=0;
                while(loc<256&&!S::roamerLocationAllowed(S::location(row),loc))
                    ++loc;
                assert(loc<256u);
                c.originalLocation=loc;
            }
            const auto result=H::evaluate(c);
            if(c.originalIsEgg){
                assert(result.status==
                       Status::ConditionalOriginalGiftEggSourceAwaitingBreedingVerification);
                assert(result.originalGiftEgg);
                ++giftUnknown;
            }else{
                assert(result.status==Status::ConditionalOriginalStaticHistory);
                ++conditional;
            }
            auto foreign=c;
            foreign.exactOriginalGame=games[(g+1u)%games.size()];
            assert(H::evaluate(foreign).status==Status::ForeignGameRowNotEvidence);
            auto missing=c;missing.originalCaptureFieldsIndependentlyAttested=false;
            assert(H::evaluate(missing).status==Status::UnknownCaptureStage);
            missing=c;missing.originalRowIndependentlyAttested=false;
            assert(H::evaluate(missing).status==Status::UnknownOriginalRow);
            missing=c;missing.originalEggStageIndependentlyAttested=false;
            assert(H::evaluate(missing).status==Status::UnknownOriginalEggStage);
            auto wrong=c;wrong.originalSpecies=0;
            assert(H::evaluate(wrong).status==Status::CandidateIdentityContradicted);
            wrong=c;wrong.originalEggLocation^=1u;
            assert(H::evaluate(wrong).status==Status::CandidateEggStageContradicted);
            wrong=c;wrong.originalLevel^=1u;
            assert(H::evaluate(wrong).status==Status::CandidateCaptureStageContradicted);
            if(S::roaming(row)){
                missing=c;missing.originalRoamingRouteIndependentlyAttested=false;
                assert(H::evaluate(missing).status==Status::UnknownOriginalRoamingRoute);
                wrong=c;wrong.originalLocation=0;
                assert(H::evaluate(wrong).status==Status::CandidateCaptureStageContradicted);
            }
            if(S::fixedBall(row)){
                missing=c;missing.originalBallIndependentlyAttested=false;
                assert(H::evaluate(missing).status==Status::UnknownOriginalBall);
                wrong=c;wrong.originalBall^=1u;
                assert(H::evaluate(wrong).status==Status::CandidateBallContradicted);
            }
            if(!c.originalIsEgg){
                missing=c;missing.originalPidFamilyIndependentlyAttested=false;
                assert(H::evaluate(missing).status==Status::UnknownOriginalPidOrigin);
                wrong=c;wrong.historicalPidOrigin=H::PidOrigin::Pokewalker;
                assert(H::evaluate(wrong).status==Status::CandidatePidFamilyContradicted);
                wrong=c;wrong.historicalPidOrigin=static_cast<H::PidOrigin>(255u);
                assert(H::evaluate(wrong).status==Status::UnknownOriginalPidOrigin);
            }
        }
    }
    assert(conditional==15u && giftUnknown==5u);
    // HG/SS special forced-shiny statics may not be run through ordinary
    // Method 1 just because the source row's species/level looks normal.
    for(std::size_t k=0;k<2;++k){
        const std::size_t g=3+k;
        const std::size_t idx=k?151u:94u;
        const auto row=S::kPackedGen4StaticEncounters[idx];
        assert(S::species(row)==130u);
        H::Candidate c{};
        c.exactOriginalGame=games[g];c.originalRowIndex=idx;
        c.originalSpecies=S::species(row);c.originalForm=S::form(row);
        c.originalLocation=S::location(row);c.originalLevel=S::level(row);
        c.originalEggLocation=S::eggLocation(row);c.originalShiny=true;
        c.originalGameIndependentlyAttested=true;
        c.originalRowIndependentlyAttested=true;
        c.originalCaptureFieldsIndependentlyAttested=true;
        c.originalEggStageIndependentlyAttested=true;
        c.originalPidFamilyIndependentlyAttested=true;
        c.historicalPidOrigin=H::PidOrigin::ChainShiny;
        assert(H::evaluate(c).status==Status::ConditionalOriginalStaticHistory);
        auto wrong=c;wrong.historicalPidOrigin=H::PidOrigin::OrdinaryMethod1;
        assert(H::evaluate(wrong).status==Status::CandidatePidFamilyContradicted);
        wrong=c;wrong.originalShiny=false;
        assert(H::evaluate(wrong).status==Status::CandidateTemplateConstraintsContradicted);
        // HGSS Pichu special form: pinned PID category is NOT ordinary
        // wild Method1, and fixed female/nature/shiny metadata is checked.
        c.originalRowIndex=k?170u:113u;
        const auto special=S::kPackedGen4StaticEncounters[c.originalRowIndex];
        assert(S::species(special)==172 && S::form(special)==1);
        c.originalSpecies=172;c.originalForm=1;c.originalLevel=S::level(special);
        c.originalLocation=S::location(special);c.originalShiny=false;
        c.originalGender=1;c.originalNature=4;c.originalEggLocation=0;
        c.historicalPidOrigin=H::PidOrigin::Pokewalker;
        assert(H::evaluate(c).status==Status::ConditionalOriginalStaticHistory);
        wrong=c;wrong.historicalPidOrigin=H::PidOrigin::OrdinaryMethod1;
        assert(H::evaluate(wrong).status==Status::CandidatePidFamilyContradicted);
        wrong=c;wrong.originalNature=0;
        assert(H::evaluate(wrong).status==Status::CandidateTemplateConstraintsContradicted);
    }
    std::cout<<"L1 E14/E16 twenty DPPt/HGSS original static/gift/roamer + 4 special PID family rows: PASS\n";
}
