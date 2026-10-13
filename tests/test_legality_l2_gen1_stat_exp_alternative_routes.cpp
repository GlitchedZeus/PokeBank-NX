#include "Legality/Gen1OriginalStatExpRouteAlternatives.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen1OriginalStatExpRouteAlternatives;
namespace H=Legality::Gen1OriginalStatExpBattleHistory;
namespace G=PokeVault::Integration::Gen1;
static G::PokemonRecord pk1(bool party,unsigned current){
    G::PokemonRecord p{};
    p.species=1;p.rawSpecies=p.rawBody[0]=0x99;
    p.partyRecord=party;p.rawBodySize=party?44u:33u;
    p.statExperience[0]=static_cast<std::uint16_t>(current);
    p.rawBody[0x11]=static_cast<std::uint8_t>(current>>8u);
    p.rawBody[0x12]=static_cast<std::uint8_t>(current);
    return p;
}
static A::Candidate vitaminRoute(G::SourceGame game,std::uint64_t id,unsigned after){
    A::Candidate c{};c.independentlyIdentifiedRoute=id;
    c.routeIdentityIndependentlyAttested=true;
    auto& h=c.history;
    h.strictPK1IndependentlyVerified=true;
    h.originalInitialSnapshotIndependentlyAttested=true;
    h.allOriginalStatExpEventsIndependentlyEnumerated=true;
    h.sourceGame=game;
    h.count=1;
    auto& e=h.events[0];
    e.kind=H::EventKind::HPUp;
    e.originalEventId=id+100;
    e.originalMonotonicEventSequence=id+200;
    e.originalGame=game;
    e.originalOrderingIndependentlyAttested=true;
    e.originalVitaminItemIndependentlyAttested=true;
    e.originalVitaminConsumedIndependentlyAttested=true;
    e.originalAfterStatExpIndependentlyAttested=true;
    e.originalAfterStatExp[0]=static_cast<std::uint16_t>(after);
    return c;
}
int main(){
    std::uint64_t tests=0;
    for(unsigned game=0;game<3;++game)
    for(std::uint64_t seed=1;seed<=4096;++seed)
    for(bool party:{false,true}){
        const auto g=static_cast<G::SourceGame>(game);
        auto p=pk1(party,2560);
        A::Input input{};input.count=2;
        input.allPossibleOriginalRoutesIndependentlyEnumerated=true;
        input.routes[0]=vitaminRoute(g,seed*10+1,2500);
        input.routes[1]=vitaminRoute(g,seed*10+2,2560);
        auto r=A::evaluate(p,input);
        assert(r.status==A::Status::OneSupportedOriginalHistory);
        assert(r.supported==1 && r.contradicted==1 && r.unknown==0);
        tests+=2;
        auto x=input;x.routes[1].history.events[0].originalAfterStatExp[0]=0;
        r=A::evaluate(p,x);
        assert(r.status==A::Status::AllAttestedOriginalHistoriesContradicted);
        assert(r.contradicted==2);tests+=2;
        x.allPossibleOriginalRoutesIndependentlyEnumerated=false;
        assert(A::evaluate(p,x).status==A::Status::UnknownOriginalRouteEnumeration);
        ++tests;
        x=input;x.routes[1].history.events[0].originalVitaminItemIndependentlyAttested=false;
        r=A::evaluate(p,x);
        assert(r.status==A::Status::UnresolvedOriginalHistories);
        assert(r.contradicted==1 && r.unknown==1);tests+=2;
        x=input;x.routes[1].independentlyIdentifiedRoute=x.routes[0].independentlyIdentifiedRoute;
        r=A::evaluate(p,x);
        assert(r.status==A::Status::UnresolvedOriginalHistories);
        assert(r.unknown==1 && r.contradicted==1);tests+=2;
        x=input;x.routes[1].routeIdentityIndependentlyAttested=false;
        assert(A::evaluate(p,x).status==A::Status::UnresolvedOriginalHistories);
        ++tests;
        x=input;x.count=9;
        assert(A::evaluate(p,x).status==A::Status::SourceCapacityUnknown);++tests;
        x=input;x.count=0;
        assert(A::evaluate(p,x).status==A::Status::NoOriginalRoutes);++tests;
        x=input;x.routes[0].history.events[0].originalVitaminItemIndependentlyAttested=false;
        r=A::evaluate(p,x);
        assert(r.status==A::Status::OneSupportedOriginalHistory);
        assert(r.unknown==1 && r.supported==1);tests+=2;
        x=input;x.routes[1].history.originalInitialSnapshotIndependentlyAttested=false;
        assert(A::evaluate(p,x).status==A::Status::UnresolvedOriginalHistories);
        ++tests;
        x=input;x.routes[1].history.events[0].originalMonotonicEventSequence=0;
        assert(A::evaluate(p,x).status==A::Status::UnresolvedOriginalHistories);
        ++tests;
    }
    std::cout<<"Gen I original StatExp route-local alternatives: "<<tests<<" checks PASS\n";
}
