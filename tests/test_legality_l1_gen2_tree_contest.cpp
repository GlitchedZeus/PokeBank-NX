#include "Legality/Gen2OriginalTreeContestStage.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main() {
    namespace H=Legality::Gen2OriginalTreeContestStage;
    using S=H::Status;
    static_assert(H::kTreeRows.size()==72);
    static_assert(H::kMapRows.size()==6);
    static_assert(H::kContestRows.size()==10);
    constexpr std::array<const char*,3> games{"gold_gbc","silver_gbc","crystal_gbc"};
    std::size_t treePos=0, contestPos=0, localNeg=0, unknown=0;
    for(unsigned game=0;game<3;++game) {
        const auto mask=static_cast<uint8_t>(1u<<game);
        for(std::size_t mapIndex=0;mapIndex<H::kMapRows.size();++mapIndex) {
            const auto& map=H::kMapRows[mapIndex];
            if(!(map.gameMask&mask))continue;
            for(std::size_t treeIndex=0;treeIndex<H::kTreeRows.size();++treeIndex) {
                const auto& row=H::kTreeRows[treeIndex];
                if(!(row.gameMask&mask) || row.set!=map.set)continue;
                H::TreeCandidate c{};
                c.originalGame=games[game]; c.originalMap=map.map;
                c.originalMapRow=mapIndex;c.originalTreeRow=treeIndex;
                c.rare=row.rare;c.originalSpecies=row.species;c.originalLevel=row.level;
                c.gameAttested=c.mapRowAttested=c.treeRowAttested=true;
                c.originalMapAttested=c.treeEligibleAttested=true;
                c.originalRarityAttested=c.speciesAttested=c.levelAttested=true;
                assert(H::evaluateTree(c).status==S::SourceRowCompatibleHistoryUnverified);
                ++treePos;
                auto missing=c;missing.treeEligibleAttested=false;
                assert(H::evaluateTree(missing).status==S::UnknownOriginalTreeEligibility);
                missing=c;missing.originalRarityAttested=false;
                assert(H::evaluateTree(missing).status==S::UnknownRarityWitness);
                missing=c;missing.mapRowAttested=false;
                assert(H::evaluateTree(missing).status==S::UnknownOriginalMapRow);
                missing=c;missing.treeRowAttested=false;
                assert(H::evaluateTree(missing).status==S::UnknownOriginalTreeRow);
                missing=c;missing.originalMapAttested=false;
                assert(H::evaluateTree(missing).status==S::UnknownMapWitness);
                missing=c;missing.speciesAttested=false;
                assert(H::evaluateTree(missing).status==S::UnknownOriginalSpecies);
                missing=c;missing.levelAttested=false;
                assert(H::evaluateTree(missing).status==S::UnknownOriginalLevel);
                missing=c;missing.originalSpecies=0;
                assert(H::evaluateTree(missing).status==S::UnknownOriginalSpecies);
                missing=c;missing.originalLevel=101;
                assert(H::evaluateTree(missing).status==S::UnknownOriginalLevel);
                missing=c;missing.gameAttested=false;
                assert(H::evaluateTree(missing).status==S::UnknownOriginalGame);
                unknown+=10;
                auto bad=c;bad.originalMap="OtherMap";
                assert(H::evaluateTree(bad).status==S::CandidateMapContradicted);
                bad=c;bad.rare=!c.rare;
                assert(H::evaluateTree(bad).status==S::CandidateRarityContradicted);
                bad=c;bad.originalSpecies=1;
                assert(H::evaluateTree(bad).status==S::CandidateSpeciesContradicted);
                bad=c;bad.originalLevel=11;
                assert(H::evaluateTree(bad).status==S::CandidateLevelContradicted);
                localNeg+=4;
            }
        }
        for(std::size_t row=0;row<H::kContestRows.size();++row) {
            const auto& r=H::kContestRows[row];
            H::ContestCandidate c{};
            c.originalGame=games[game];c.originalContestRow=row;
            c.gameAttested=c.rowAttested=c.contestSessionAttested=true;
            c.speciesAttested=c.levelAttested=true;
            c.originalSpecies=r.species;
            for(const uint8_t lev : {r.minimum,r.maximum}) {
                c.originalLevel=lev;
                assert(H::evaluateContest(c).status==S::SourceRowCompatibleHistoryUnverified);
                ++contestPos;
            }
            c.originalLevel=r.minimum;
            auto missing=c;missing.contestSessionAttested=false;
            assert(H::evaluateContest(missing).status==S::UnknownContestSession);
            missing=c;missing.speciesAttested=false;
            assert(H::evaluateContest(missing).status==S::UnknownOriginalSpecies);
            missing=c;missing.levelAttested=false;
            assert(H::evaluateContest(missing).status==S::UnknownOriginalLevel);
            unknown+=3;
            auto bad=c;bad.originalSpecies=1;
            assert(H::evaluateContest(bad).status==S::CandidateSpeciesContradicted);
            bad=c;bad.originalLevel=static_cast<uint8_t>(r.minimum-1);
            assert(H::evaluateContest(bad).status==S::CandidateLevelContradicted);
            bad=c;bad.originalLevel=static_cast<uint8_t>(r.maximum+1);
            assert(H::evaluateContest(bad).status==S::CandidateLevelContradicted);
            localNeg+=3;
        }
    }
    // G/S source forest identities cannot silently become Crystal sources.
    H::TreeCandidate foreign{};
    foreign.originalGame="crystal_gbc"; foreign.originalMap="IlexForest";
    foreign.originalMapRow=5;foreign.originalTreeRow=24; // Gold forest start
    foreign.gameAttested=foreign.mapRowAttested=foreign.treeRowAttested=true;
    assert(H::evaluateTree(foreign).status==S::ForeignGameRowNotEvidence);
    foreign.originalTreeRow=H::kTreeRows.size();
    assert(H::evaluateTree(foreign).status==S::UnknownOriginalTreeRow);
    H::ContestCandidate invalid{};
    invalid.originalGame="crystal_gbc";invalid.gameAttested=true;
    invalid.rowAttested=true;invalid.originalContestRow=H::kContestRows.size();
    assert(H::evaluateContest(invalid).status==S::UnknownOriginalTreeRow);
    assert(treePos==108);
    assert(contestPos==60);
    assert(localNeg==522);
    assert(unknown==1170);
    std::cout<<"L1 GSC tree/contest source: "<<treePos<<" tree + "<<contestPos
             <<" contest positives, "<<localNeg<<" local negatives, "
             <<unknown<<" unknown checks PASS\n";
}
