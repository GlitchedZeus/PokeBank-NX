// L1 Stage 2: all 11 original GSC NPC trade templates with independent
// source, OT/DV, Crystal caught-data and later-evolution alternatives.
#include "Legality/Gen2OriginalNpcTradeStage.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace N=Legality::Gen2OriginalNpcTradeStage;
namespace T=Legality::Gen2Trade;
using S=N::Status;

N::Candidate candidate(std::size_t idx, const char* game) {
    const auto row=T::kPackedGen2Trades[idx];
    N::Candidate c{};
    c.exactOriginalGame=game;c.originalTradeRow=idx;
    c.originalSpecies=T::species(row);
    c.presentSpecies=c.originalSpecies;c.originalLevel=T::level(row);
    c.originalTrainerId=T::tid(row);
    c.originalDVs={0,T::atkDv(row),T::defDv(row),T::speDv(row),T::specialDv(row)};
    c.originalCrystalCaughtData=T::kNpcTradeLocation;
    c.originalGameIndependentlyAttested=true;
    c.originalTradeRowIndependentlyAttested=true;
    c.originalReceivedSpeciesAndLevelIndependentlyAttested=true;
    c.originalTradeOtAndTrainerIndependentlyAttested=true;
    c.originalDvsIndependentlyAttested=true;
    c.originalCrystalCaughtDataIndependentlyAttested=true;
    c.noLaterEvolutionOrTimeCapsuleIndependentlyAttested=true;
    return c;
}

int main() {
    std::uint64_t checks=0;
    static_assert(std::size(T::kPackedGen2Trades)==11);
    for(auto game:{"gold_gbc","silver_gbc","crystal_gbc"}) {
        for(std::size_t idx=0;idx<std::size(T::kPackedGen2Trades);++idx) {
            auto c=candidate(idx,game);
            const auto base=N::evaluate(c);
            assert(base.status==S::ConditionalOriginalTradeTemplateOnly);
            assert(base.templateSpecies==c.originalSpecies);
            assert(base.templateMinimumLevel==c.originalLevel);
            checks+=3;
            // Negative source evidence is candidate-local, not a global
            // impossibility for gifts, linked trades or acquired descendants.
            auto bad=c;bad.originalSpecies=c.originalSpecies==1?2:1;
            assert(N::evaluate(bad).status==S::CandidateOriginalSpeciesContradicted);
            bad=c;bad.originalLevel=static_cast<uint8_t>(c.originalLevel-1u);
            if(c.originalLevel>1)
                assert(N::evaluate(bad).status==S::CandidateMinimumLevelContradicted);
            else assert(N::evaluate(bad).status==S::UnknownOriginalLevel);
            checks+=2;
            if(base.fixedTrainerAndDvsAvailable) {
                bad=c;bad.originalTrainerId^=1u;
                assert(N::evaluate(bad).status==S::CandidateFixedTrainerContradicted);
                bad=c;bad.originalDVs[2]^=1u;
                assert(N::evaluate(bad).status==S::CandidateFixedDVsContradicted);
                bad=c;bad.originalDVs[2]=255u;
                assert(N::evaluate(bad).status==S::UnknownOriginalDVs);
                checks+=3;
            } else {
                bad=c;bad.originalDvsIndependentlyAttested=false;
                assert(N::evaluate(bad).status==S::ConditionalOriginalTradeTemplateOnly);
                ++checks;
            }
            if(c.exactOriginalGame=="crystal_gbc") {
                bad=c;bad.originalCrystalCaughtData=T::kNpcTradeLocation+1u;
                assert(N::evaluate(bad).status==S::CandidateCaughtDataContradicted);
                bad=c;bad.originalCrystalCaughtDataIndependentlyAttested=false;
                assert(N::evaluate(bad).status==S::UnknownOriginalCaughtData);
                checks+=2;
            }
            bad=c;bad.originalTradeRowIndependentlyAttested=false;
            assert(N::evaluate(bad).status==S::UnknownOriginalRow);
            bad=c;bad.originalGameIndependentlyAttested=false;
            assert(N::evaluate(bad).status==S::UnknownOriginalGame);
            bad=c;bad.originalReceivedSpeciesAndLevelIndependentlyAttested=false;
            assert(N::evaluate(bad).status==S::UnknownOriginalTrade);
            bad=c;bad.originalTradeOtAndTrainerIndependentlyAttested=false;
            assert(N::evaluate(bad).status==S::UnknownOriginalTrainer);
            bad=c;bad.originalDvsIndependentlyAttested=false;
            assert(N::evaluate(bad).status==(base.fixedTrainerAndDvsAvailable?
                S::UnknownOriginalDVs:S::ConditionalOriginalTradeTemplateOnly));
            checks+=5;
            bad=c;bad.presentSpecies=0;
            assert(N::evaluate(bad).status==S::UnknownPresentSpecies);
            bad=c;bad.presentSpecies=static_cast<uint16_t>(c.originalSpecies+1u);
            assert(N::evaluate(bad).status==S::UnknownLaterEvolutionOrTimeCapsule);
            bad=c;bad.noLaterEvolutionOrTimeCapsuleIndependentlyAttested=false;
            assert(N::evaluate(bad).status==S::UnknownLaterEvolutionOrTimeCapsule);
            checks+=3;
        }
    }
    // Exhaust each corrupt source-row offset and every byte-domain species
    // input while preserving a known legitimate trade route.
    for(std::size_t index=std::size(T::kPackedGen2Trades);index<1024;++index) {
        auto c=candidate(0,"gold_gbc");
        c.originalTradeRow=index;
        assert(N::evaluate(c).status==S::UnknownOriginalRow);
        ++checks;
    }
    for(unsigned input=0;input<65536;++input) {
        auto c=candidate(0,"crystal_gbc");
        c.originalSpecies=static_cast<uint16_t>(input);
        const auto status=N::evaluate(c).status;
        if(input==0 || input>251)assert(status==S::UnknownOriginalSpecies);
        else if(input==c.presentSpecies)
            assert(status==S::ConditionalOriginalTradeTemplateOnly);
        else assert(status==S::CandidateOriginalSpeciesContradicted);
        ++checks;
    }
    assert(checks>66000);
    std::cout<<"L1 Stage2 GSC original NPC trade: "<<checks
             <<" PASS (source templates only, no global legality)\n";
}
