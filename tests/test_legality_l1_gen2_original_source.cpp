#include "Legality/Gen2IndexedOriginalSourceHistory.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main(){
    namespace H=Legality::Gen2IndexedOriginalSourceHistory;
    namespace W=Legality::Gen2Wild;
    namespace S=Legality::Gen2Static;
    using Status=H::Status;
    // Index is the actual frozen packed row, NOT a species/location lookup:
    // nine separate original wild type classes in each exact Gold/Silver/Crystal.
    constexpr std::array<std::array<std::size_t,9>,3> cases{{
        {{32,0,3,6,14,617,22,27,836}},
        {{3304,3272,3275,3278,3286,3889,3294,3299,4108}},
        {{6578,6544,6547,6550,6558,7167,6566,6572,7388}}
    }};
    constexpr std::array<const char*,3> ids{{
        "gold_gbc","silver_gbc","crystal_gbc"
    }};
    for(std::size_t game=0;game<3;++game){
        for(std::size_t kind=0;kind<9;++kind){
            const auto row=W::kPackedGen2WildEncounters[cases[game][kind]];
            assert(W::game(row)==static_cast<W::Game>(game));
            assert(W::type(row)==kind);
            H::Candidate c{};
            c.exactOriginalGame=ids[game];c.originalRowIndex=cases[game][kind];
            c.originalSpecies=W::species(row);c.originalLocation=W::location(row);
            c.originalLevel=W::minLevel(row);c.originalMethod=W::type(row);
            // Original time masks (rather than current clock) must be sampled.
            const auto mask=W::timeMask(row);
            c.originalTime=(mask&1u)?0u:(mask&2u)?1u:(mask&4u)?2u:3u;
            c.originalGameIndependentlyAttested=true;
            c.originalRowIndependentlyAttested=true;
            c.originalCaptureStageIndependentlyAttested=true;
            c.originalMethodIndependentlyAttested=true;
            c.originalTimeIndependentlyAttested=true;
            c.originalTreeEligibilityIndependentlyAttested=true;
            c.originalContestSessionIndependentlyAttested=true;
            if(game==2){
                c.originalCrystalCaughtDataIndependentlyAttested=true;
                c.originalCrystalCaughtData=static_cast<uint16_t>(
                    (static_cast<uint16_t>(c.originalTime)<<14) |
                    (static_cast<uint16_t>(c.originalLevel)<<8) |
                    c.originalLocation);
            }
            const auto result=H::wild(c);
            assert(result.status==Status::ConditionalOriginalSource);
            assert(result.sourceMethod==kind);
            assert(result.slot==W::slot(row));
            auto foreign=c;
            foreign.exactOriginalGame=ids[(game+1u)%3u];
            assert(H::wild(foreign).status==Status::ForeignGameRowNotEvidence);
            auto missing=c;missing.originalRowIndependentlyAttested=false;
            assert(H::wild(missing).status==Status::UnknownOriginalRow);
            missing=c;missing.originalCaptureStageIndependentlyAttested=false;
            assert(H::wild(missing).status==Status::UnknownCaptureStage);
            missing=c;missing.originalMethodIndependentlyAttested=false;
            assert(H::wild(missing).status==Status::UnknownOriginalMethod);
            // Time-restricted rows must not silently trust default morning.
            missing=c;missing.originalTimeIndependentlyAttested=false;
            if(W::timeMask(row)!=0u && game!=2)
                assert(H::wild(missing).status==Status::UnknownOriginalTime);
            else
                assert(H::wild(missing).status==Status::ConditionalOriginalSource);
            if(game==2 && W::timeMask(row)!=0u){
                // Crystal caught-data is a separate direct clock witness:
                // a detached time field is unnecessary if that history is
                // separately recorded, not simply guessed from today's time.
                missing.originalTime=3u;
                assert(H::wild(missing).status==Status::ConditionalOriginalSource);
                missing.originalCrystalCaughtDataIndependentlyAttested=false;
                assert(H::wild(missing).status==Status::UnknownOriginalTime);
            }
            if(kind==6 || kind==7){
                missing=c;missing.originalTreeEligibilityIndependentlyAttested=false;
                assert(H::wild(missing).status==Status::UnknownOriginalTree);
            }
            if(kind==8){
                missing=c;missing.originalContestSessionIndependentlyAttested=false;
                assert(H::wild(missing).status==Status::UnknownOriginalContest);
            }
            auto wrong=c;wrong.originalSpecies=0;
            assert(H::wild(wrong).status==Status::CandidateSpeciesContradicted);
            wrong=c;wrong.originalLocation=0;
            assert(H::wild(wrong).status==Status::CandidateLocationContradicted);
            wrong=c;wrong.originalLevel=0;
            assert(H::wild(wrong).status==Status::CandidateLevelContradicted);
            wrong=c;wrong.originalMethod=15;
            assert(H::wild(wrong).status==Status::CandidateMethodContradicted);
            wrong=c;wrong.originalTime=0xFFu;
            assert(H::wild(wrong).status==Status::CandidateTimeContradicted);
            if(mask!=0u){
                wrong=c;
                for(uint8_t t=0;t<4;++t){
                    if(!W::timeAllows(mask,t)){
                        wrong.originalTime=t;
                        break;
                    }
                }
                if(wrong.originalTime!=c.originalTime)
                    assert(H::wild(wrong).status==Status::CandidateTimeContradicted);
            }
            if(game==2){
                wrong=c;wrong.originalCrystalCaughtData^=0x0100;
                assert(H::wild(wrong).status==Status::CandidateTimeContradicted);
            }
        }
    }
    // G/S/C static, original gift egg, and roamer row retain distinct
    // provenance from wild, even if captured species/current levels coincide.
    constexpr std::array<std::array<std::size_t,3>,3> statics{{
        {{0,18,27}},{{32,50,59}},{{64,80,101}}
    }};
    std::size_t positiveStatics=0;
    for(std::size_t game=0;game<3;++game){
        for(std::size_t type=0;type<3;++type){
            const auto row=S::kPackedGen2StaticEncounters[statics[game][type]];
            assert(S::game(row)==static_cast<S::Game>(game));
            H::Candidate c{};
            c.exactOriginalGame=ids[game];c.originalRowIndex=statics[game][type];
            c.originalSpecies=S::species(row);c.originalLevel=S::level(row);
            c.originalLocation=S::location(row);
            c.originalIsEgg=S::isEgg(row);
            c.originalShiny=S::shinyPolicy(row)==1u;
            c.originalGameIndependentlyAttested=true;
            c.originalRowIndependentlyAttested=true;
            c.originalCaptureStageIndependentlyAttested=true;
            c.originalEggStateIndependentlyAttested=true;
            c.originalShinyIndependentlyAttested=true;
            c.originalRoamerRouteIndependentlyAttested=true;
            if(S::roaming(row)){
                uint8_t loc=0;
                while(loc<64&&!S::roamerLocationAllowed(loc))++loc;
                assert(loc<64);c.originalLocation=loc;
            }
            if(game==2&&!S::isEgg(row)){
                c.originalCrystalCaughtDataIndependentlyAttested=true;
                c.originalCrystalCaughtData=static_cast<uint16_t>(
                    (static_cast<uint16_t>(c.originalLevel)<<8) |
                    c.originalLocation);
            }
            assert(H::staticOrGift(c).status==Status::ConditionalOriginalSource);
            ++positiveStatics;
            auto bad=c;bad.originalSpecies=0;
            assert(H::staticOrGift(bad).status==Status::CandidateSpeciesContradicted);
            bad=c;bad.originalIsEgg=!c.originalIsEgg;
            assert(H::staticOrGift(bad).status==Status::CandidateEggContradicted);
            auto lost=c;lost.originalEggStateIndependentlyAttested=false;
            assert(H::staticOrGift(lost).status==Status::UnknownOriginalEggState);
            if(S::roaming(row)){
                lost=c;lost.originalRoamerRouteIndependentlyAttested=false;
                assert(H::staticOrGift(lost).status==Status::UnknownOriginalRoamer);
            }
            if(S::shinyPolicy(row)!=0){
                bad=c;bad.originalShiny=!c.originalShiny;
                assert(H::staticOrGift(bad).status==Status::CandidateShinyContradicted);
                lost=c;lost.originalShinyIndependentlyAttested=false;
                assert(H::staticOrGift(lost).status==Status::UnknownOriginalShiny);
            }
        }
    }
    assert(positiveStatics==9u);
    assert(std::size(W::kPackedGen2WildEncounters)==9856u);
    assert(std::size(S::kPackedGen2StaticEncounters)==107u);
    std::cout<<"L1 E04/E05 27 GSC wild + 9 GSC static/egg/roamer sources: PASS\n";
}
