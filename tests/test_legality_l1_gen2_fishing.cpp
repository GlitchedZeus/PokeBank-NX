#include "Legality/Gen2OriginalFishingStage.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main() {
    namespace H=Legality::Gen2OriginalFishingStage;
    using S=H::Status;
    static_assert(H::kOriginalSlots.size()==145);
    constexpr std::array<const char*,3> games{"gold_gbc","silver_gbc","crystal_gbc"};
    std::size_t positive=0,negative=0,unknown=0;
    for(const auto game:games) {
        for(std::size_t idx=0;idx<H::kOriginalSlots.size();++idx) {
            const auto& row=H::kOriginalSlots[idx];
            H::Candidate c{};
            c.exactOriginalGame=game;c.originalRowIndex=idx;
            c.originalFishGroup=row.fishGroup;c.originalRod=row.rod;
            c.originalNight=row.phase==H::DayPhase::Night;
            c.originalSwarmActive=row.fishGroup==5 || row.fishGroup==6;
            c.originalSpecies=row.species;c.originalLevel=row.level;
            c.gameAttested=c.rowAttested=c.groupAttested=c.rodAttested=true;
            c.dayNightAttested=c.swarmStateAttested=true;
            c.speciesAttested=c.levelAttested=true;
            assert(H::evaluate(c).status==S::SourceSlotCompatibleWorldAndRngUnverified);
            ++positive;
            auto bad=c;bad.originalRod=row.rod==H::Rod::Old?H::Rod::Good:H::Rod::Old;
            assert(H::evaluate(bad).status==S::CandidateRodContradicted);
            bad=c;bad.originalSpecies=1;
            assert(H::evaluate(bad).status==S::CandidateSpeciesContradicted);
            bad=c;bad.originalLevel=static_cast<uint8_t>(row.level+1);
            assert(H::evaluate(bad).status==S::CandidateLevelContradicted);
            negative+=3;
            if(row.phase!=H::DayPhase::Any) {
                bad=c;bad.originalNight=!c.originalNight;
                assert(H::evaluate(bad).status==S::CandidateDayNightContradicted);
                ++negative;
                auto missing=c;missing.dayNightAttested=false;
                assert(H::evaluate(missing).status==S::UnknownOriginalDayNight);
                ++unknown;
            }
            if(row.fishGroup==5u || row.fishGroup==6u || row.fishGroup==12u) {
                bad=c;bad.originalSwarmActive=!c.originalSwarmActive;
                assert(H::evaluate(bad).status==S::CandidateSwarmContradicted);
                ++negative;
                auto missing=c;missing.swarmStateAttested=false;
                assert(H::evaluate(missing).status==S::UnknownOriginalSwarm);
                ++unknown;
            }
            auto missing=c;missing.gameAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalGame);
            missing=c;missing.rowAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalSlot);
            missing=c;missing.groupAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalGroup);
            missing=c;missing.rodAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalRod);
            missing=c;missing.speciesAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalSpecies);
            missing=c;missing.levelAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalLevel);
            unknown+=6;
            missing=c;missing.originalLevel=0;
            assert(H::evaluate(missing).status==S::UnknownOriginalLevel);
            missing=c;missing.originalSpecies=0;
            assert(H::evaluate(missing).status==S::UnknownOriginalSpecies);
            unknown+=2;
        }
    }
    H::Candidate malformed{};
    malformed.gameAttested=malformed.rowAttested=malformed.groupAttested=true;
    malformed.exactOriginalGame="gold_gbc";
    malformed.originalRowIndex=H::kOriginalSlots.size();
    assert(H::evaluate(malformed).status==S::UnknownOriginalSlot);
    malformed.originalRowIndex=0;
    malformed.originalFishGroup=255;
    assert(H::evaluate(malformed).status==S::UnknownOriginalGroup);
    malformed.originalFishGroup=0;malformed.rodAttested=true;
    malformed.originalRod=static_cast<H::Rod>(255);
    assert(H::evaluate(malformed).status==S::UnknownOriginalRod);
    assert(positive==435);
    assert(negative>1400);
    assert(unknown>3500);
    std::cout<<"L1 GSC original rod: "<<positive<<" positives, "<<negative
             <<" local negatives, "<<unknown<<" Unknown conditions PASS\n";
}
