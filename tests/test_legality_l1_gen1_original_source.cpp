#include "Legality/Gen1IndexedOriginalSourceHistory.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main(){
    namespace H=Legality::Gen1IndexedOriginalSourceHistory;
    namespace E=Legality::Gen1Encounter;
    using Status=H::Status;
    static_assert(E::kStaticEntries.size()==31);
    static_assert(E::kTradeEntries.size()==17);
    std::size_t checked=0;
    // Every original exact R/B/Y source row is checked in a title actually
    // containing it, with distinct original/current species and the original
    // acquisition catch-rate byte (not today's mutable PK1 byte).
    for(std::size_t i=0;i<E::kStaticEntries.size();++i){
        const auto& row=E::kStaticEntries[i];
        for(int game=0;game<3;++game){
            const auto mask=static_cast<uint8_t>(1u<<game);
            if((row.nativeGameMask&mask)==0)continue;
            const char* id=game==0?"red_gb":game==1?"blue_gb":"yellow_gb";
            H::Candidate c{};
            c.kind=H::Kind::Static;c.exactOriginalGame=id;
            c.originalRowIndex=i;c.originalSpecies=row.species;
            c.presentSpecies=row.species;c.originalLevel=row.minimumLevel;
            c.originalCatchRate=Legality::Gen1CatchRate::expectedRate(id,row.species);
            c.originalGameIndependentlyAttested=true;
            c.originalSourceRowIndependentlyAttested=true;
            c.originalCatchRateIndependentlyAttested=true;
            assert(H::evaluate(c).status==Status::ConditionalOriginalSource);
            auto missing=c;missing.originalSourceRowIndependentlyAttested=false;
            assert(H::evaluate(missing).status==Status::UnknownOriginalRow);
            missing=c;missing.originalCatchRateIndependentlyAttested=false;
            assert(H::evaluate(missing).status==Status::UnknownOriginalCatchRate);
            auto bad=c;bad.originalSpecies=0;
            assert(H::evaluate(bad).status==Status::CandidateOriginalSpeciesContradicted);
            bad=c;bad.originalLevel=0;
            assert(H::evaluate(bad).status==Status::CandidateOriginalLevelContradicted);
            bad=c;bad.originalCatchRate^=1u;
            assert(H::evaluate(bad).status==
                Status::CandidateOriginalCatchRateContradicted);
            ++checked;
        }
    }
    for(std::size_t i=0;i<E::kTradeEntries.size();++i){
        const auto& row=E::kTradeEntries[i];
        const char* id=(row.nativeGameMask&1u)?"red_gb":"yellow_gb";
        H::Candidate c{};
        c.kind=H::Kind::InGameTrade;c.exactOriginalGame=id;
        c.originalRowIndex=i;c.originalSpecies=row.species;
        c.presentSpecies=row.species;c.originalLevel=row.minimumLevelRBY;
        c.originalCatchRate=Legality::Gen1CatchRate::expectedRate(id,row.species);
        c.originalGameIndependentlyAttested=true;
        c.originalSourceRowIndependentlyAttested=true;
        c.originalTradeOTIndependentlyAttested=true;
        c.noTimeCapsuleHistoryIndependentlyAttested=true;
        c.originalCatchRateIndependentlyAttested=true;
        assert(H::evaluate(c).status==Status::ConditionalOriginalSource);
        auto unknown=c;unknown.originalTradeOTIndependentlyAttested=false;
        assert(H::evaluate(unknown).status==Status::UnknownTradeOT);
        unknown=c;unknown.noTimeCapsuleHistoryIndependentlyAttested=false;
        assert(H::evaluate(unknown).status==Status::UnknownTradebackHistory);
        ++checked;
    }
    // Yellow-only Surfing Pikachu encounter row must not claim native R/B.
    H::Candidate yellow{};
    yellow.exactOriginalGame="yellow_gb";yellow.originalRowIndex=28;
    yellow.originalSpecies=25;yellow.presentSpecies=25;yellow.originalLevel=5;
    yellow.originalCatchRate=Legality::Gen1CatchRate::expectedRate("yellow_gb",25);
    yellow.originalGameIndependentlyAttested=true;
    yellow.originalSourceRowIndependentlyAttested=true;
    yellow.originalCatchRateIndependentlyAttested=true;
    assert(H::evaluate(yellow).status==Status::ConditionalOriginalSource);
    yellow.exactOriginalGame="blue_gb";
    assert(H::evaluate(yellow).status==Status::ForeignGameRowNotEvidence);
    // Versioned Yellow Machoke original in-game trade can evolve to Machamp;
    // a detached Machamp alone cannot attest the original trade/evolution.
    H::Candidate trade{};
    trade.kind=H::Kind::InGameTrade;trade.exactOriginalGame="yellow_gb";
    trade.originalRowIndex=11;trade.originalSpecies=67;
    trade.presentSpecies=68;trade.originalLevel=16;
    trade.originalCatchRate=Legality::Gen1CatchRate::expectedRate("yellow_gb",67);
    trade.originalGameIndependentlyAttested=true;
    trade.originalSourceRowIndependentlyAttested=true;
    trade.originalTradeOTIndependentlyAttested=true;
    trade.noTimeCapsuleHistoryIndependentlyAttested=true;
    trade.originalCatchRateIndependentlyAttested=true;
    assert(H::evaluate(trade).status==Status::UnknownEvolutionChain);
    trade.originalEvolutionChainIndependentlyAttested=true;
    assert(H::evaluate(trade).status==Status::ConditionalOriginalSource);
    auto rateHistory=trade;
    rateHistory.originalLevel=5;
    rateHistory.noTimeCapsuleHistoryIndependentlyAttested=false;
    rateHistory.timeCapsuleHistoryIndependentlyAttested=true;
    assert(H::evaluate(rateHistory).status==Status::ConditionalOriginalSource);
    rateHistory.noTimeCapsuleHistoryIndependentlyAttested=true;
    assert(H::evaluate(rateHistory).status==Status::UnknownTradebackHistory);
    rateHistory.timeCapsuleHistoryIndependentlyAttested=false;
    assert(H::evaluate(rateHistory).status==Status::CandidateOriginalLevelContradicted);
    // Distinct Eevee branches cannot evolve into each other even when
    // a numeric species interval or stage-offset heuristic would include them.
    static_assert(H::possibleOriginalEvolution(133,135));
    static_assert(!H::possibleOriginalEvolution(134,135));
    assert(checked>=70u);
    std::cout<<"L1 E02/E03 exact RBY original static/trade rows: PASS\n";
}
