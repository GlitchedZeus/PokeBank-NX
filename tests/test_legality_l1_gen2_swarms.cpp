#include "Legality/Gen2OriginalSwarmStage.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main() {
    namespace H=Legality::Gen2OriginalSwarmStage;
    using S=H::Status;
    static_assert(H::kSwarmRows.size()==150);
    constexpr std::array<const char*,3> games{"gold_gbc","silver_gbc","crystal_gbc"};
    std::size_t positives=0,localNeg=0,unknown=0,foreign=0;
    for(std::size_t i=0;i<H::kSwarmRows.size();++i){
        const auto& r=H::kSwarmRows[i];
        for(unsigned game=0;game<3;++game){
            H::Candidate c{};
            c.exactOriginalGame=games[game];c.originalRowIndex=i;
            c.originalMap=r.map;c.originalMethod=r.method;
            c.originalTimeGroup=r.timeGroup;
            c.originalSpecies=r.species;c.originalLevel=r.level;
            c.gameAttested=c.rowAttested=c.swarmActiveAtCaptureAttested=true;
            c.timeAttested=c.methodAttested=c.mapAttested=true;
            c.speciesAttested=c.levelAttested=true;
            if(!(r.gameMask&(1u<<game))) {
                assert(H::evaluate(c).status==S::ForeignGameRowNotEvidence);
                ++foreign;
                continue;
            }
            assert(H::evaluate(c).status==S::SourceSwarmRowCompatibleOriginalWorldUnverified);
            ++positives;
            auto lost=c;lost.swarmActiveAtCaptureAttested=false;
            assert(H::evaluate(lost).status==S::UnknownOriginalEvent);
            lost=c;lost.timeAttested=false;
            assert(H::evaluate(lost).status==S::UnknownOriginalTime);
            lost=c;lost.mapAttested=false;
            assert(H::evaluate(lost).status==S::UnknownOriginalMap);
            lost=c;lost.methodAttested=false;
            assert(H::evaluate(lost).status==S::UnknownOriginalMethod);
            lost=c;lost.speciesAttested=false;
            assert(H::evaluate(lost).status==S::UnknownOriginalSpecies);
            lost=c;lost.levelAttested=false;
            assert(H::evaluate(lost).status==S::UnknownOriginalLevel);
            lost=c;lost.rowAttested=false;
            assert(H::evaluate(lost).status==S::UnknownOriginalRow);
            lost=c;lost.gameAttested=false;
            assert(H::evaluate(lost).status==S::UnknownOriginalGame);
            lost=c;lost.originalLevel=0;
            assert(H::evaluate(lost).status==S::UnknownOriginalLevel);
            lost=c;lost.originalTimeGroup=255;
            assert(H::evaluate(lost).status==S::UnknownOriginalTime);
            unknown+=10;
            auto bad=c;bad.originalMethod=r.method==H::Method::Grass?H::Method::Surf:H::Method::Grass;
            assert(H::evaluate(bad).status==S::CandidateMethodContradicted);
            bad=c;bad.originalMap="DifferentPlace";
            assert(H::evaluate(bad).status==S::CandidateMapContradicted);
            bad=c;bad.originalSpecies=1;
            assert(H::evaluate(bad).status==S::CandidateSpeciesContradicted);
            bad=c;bad.originalLevel=static_cast<uint8_t>(r.level+1);
            assert(H::evaluate(bad).status==S::CandidateLevelContradicted);
            localNeg+=4;
            if(r.method==H::Method::Grass){
                bad=c;bad.originalTimeGroup=static_cast<uint8_t>((r.timeGroup+1)%3);
                assert(H::evaluate(bad).status==S::CandidateTimeContradicted);
                ++localNeg;
            }
        }
    }
    // Gold/Silver-only Marill water swarm: not a Crystal source.
    H::Candidate c{};c.exactOriginalGame="crystal_gbc";
    c.gameAttested=c.rowAttested=true;
    c.originalRowIndex=H::kSwarmRows.size()-1;
    assert(H::evaluate(c).status==S::ForeignGameRowNotEvidence);
    c.originalRowIndex=H::kSwarmRows.size();
    assert(H::evaluate(c).status==S::UnknownOriginalRow);
    assert(positives==216);
    assert(foreign==234);
    assert(unknown==2160);
    assert(localNeg==1074);
    std::cout<<"L1 GSC source swarm: "<<positives<<" positives, "<<localNeg
             <<" local negatives, "<<unknown<<" unknown, "<<foreign
             <<" cross-game controls PASS\n";
}
