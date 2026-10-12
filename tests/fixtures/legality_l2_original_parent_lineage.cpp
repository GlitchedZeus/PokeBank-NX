#include "Legality/Gen34OriginalParentLineage.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen34OriginalParentLineage;
namespace R=Legality::Gen34OriginalSameRouteProof;
using S=L::Snapshot;
L::Input base(R::Game game,unsigned seed) {
    L::Input in{};
    in.supplied=true;
    in.originalEventOrderingIndependentlyAttested=true;
    in.originalEggEventSequence=10000+seed;
    for(unsigned j=0;j<2;++j) {
        auto& p=in.physicalSlots[j];
        p.supplied=true;
        p.originalOriginIndependentlyAttested=true;
        p.originalDepositIndependentlyAttested=true;
        p.originalOrigin={100+j,game,static_cast<uint16_t>(j?132:25)};
        p.deposited=p.originalOrigin;
    }
    return in;
}
L::Expected expected(const L::Input& in,R::Game game) {
    return {game,{in.physicalSlots[0].deposited.identity,
                  in.physicalSlots[1].deposited.identity},
                 {in.physicalSlots[0].deposited.species,
                  in.physicalSlots[1].deposited.species}};
}
void append(L::Parent& p,L::TransitionKind kind,uint64_t sequence,
            S next,bool method=false){
    auto& t=p.history[p.transitions++];
    t={true,true,method,kind,sequence,p.transitions==1?p.originalOrigin:
       p.history[p.transitions-2].after,next};
    p.deposited=next;
}
int main(){
    uint64_t checks=0;
    for(unsigned g=1;g<=10;++g)
    for(unsigned seed=1;seed<=1024;++seed){
        const auto game=static_cast<R::Game>(g);
        auto in=base(game,seed);
        auto wanted=expected(in,game);
        assert(L::evaluate(in,wanted).status==L::Status::ConditionallyReplayed);++checks;
        auto traded=in;
        append(traded.physicalSlots[0],L::TransitionKind::Trade,seed,
               {500+seed,game,25});
        assert(L::evaluate(traded,expected(traded,game)).status==
               L::Status::ConditionallyReplayed);++checks;
        auto wrong=traded;wrong.physicalSlots[0].history[0].after.species=26;
        assert(L::evaluate(wrong,expected(traded,game)).status==
               L::Status::ContradictedOriginalRoute);++checks;
        wrong=traded;wrong.physicalSlots[0].history[0].before.identity++;
        assert(L::evaluate(wrong,expected(traded,game)).status==
               L::Status::CrossedBiologicalIdentity);++checks;
        wrong=traded;wrong.physicalSlots[0].history[0].sameIndividualIndependentlyAttested=false;
        assert(L::evaluate(wrong,expected(traded,game)).status==
               L::Status::UnknownOriginalHistory);++checks;
        wrong=traded;wrong.physicalSlots[0].history[0].originalSequence=in.originalEggEventSequence;
        assert(L::evaluate(wrong,expected(traded,game)).status==
               L::Status::ContradictedOriginalRoute);++checks;
        wrong=traded;wrong.originalEventOrderingIndependentlyAttested=false;
        assert(L::evaluate(wrong,expected(traded,game)).status==
               L::Status::UnknownOriginalHistory);++checks;
        wrong=traded;wrong.physicalSlots[0].deposited.identity++;
        assert(L::evaluate(wrong,expected(traded,game)).status==
               L::Status::CrossedBiologicalIdentity);++checks;
        auto evolved=base(game,seed);
        evolved.physicalSlots[0].originalOrigin.species=172;
        append(evolved.physicalSlots[0],L::TransitionKind::Evolution,seed,
               {500+seed,game,25},true);
        assert(L::evaluate(evolved,expected(evolved,game)).status==
               L::Status::ConditionallyReplayed);++checks;
        wrong=evolved;wrong.physicalSlots[0].history[0].evolutionMethodIndependentlyVerified=false;
        assert(L::evaluate(wrong,expected(evolved,game)).status==
               L::Status::UnknownOriginalHistory);++checks;
        wrong=evolved;wrong.physicalSlots[0].history[0].after.species=172;
        assert(L::evaluate(wrong,expected(evolved,game)).status==
               L::Status::ContradictedOriginalRoute);++checks;
        wrong=evolved;wrong.physicalSlots[0].history[0].after.game=
            R::Game::Unknown;
        assert(L::evaluate(wrong,expected(evolved,game)).status==
               L::Status::UnknownOriginalHistory);++checks;
        wrong=evolved;wrong.physicalSlots[0].transitions=5;
        assert(L::evaluate(wrong,expected(evolved,game)).status==
               L::Status::UnknownOriginalHistory);++checks;
        wrong=in;wrong.physicalSlots[0].supplied=false;
        assert(L::evaluate(wrong,wanted).status==L::Status::UnknownOriginalHistory);++checks;
        wrong=in;wrong.supplied=false;
        assert(L::evaluate(wrong,wanted).status==L::Status::NotSupplied);++checks;
    }
    for(unsigned g=1;g<=5;++g)for(unsigned seed=1;seed<=1024;++seed){
        auto original=static_cast<R::Game>(g);
        const auto dp=R::Game::Diamond;
        auto in=base(original,seed);
        append(in.physicalSlots[0],L::TransitionKind::PalPark,seed,
               {500+seed,dp,25});
        append(in.physicalSlots[1],L::TransitionKind::PalPark,seed,
               {1000+seed,dp,132});
        auto e=expected(in,dp);
        assert(L::evaluate(in,e).status==L::Status::ConditionallyReplayed);++checks;
        auto wrong=in;wrong.physicalSlots[0].history[0].after.game=original;
        assert(L::evaluate(wrong,e).status==L::Status::ContradictedOriginalRoute);++checks;
        wrong=in;wrong.physicalSlots[0].history[0].after.species=26;
        assert(L::evaluate(wrong,e).status==L::Status::ContradictedOriginalRoute);++checks;
        wrong=in;wrong.physicalSlots[1].history[0].independentlyAttested=false;
        assert(L::evaluate(wrong,e).status==L::Status::UnknownOriginalHistory);++checks;
        wrong=in;wrong.physicalSlots[0].history[0].originalSequence=in.originalEggEventSequence;
        assert(L::evaluate(wrong,e).status==L::Status::ContradictedOriginalRoute);++checks;
        wrong=in;wrong.physicalSlots[0].history[0].before.game=R::Game::HeartGold;
        assert(L::evaluate(wrong,e).status==L::Status::CrossedBiologicalIdentity);++checks;
    }
    std::cout << "L2 Stage6 original lineage assertions: " << checks << " PASS\n";
    assert(checks==184320);
}
