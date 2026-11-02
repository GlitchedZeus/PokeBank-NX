#include "Legality/Gen34OriginalParentLineage.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen34OriginalParentLineage;
namespace R=Legality::Gen34OriginalSameRouteProof;
static L::Input setup(R::Game game,std::uint64_t seed){
    L::Input in{};
    in.supplied=true;
    in.originalEventOrderingIndependentlyAttested=true;
    in.originalEggEventSequence=10000;
    for(unsigned j=0;j<2;++j){
        auto& p=in.physicalSlots[j];
        p.supplied=true;
        p.originalOriginIndependentlyAttested=true;
        p.originalDepositIndependentlyAttested=true;
        p.originalOrigin={seed*100+1+j,game,static_cast<std::uint16_t>(j?132:25)};
        p.deposited=p.originalOrigin;
    }
    return in;
}
static L::Expected expected(const L::Input& in,R::Game game){
    L::Expected e{};
    e.originalDaycareGame=game;
    for(unsigned j=0;j<2;++j){
        e.originalDepositIdentity[j]=in.physicalSlots[j].deposited.identity;
        e.originalDepositSpecies[j]=in.physicalSlots[j].deposited.species;
    }
    return e;
}
static void trade(L::Parent& p,std::uint64_t destination,
                  std::uint64_t sequence){
    auto previous=p.deposited;
    auto later=previous;
    later.identity=destination;
    p.history[p.transitions++]={true,true,false,L::TransitionKind::Trade,
        sequence,previous,later};
    p.deposited=later;
}
int main(){
    std::uint64_t checks=0;
    for(unsigned g=1;g<=10;++g)
    for(std::uint64_t seed=1;seed<=1024;++seed){
        auto game=static_cast<R::Game>(g);
        auto in=setup(game,seed);
        auto good=expected(in,game);
        assert(L::evaluate(in,good).status==L::Status::ConditionallyReplayed);++checks;
        // Two independently labelled final slots, same ORIGINAL biology.
        auto x=in;
        x.physicalSlots[1].originalOrigin.identity=x.physicalSlots[0].originalOrigin.identity;
        x.physicalSlots[1].deposited.identity+=100000;
        trade(x.physicalSlots[1],x.physicalSlots[1].deposited.identity+10,50);
        // Reconnect first transition's before to the shared original.
        x.physicalSlots[1].history[0].before=x.physicalSlots[1].originalOrigin;
        const auto duplicate=L::evaluate(x,expected(x,game));
        assert(duplicate.status==L::Status::CrossedBiologicalIdentity);++checks;
        // The same archive ID in different source-game namespaces is
        // not by itself proof of the same individual.
        auto unrelated=in;
        unrelated.physicalSlots[1].originalOrigin.identity=in.physicalSlots[0].originalOrigin.identity;
        unrelated.physicalSlots[1].originalOrigin.game=R::Game::Ruby;
        if(game==R::Game::Ruby)unrelated.physicalSlots[1].originalOrigin.game=R::Game::Pearl;
        auto prior=unrelated.physicalSlots[1].originalOrigin;
        auto final=unrelated.physicalSlots[1].deposited;
        unrelated.physicalSlots[1].transitions=1;
        unrelated.physicalSlots[1].history[0]={true,true,false,
            L::TransitionKind::PalPark,40,prior,final};
        // Cross-generation provenance is accepted only in III -> IV.
        if(L::gen3(prior.game) && L::gen4(final.game)){
            assert(L::evaluate(unrelated,expected(unrelated,game)).status==
                   L::Status::ConditionallyReplayed);++checks;
        }else{
            assert(L::evaluate(unrelated,expected(unrelated,game)).status!=
                   L::Status::ConditionallyReplayed);++checks;
        }
        // Parent 0's INTERMEDIATE traded snapshot cannot be reused as
        // parent 1's independently archived biological origin.
        auto intermediary=in;
        trade(intermediary.physicalSlots[0],seed*100+700,10);
        trade(intermediary.physicalSlots[0],seed*100+800,20);
        intermediary.physicalSlots[1].originalOrigin.identity=seed*100+700;
        intermediary.physicalSlots[1].deposited.identity=seed*100+700;
        trade(intermediary.physicalSlots[1],seed*100+900,30);
        assert(L::evaluate(intermediary,expected(intermediary,game)).status==
               L::Status::CrossedBiologicalIdentity);++checks;
        auto separate=in;
        trade(separate.physicalSlots[0],seed*100+700,10);
        trade(separate.physicalSlots[0],seed*100+800,20);
        trade(separate.physicalSlots[1],seed*100+900,30);
        assert(L::evaluate(separate,expected(separate,game)).status==
               L::Status::ConditionallyReplayed);++checks;
        separate.physicalSlots[1].history[0].sameIndividualIndependentlyAttested=false;
        assert(L::evaluate(separate,expected(separate,game)).status==
               L::Status::UnknownOriginalHistory);++checks;
    }
    assert(checks==61440);
    std::cout<<"L2 Stage7 custody alias and collision: "<<checks<<" PASS\n";
}
