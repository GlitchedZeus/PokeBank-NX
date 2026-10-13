#include "Legality/Gen4SinnohSpecialOriginalStage.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main() {
    namespace S=Legality::Gen4SinnohSpecialOriginalStage;
    namespace W=Legality::Gen4Wild;
    using Status=S::Status;
    constexpr std::array<const char*,3> games{
        "diamond_nds","pearl_nds","platinum_nds"
    };
    // Fixed source rows: ordinary non-Radar Grass, Radar-eligible Grass,
    // ordinary Honey Tree, trainer-specific Munchlax Honey Tree.
    constexpr std::array<std::array<std::size_t,4>,3> rows{{
        {{34,0,1406,2787}},{{2946,2912,4308,5678}},
        {{5839,5803,7155,8323}}
    }};
    unsigned count=0;
    for(std::size_t g=0;g<games.size();++g){
        for(std::size_t kind=0;kind<4;++kind){
            const auto packed=W::kPackedGen4WildEncounters[rows[g][kind]];
            assert(W::game(packed)==static_cast<W::Game>(g));
            S::Candidate c{};
            c.exactOriginalGame=games[g];
            c.originalRowIndex=rows[g][kind];
            c.originalSpecies=W::species(packed);
            c.originalLocation=W::location(packed);
            c.originalLevel=W::minLevel(packed);
            c.originalForm=W::form(packed);
            c.originalMethod=W::method(packed);
            c.originalGameIndependentlyAttested=true;
            c.originalRowIndependentlyAttested=true;
            c.originalCaptureStageIndependentlyAttested=true;
            c.originalPathIndependentlyAttested=true;
            c.originalPath=kind==0?S::Path::OrdinaryWild:
                kind==1?S::Path::RadarWild:S::Path::HoneyTree;
            c.originalTrainerIdIndependentlyAttested=true;
            c.originalSaveTrainerId32=0u;
            const auto result=S::evaluate(c);
            if(kind>=2){
                assert(result.status==Status::ConditionalOriginalHoneySource);
                assert(result.trainerDependentTree==(kind==3));
            }else if(kind==1){
                assert(result.status==Status::UnknownRadarActivation);
                c.originalRadarActivationIndependentlyAttested=true;
                assert(S::evaluate(c).status==
                       Status::ConditionalRadarSourceAwaitingPidReplay);
                c.originalPath=S::Path::RadarChainShiny;
                assert(S::evaluate(c).status==Status::UnknownChainShinyStage);
                c.originalChainShinyStageIndependentlyAttested=true;
                assert(S::evaluate(c).status==
                       Status::ConditionalRadarSourceAwaitingPidReplay);
                c.originalPath=S::Path::RadarWild;
            }else{
                assert(result.status==Status::ConditionalOriginalWildSource);
                auto radar=c;radar.originalPath=S::Path::RadarWild;
                radar.originalRadarActivationIndependentlyAttested=true;
                assert(S::evaluate(radar).status==Status::CandidateRadarSourceContradicted);
            }
            ++count;
            auto missing=c;
            missing.originalCaptureStageIndependentlyAttested=false;
            assert(S::evaluate(missing).status==Status::UnknownOriginalCaptureStage);
            missing=c;missing.originalRowIndependentlyAttested=false;
            assert(S::evaluate(missing).status==Status::UnknownOriginalRow);
            missing=c;missing.originalPathIndependentlyAttested=false;
            assert(S::evaluate(missing).status==Status::UnknownOriginalPath);
            missing=c;missing.originalPath=static_cast<S::Path>(255u);
            assert(S::evaluate(missing).status==Status::UnknownOriginalPath);
            auto wrong=c;wrong.originalSpecies=0;
            assert(S::evaluate(wrong).status==
                   Status::CandidateOriginalSpeciesOrFormContradicted);
            wrong=c;wrong.originalLocation=0;
            assert(S::evaluate(wrong).status==
                   Status::CandidateOriginalLocationOrLevelContradicted);
            wrong=c;wrong.originalMethod=15u;
            assert(S::evaluate(wrong).status==Status::CandidateOriginalMethodContradicted);
            wrong=c;wrong.exactOriginalGame=games[(g+1u)%3];
            assert(S::evaluate(wrong).status==Status::ForeignGameRowNotEvidence);
            if(kind==3){
                missing=c;missing.originalTrainerIdIndependentlyAttested=false;
                assert(S::evaluate(missing).status==Status::UnknownTreeTrainer);
                wrong=c;wrong.originalSaveTrainerId32=0x0F101112u;
                assert(!W::isMunchlaxTreeLocation(
                    wrong.originalSaveTrainerId32,wrong.originalLocation));
                assert(S::evaluate(wrong).status==
                       Status::CandidateTreeSelectionContradicted);
                // The original save's 32-bit ID is not interchangeable
                // with current Pokémon trainer metadata after transfers.
                static_assert(W::isMunchlaxTreeLocation(0u,20u));
                static_assert(!W::isMunchlaxTreeLocation(0x0F101112u,20u));
            }
        }
    }
    assert(count==12);
    assert(!W::isMunchlaxTreeLocation(0x0F101112u,20u));
    std::cout<<"L1 E12/E16 12 Sinnoh wild/Radar/Honey original row cases: PASS\n";
}
