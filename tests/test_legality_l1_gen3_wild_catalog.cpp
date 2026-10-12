#include "Legality/Gen3OriginalWildSourceStage.h"
#include <cassert>
#include <cstddef>
#include <iostream>
int main(){
    namespace H=Legality::Gen3OriginalWildSourceStage;
    using S=H::Status;
    static_assert(H::kGroups.size()==33);
    std::size_t count=0,valid=0,local=0,unknown=0;
    for(std::size_t i=0;i<H::kGroups.size();++i){
        const auto& g=H::kGroups[i];
        assert(g.packed.size()%6==0);
        for(std::size_t slot=0;slot<H::slotCount(g);++slot){
            const auto row=H::decode(g,slot);
            assert(row.speciesIndex<H::kSpecies.size());
            assert(row.minLevel>0&&row.minLevel<=row.maxLevel);
            H::Candidate c{};c.originalGame=g.game;c.exactGroupIndex=i;c.physicalSlot=slot;
            c.originalMap=g.map;c.originalMethod=g.method;c.originalSpecies=H::kSpecies[row.speciesIndex];
            c.originalLevel=row.minLevel;c.originalRod=H::requiredRod(slot);
            c.originalGameAttested=c.originalGroupAndSlotAttested=true;
            c.originalMapAndMethodAttested=c.originalSpeciesAttested=true;
            c.originalLevelAttested=c.originalRodAttested=true;
            assert(H::evaluate(c).status==S::SourceTemplateCompatibleHistoryUnverified);
            ++valid; ++count;
            auto wrong=c;wrong.originalGame=static_cast<H::Game>((static_cast<unsigned>(g.game)+1)%5);
            assert(H::evaluate(wrong).status==S::ForeignSource);++unknown;
            wrong=c;wrong.originalSpecies="IMPOSSIBLE_SOURCE_SYMBOL";
            assert(H::evaluate(wrong).status==S::CandidateSpeciesContradicted);++local;
            wrong=c;wrong.originalLevel=static_cast<uint8_t>(row.maxLevel+1);
            assert(H::evaluate(wrong).status==S::CandidateLevelContradicted);++local;
            wrong=c;wrong.originalMethod=g.method==H::Method::Grass?H::Method::Surf:H::Method::Grass;
            assert(H::evaluate(wrong).status==S::CandidateMethodContradicted);++local;
            if(g.method==H::Method::Fishing){wrong=c;wrong.originalRod=c.originalRod==H::Rod::Old?H::Rod::Good:H::Rod::Old;
                assert(H::evaluate(wrong).status==S::CandidateRodContradicted);++local;}
            wrong=c;wrong.originalGroupAndSlotAttested=false;
            assert(H::evaluate(wrong).status==S::UnknownOriginalSource);++unknown;
            wrong=c;wrong.originalGameAttested=false;
            assert(H::evaluate(wrong).status==S::UnknownOriginalGame);++unknown;
            wrong=c;wrong.originalSpeciesAttested=false;
            assert(H::evaluate(wrong).status==S::UnknownOriginalIdentity);++unknown;
            wrong=c;wrong.originalLevelAttested=false;
            assert(H::evaluate(wrong).status==S::UnknownOriginalLevel);++unknown;
            wrong=c;wrong.originalMapAndMethodAttested=false;
            assert(H::evaluate(wrong).status==S::UnknownOriginalSource);++unknown;
            wrong=c;wrong.originalMap="UNMODELED_ORIGIN";
            assert(H::evaluate(wrong).status==S::UnknownOriginalSource);++unknown;
            if(g.method==H::Method::Fishing){wrong=c;wrong.originalRodAttested=false;
                assert(H::evaluate(wrong).status==S::UnknownOriginalRod);++unknown;}
        }
    }
    assert(count==306&&valid==306&&local>=918&&unknown>=1836);
    H::Candidate invalid{};invalid.originalGame=H::Game::Ruby;invalid.originalGameAttested=true;
    invalid.originalGroupAndSlotAttested=true;invalid.exactGroupIndex=999;
    assert(H::evaluate(invalid).status==S::UnknownOriginalSource);
    invalid.exactGroupIndex=0;invalid.physicalSlot=99;
    assert(H::evaluate(invalid).status==S::UnknownOriginalSource);
    std::cout<<"L1 Gen3 original wild 33 exact source groups / "<<count<<" slots / "<<local
             <<" candidate negatives / "<<unknown<<" unknown controls: PASS\n";
}
