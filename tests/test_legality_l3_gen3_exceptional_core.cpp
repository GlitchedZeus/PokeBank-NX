// Real algorithm, independently source-matched small Ruby/FRLG fixtures;
// no proof of a real trainer's history or of source-retail authentication.
#include "Legality/Gen3GbaExceptionalHistoryCore.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace X=Legality::Gen3GbaExceptionalHistoryCore;
namespace H=X::H;
using G=X::Game;using S=X::Status;using A=H::Action;
namespace {
constexpr std::array<std::uint16_t,58> machines=[] {
    std::array<std::uint16_t,58> m{};
    m.fill(1);m[5]=92;m[9]=237;m[50]=15;return m;
}();
// Selected ordered original Ruby/FRLG source rows, complete in the covered
// level windows. Exact full 386-species verification is separate native gate.
constexpr std::array<H::Offer,6> bulba{{{1,33},{4,45},{7,73},{10,22},{15,77},{15,79}}};
constexpr std::array<H::Offer,7> ivy{{{1,33},{4,45},{7,73},{10,22},{15,77},{15,79},{22,75}}};
constexpr std::array<H::Offer,2> nincada{{{14,154},{19,170}}};
constexpr std::array<H::Offer,4> ninjask{{{19,170},{20,104},{20,210},{20,103}}};
constexpr std::array<H::Offer,2> shedinja{{{19,170},{25,180}}};
constexpr H::Catalog cat(G g,std::uint16_t sp,std::span<const H::Offer> offers) {
    return {g,sp,offers,machines,std::uint64_t{1}<<5,true,true,true,true};
}
constexpr H::History history(G g,std::uint16_t sp,std::uint32_t id,
                             std::uint8_t low,std::uint8_t high,
                             std::array<std::uint16_t,4> start,
                             std::array<std::uint16_t,4> end) {
    H::History h{};h.candidateId=id;h.game=g;h.species=sp;
    h.initialLevel=low;h.observedLevel=high;
    h.initialMoves=start;h.observedMoves=end;
    h.initialSnapshotVerified=h.samePokemonAndGameVerified=
        h.allPlayerActionsVerified=true;
    return h;
}
constexpr X::Edge edge(X::EdgeKind k,G from,G to,std::uint16_t a,std::uint16_t b,
                       std::uint8_t level,std::uint32_t original,std::uint32_t next) {
    X::Edge e{};e.kind=k;e.fromGame=from;e.toGame=to;
    e.fromSpecies=a;e.toSpecies=b;e.level=level;
    e.fromCandidate=original;e.toCandidate=next;
    e.originalTransitionAttested=e.moveSlotsImmutableAcrossEdgeAttested=true;
    e.samePokemonAttested=k!=X::EdgeKind::ShedinjaSibling;
    e.allImmediateOffersAttested=true;
    return e;
}
constexpr X::Witness bulbasaurEvolutionTradeAndService() {
    X::Witness w{};w.phaseCount=3;w.entireClaimedHistoryAttested=true;
    auto& a=w.phases[0];
    a.catalog=cat(G::Ruby,1,bulba);
    a.history=history(G::Ruby,1,1007,5,16,{33,45,0,0},{33,45,73,22});
    a.history.stepCount=4;
    a.history.steps[0]={G::Ruby,7,A::LevelLearn,73,2,0,true};
    a.history.steps[1]={G::Ruby,10,A::LevelLearn,22,3,0,true};
    a.history.steps[2]={G::Ruby,15,A::LevelDecline,77,0,0,true};
    a.history.steps[3]={G::Ruby,15,A::LevelDecline,79,0,0,true};
    auto& b=w.phases[1];
    b.catalog=cat(G::Ruby,2,ivy);
    b.history=history(G::Ruby,2,1007,16,20,{33,45,73,22},{33,45,73,22});
    w.edges[0]=edge(X::EdgeKind::Evolve,G::Ruby,G::Ruby,1,2,16,1007,1007);
    auto& c=w.phases[2];
    c.catalog=cat(G::FireRed,2,ivy);
    c.history=history(G::FireRed,2,1007,20,25,{33,45,73,22},{92,45,22,0});
    c.history.stepCount=4;
    c.history.steps[0]={G::FireRed,22,A::LevelLearn,75,1,0,true};
    c.history.steps[1]={G::FireRed,23,A::TeachTM,92,0,6,true};
    c.history.steps[2]={G::FireRed,24,A::Relearn,45,2,0,true};
    c.history.steps[3]={G::FireRed,25,A::DeleteMove,0,1,0,true};
    w.edges[1]=edge(X::EdgeKind::CartridgeVisit,G::Ruby,G::FireRed,2,2,20,1007,1007);
    return w;
}
constexpr X::Witness splitShedinjaSibling() {
    X::Witness w{};w.entireClaimedHistoryAttested=true;w.phaseCount=3;
    w.phases[0].catalog=cat(G::Ruby,290,nincada);
    w.phases[0].history=history(G::Ruby,290,2077,19,20,{10,106,141,170},{10,106,141,170});
    w.phases[1].catalog=cat(G::Ruby,291,ninjask);
    w.phases[1].history=history(G::Ruby,291,2077,20,20,{104,210,141,170},{104,210,141,170});
    auto& e=w.edges[0];e=edge(X::EdgeKind::Evolve,G::Ruby,G::Ruby,290,291,20,2077,2077);
    e.offerCount=3;
    e.immediateOffers[0]={20,104,X::OfferChoice::Learn,0,true};
    e.immediateOffers[1]={20,210,X::OfferChoice::Learn,1,true};
    e.immediateOffers[2]={20,103,X::OfferChoice::Decline,0,true};
    w.phases[2].catalog=cat(G::Ruby,292,shedinja);
    w.phases[2].history=history(G::Ruby,292,2078,20,25,{104,210,141,170},{104,210,180,170});
    w.phases[2].history.stepCount=1;
    w.phases[2].history.steps[0]={G::Ruby,25,A::LevelLearn,180,2,0,true};
    w.edges[1]=edge(X::EdgeKind::ShedinjaSibling,G::Ruby,G::Ruby,291,292,20,2077,2078);
    w.edges[1].nincadaToNinjaskAlreadyAttested=true;
    w.edges[1].partyCapacityAndPokeballAttested=true;
    w.edges[1].copiedAfterNinjaskOffersAttested=true;
    return w;
}
static_assert(X::evaluate(bulbasaurEvolutionTradeAndService()).status==S::CandidateConsistent);
static_assert(X::evaluate(splitShedinjaSibling()).status==S::CandidateConsistent);
static_assert([]{auto w=splitShedinjaSibling();w.edges[1].toCandidate=2077;
    return X::evaluate(w).status==S::Unknown;}());
static_assert([]{auto w=bulbasaurEvolutionTradeAndService();w.edges[1].toGame=G::Ruby;
    return X::evaluate(w).status==S::Unknown;}());
static_assert([]{auto w=bulbasaurEvolutionTradeAndService();w.phases[2].history.steps[1].machineNumber=10;
    return X::evaluate(w).status==S::CandidateContradicted;}());
static_assert([]{auto w=bulbasaurEvolutionTradeAndService();w.phases[2].history.steps[2].originalActionReceiptVerified=false;
    return X::evaluate(w).status==S::Unknown;}());
void verify(bool expected) {assert(expected);}
}
int main() {
    std::uint64_t checks=0;
    for(unsigned i=0;i<4096;++i) {
        auto a=bulbasaurEvolutionTradeAndService();
        const unsigned k=i%16;
        if(k==0)a.phases[0].history.steps[0].move=92;
        if(k==1)a.phases[0].history.steps[2].action=A::LevelLearn;
        if(k==2)a.phases[1].history.initialMoves[0]=92;
        if(k==3)a.phases[2].history.initialMoves[0]=92;
        if(k==4)a.phases[2].history.steps[1].machineNumber=10;
        if(k==5)a.phases[2].history.steps[2].move=77;
        if(k==6)a.phases[2].history.steps[3].slot=3;
        if(k==7)a.edges[0].fromSpecies=4;
        if(k==8)a.edges[1].samePokemonAttested=false;
        if(k==9)a.phases[2].catalog.originalSourceVerified=false;
        if(k==10)a.phases[2].history.steps[3].originalActionReceiptVerified=false;
        if(k==11)a.phases[1].history.candidateId=42;
        if(k==12)a.edges[1].toGame=G::Ruby;
        if(k==13)a.phases[2].history.observedMoves={92,45,73,22};
        if(k==14)a.phases[2].history.steps[2].action=A::UnknownService;
        if(k==15)a.phases[2].history.stepCount=3;
        const auto result=X::evaluate(a);
        verify(result.status!=S::CandidateConsistent);
        verify(!result.globalInvalidAuthorized);
        ++checks;
        auto b=splitShedinjaSibling();
        const unsigned s=(i/16)%16;
        if(s==0)b.edges[0].immediateOffers[0].move=103;
        if(s==1)b.edges[0].offerCount=2;
        if(s==2)b.edges[0].immediateOffers[1].slot=3;
        if(s==3)b.edges[0].immediateOffers[2].choice=X::OfferChoice::Learn;
        if(s==4)b.edges[0].samePokemonAttested=false;
        if(s==5)b.edges[1].samePokemonAttested=true;
        if(s==6)b.edges[1].toCandidate=2077;
        if(s==7)b.edges[1].partyCapacityAndPokeballAttested=false;
        if(s==8)b.edges[1].copiedAfterNinjaskOffersAttested=false;
        if(s==9)b.edges[1].nincadaToNinjaskAlreadyAttested=false;
        if(s==10)b.phases[2].history.initialMoves[0]=10;
        if(s==11)b.phases[2].history.candidateId=2077;
        if(s==12)b.phases[2].history.steps[0].move=103;
        if(s==13)b.edges[0].immediateOffers[0].choiceAuthenticated=false;
        if(s==14)b.phases[0].history.initialMoves={0,0,0,0};
        if(s==15)b.edges[1].level=21;
        verify(X::evaluate(b).status!=S::CandidateConsistent);
        ++checks;
    }
    for(unsigned m=0;m<1024;++m) {
        auto w=bulbasaurEvolutionTradeAndService();
        w.phases[2].history.steps[3].originalActionReceiptVerified=false;
        // Unknown future source must dominate earlier putative contradiction.
        w.phases[0].history.steps[0].move=92;
        verify(X::evaluate(w).status==S::Unknown);
        ++checks;
        X::Alternatives options{};
        options.count=2;options.allGenuineHistoriesEnumerated=false;
        options.candidates[0]=w;
        options.candidates[1]=bulbasaurEvolutionTradeAndService();
        verify(X::evaluate(options)==S::CandidateConsistent);
        options.candidates[1]=w;
        verify(X::evaluate(options)==S::Unknown);
        ++checks;
    }
    verify(checks==10240);
    std::cout<<"L3 Stage4 evolution/cartridge/Shedinja/relearner exceptional route: "
             <<checks<<" source-scoped adversarial groups PASS\n";
}
