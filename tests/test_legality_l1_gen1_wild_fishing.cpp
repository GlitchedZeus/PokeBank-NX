#include "Legality/Gen1OriginalWildFishingStage.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main() {
    namespace H=Legality::Gen1OriginalWildFishingStage;
    using S=H::Status;
    static_assert(H::kVerifiedSlots.size()==46);
    static_assert(H::kVerifiedSlots[1].species==H::kVerifiedSlots[2].species);
    static_assert(H::kVerifiedSlots[1].physicalSlot!=H::kVerifiedSlots[2].physicalSlot);
    std::size_t positives=0, negatives=0, unknowns=0;
    for(std::size_t i=0;i<H::kVerifiedSlots.size();++i) {
        const auto& row=H::kVerifiedSlots[i];
        for(unsigned game=0;game<3;++game) {
            const uint8_t mask=static_cast<uint8_t>(1u<<game);
            H::Candidate c{};
            c.originalGame=game==0?"red_gb":game==1?"blue_gb":"yellow_gb";
            c.originalSourceIndex=i;c.originalMap=row.map;
            c.originalMethod=row.method;c.originalSpecies=row.species;
            c.originalLevel=row.level;
            c.originalGameAttested=c.originalSourceSlotAttested=true;
            c.originalMapAttested=c.originalMethodAttested=true;
            c.originalSpeciesAttested=c.originalLevelAttested=true;
            if((row.gameMask&mask)==0){
                assert(H::evaluate(c).status==S::ForeignGameRowNotEvidence);
                ++unknowns;continue;
            }
            assert(H::evaluate(c).status==S::SourceSlotCompatibleRateAndHistoryUnverified);
            ++positives;
            auto bad=c;bad.originalSpecies=1;
            if(row.species==1)bad.originalSpecies=2;
            assert(H::evaluate(bad).status==S::CandidateOriginalSpeciesContradicted);
            bad=c;bad.originalLevel=static_cast<uint8_t>(row.level+1);
            assert(H::evaluate(bad).status==S::CandidateOriginalLevelContradicted);
            bad=c;bad.originalMap="SomeOtherMap";
            assert(H::evaluate(bad).status==S::CandidateOriginalMapContradicted);
            bad=c;bad.originalMethod=row.method==H::Method::Grass?
                H::Method::SuperRod:H::Method::Grass;
            assert(H::evaluate(bad).status==S::CandidateOriginalMethodContradicted);
            negatives+=4;
            auto missing=c;missing.originalSourceSlotAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalSourceRow);
            missing=c;missing.originalMapAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalMap);
            missing=c;missing.originalMethodAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalMethod);
            missing=c;missing.originalSpeciesAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalSpecies);
            missing=c;missing.originalLevelAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalLevel);
            missing=c;missing.originalGameAttested=false;
            assert(H::evaluate(missing).status==S::UnknownOriginalGame);
            unknowns+=6;
        }
    }
    H::Candidate corrupt{};
    corrupt.originalGame="red_gb";corrupt.originalGameAttested=true;
    corrupt.originalSourceSlotAttested=true;
    corrupt.originalSourceIndex=H::kVerifiedSlots.size();
    assert(H::evaluate(corrupt).status==S::UnknownOriginalSourceRow);
    corrupt.originalSourceIndex=0;
    corrupt.originalMap="Route1";corrupt.originalMapAttested=true;
    corrupt.originalMethod=static_cast<H::Method>(255);
    corrupt.originalMethodAttested=true;
    assert(H::evaluate(corrupt).status==S::UnknownOriginalMethod);
    assert(positives==62); // 20 common RB Route1 + 10 Yellow + 10 Red + 10 Blue + 12 rod
    assert(negatives==positives*4);
    assert(unknowns>250);
    std::cout<<"L1 Gen I source-pinned original wild/rod: "<<positives
             <<" positive, "<<negatives<<" candidate-local, "<<unknowns
             <<" Unknown/foreign assertions PASS\n";
}
