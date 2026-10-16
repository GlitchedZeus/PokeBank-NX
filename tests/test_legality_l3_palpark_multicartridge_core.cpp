#include "Legality/Gen34PalParkMultiCartridgeCore.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace P=Legality::Gen34PalParkMultiCartridgeCore;
namespace X=P::X;
namespace H=P::H;
using G=X::Game;using S=P::Status;using A=H::Action;using N=P::Game;using B=P::Action;
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
constexpr std::array<P::Offer,7> nativeBulba{{
    {1,33},{4,45},{7,73},{10,22},{15,77},{15,79},{21,230}}};
constexpr std::array<P::Offer,3> nativeIvy{{{1,33},{15,77},{21,230}}};
constexpr std::array<P::Machine,4> nativeMachines{{
    {false,6,92,true},{false,53,412,true},{true,1,15,true},{true,2,19,false}}};
constexpr std::array<std::uint16_t,2> nativeTutors{{235,173}};
constexpr P::Catalog originalPlatinum(std::uint16_t species=1) {
    return {N::Platinum,species,species==1?std::span<const P::Offer>(nativeBulba):
        std::span<const P::Offer>(nativeIvy),nativeMachines,nativeTutors,true,true,true};
}
constexpr std::array<P::Catalog,2> retail{{originalPlatinum(1),originalPlatinum(2)}};
constexpr P::Input transfer(X::Witness w,N destination=N::Platinum) {
    P::Input p{};p.gba=w;
    const auto& last=w.phases[w.phaseCount-1].history;
    p.transferredCandidateId=last.candidateId;
    p.species=last.species;p.arrivalLevel=p.observedLevel=last.observedLevel;
    p.arrivalGame=p.observedGame=destination;
    p.outboundGba=p.immediateNds=p.observedMoves=last.observedMoves;
    p.formContinuityVerified=true;
    p.witnessedSameIndividualAtTransfer=true;
    p.witnessedOutboundSnapshot=true;
    p.witnessedImmediateArrivalSnapshot=true;
    p.witnessedWholeNativeActionHistory=true;
    // The original FireRed TM06 was actually consumed at step 1 of phase 2.
    if(w.phaseCount==3 && w.phases[2].history.stepCount==4 &&
       w.phases[2].history.steps[1].action==A::TeachTM) {
        p.sourceTmReceiptCount=1;
        p.sourceTmReceipts[0]={2,1,6,G::FireRed,2,92,881,true};
    }
    return p;
}
constexpr P::Input bulbaTransfer(N destination=N::Platinum) {
    X::Witness w{};w.phaseCount=1;w.entireClaimedHistoryAttested=true;
    w.phases[0].catalog=cat(G::Ruby,1,bulba);
    w.phases[0].history=history(G::Ruby,1,501,20,20,{33,45,73,22},{33,45,73,22});
    return transfer(w,destination);
}
constexpr P::Input originalHMDeleted() {
    X::Witness w{};w.phaseCount=1;w.entireClaimedHistoryAttested=true;
    w.phases[0].catalog=cat(G::Ruby,1,bulba);
    w.phases[0].history=history(G::Ruby,1,900,20,20,{15,33,45,73},{33,45,73,0});
    w.phases[0].history.stepCount=1;
    w.phases[0].history.steps[0]={G::Ruby,20,A::DeleteMove,0,0,0,true};
    return transfer(w);
}
constexpr P::Input trained() {
    auto p=bulbaTransfer();
    p.observedLevel=21;p.stepCount=4;
    p.steps[0]={N::Platinum,21,B::OfferDecline,230,0,0,0,true,true};
    p.steps[1]={N::Platinum,21,B::TeachTM,412,0,53,10001,true,true};
    p.steps[2]={N::Platinum,21,B::Tutor,235,1,0,10002,true,true};
    p.steps[3]={N::Platinum,21,B::Reminder,230,2,0,10003,true,true};
    p.observedMoves={412,235,230,22};return p;
}
constexpr P::Input doubleMachine(bool sameReceipt) {
    auto p=bulbaTransfer();
    p.observedLevel=21;p.stepCount=3;
    p.steps[0]={N::Platinum,21,B::OfferDecline,230,0,0,0,true,true};
    p.steps[1]={N::Platinum,21,B::TeachTM,412,0,53,111,true,true};
    p.steps[2]={N::Platinum,21,B::TeachTM,92,1,6,
                sameReceipt?111u:112u,true,true};
    p.observedMoves={412,92,73,22};return p;
}
constexpr P::Input nativeVisit(bool gainLevel) {
    auto p=bulbaTransfer();p.observedGame=N::HeartGold;
    p.observedLevel=gainLevel?21:20;
    p.stepCount=1;
    p.steps[0]={N::HeartGold,20,B::VisitGame,0,0,0,0,true,true};
    return p;
}
constexpr P::Input nativeHM() {
    auto p=bulbaTransfer();p.stepCount=1;
    p.steps[0]={N::Platinum,20,B::TeachHM,15,0,1,0,true,true};
    p.observedMoves={15,45,73,22};return p;
}
static_assert(P::evaluate(doubleMachine(false),retail).status==S::CandidateConsistent);
static_assert(P::evaluate(doubleMachine(true),retail).status==S::CandidateContradicted);
static_assert(P::evaluate(nativeVisit(false),retail).status==S::CandidateConsistent);
static_assert(P::evaluate(nativeVisit(true),retail).status==S::Unknown);
static_assert(P::evaluate(nativeHM(),retail).status==S::CandidateConsistent);
static_assert(P::evaluate(transfer(bulbasaurEvolutionTradeAndService()),retail).status==S::CandidateConsistent);
static_assert(P::evaluate(transfer(splitShedinjaSibling()),retail).status==S::CandidateConsistent);
static_assert(P::evaluate(trained(),retail).status==S::CandidateConsistent);
static_assert(P::evaluate(originalHMDeleted(),retail).status==S::CandidateConsistent);
static_assert([]{auto p=trained();p.steps[0].move=75;
    return P::evaluate(p,retail).status==S::CandidateContradicted;}());
static_assert([]{auto p=trained();p.steps[1].independentReceipt=0;
    return P::evaluate(p,retail).status==S::Unknown;}());
static_assert([]{auto p=transfer(bulbasaurEvolutionTradeAndService());p.sourceTmReceipts[0].game=G::Ruby;
    return P::evaluate(p,retail).status==S::Unknown;}());
static_assert([]{auto p=originalHMDeleted();p.gba.phases[0].history.stepCount=0;
    return P::evaluate(p,retail).status==S::CandidateContradicted;}());
}
int main() {
    std::uint64_t checks=0;
    const std::array<N,5> destinations{N::Diamond,N::Pearl,N::Platinum,N::HeartGold,N::SoulSilver};
    for(N dest:destinations) {
        auto direct=bulbaTransfer(dest);
        assert(P::evaluate(direct,retail).status==S::CandidateConsistent);++checks;
        auto multicartridge=transfer(bulbasaurEvolutionTradeAndService(),dest);
        assert(P::evaluate(multicartridge,retail).status==S::CandidateConsistent);++checks;
        auto sibling=transfer(splitShedinjaSibling(),dest);
        assert(P::evaluate(sibling,retail).status==S::CandidateConsistent);++checks;
    }
    for(unsigned i=0;i<1024;++i) {
        auto a=doubleMachine(false);
        assert(P::evaluate(a,retail).status==S::CandidateConsistent);++checks;
        a.steps[2].independentReceipt=a.steps[1].independentReceipt;
        assert(P::evaluate(a,retail).status==S::CandidateContradicted);++checks;
        assert(P::evaluate(nativeVisit(false),retail).status==S::CandidateConsistent);++checks;
        assert(P::evaluate(nativeVisit(true),retail).status==S::Unknown);++checks;
        assert(P::evaluate(nativeHM(),retail).status==S::CandidateConsistent);++checks;
    }
    for(unsigned i=0;i<4096;++i) {
        auto p=trained();
        switch(i%20) {
        case 0:p.witnessedOutboundSnapshot=false;break;
        case 1:p.witnessedImmediateArrivalSnapshot=false;break;
        case 2:p.witnessedWholeNativeActionHistory=false;break;
        case 3:p.formContinuityVerified=false;break;
        case 4:p.arrivalLevel=0;break;
        case 5:p.species=4;break;
        case 6:p.immediateNds={33,45,73,76};break;
        case 7:p.outboundGba={57,45,73,22};break;
        case 8:p.steps[0].move=75;break;
        case 9:p.steps[0].exactActionVerified=false;break;
        case 10:p.steps[1].number=6;break;
        case 11:p.steps[1].independentReceipt=0;break;
        case 12:p.steps[1].game=N::Diamond;break;
        case 13:p.steps[2].move=164;break;
        case 14:p.steps[2].methodSourceVerified=false;break;
        case 15:p.steps[3].move=175;break;
        case 16:p.steps[3].independentReceipt=0;break;
        case 17:p.observedGame=N::Pearl;break;
        case 18:p.observedMoves={33,45,73,22};break;
        default:p.steps[0].action=B::Unsupported;break;
        }
        const auto result=P::evaluate(p,retail);
        assert(result.status!=S::CandidateConsistent&&!result.globalInvalidAuthorized);++checks;
        auto g=transfer(bulbasaurEvolutionTradeAndService());
        switch((i/16)%16) {
        case 0:g.sourceTmReceiptCount=0;break;
        case 1:g.sourceTmReceipts[0].singleUseItemReceipt=0;break;
        case 2:g.sourceTmReceipts[0].game=G::Ruby;break;
        case 3:g.sourceTmReceipts[0].species=1;break;
        case 4:g.sourceTmReceipts[0].move=93;break;
        case 5:g.sourceTmReceipts[0].machineNumber=10;break;
        case 6:g.sourceTmReceipts[0].stepIndex=2;break;
        case 7:g.sourceTmReceipts[0].phaseIndex=1;break;
        case 8:g.sourceTmReceipts[0].originalSourceAndItemVerified=false;break;
        case 9:g.gba.phases[2].history.steps[1].originalActionReceiptVerified=false;break;
        case 10:g.gba.edges[1].toGame=G::Ruby;break;
        case 11:g.transferredCandidateId=1008;break;
        case 12:g.species=1;break;
        case 13:g.outboundGba={33,45,73,22};break;
        case 14:g.immediateNds={92,45,73,22};break;
        default:g.gba.phases[0].history.steps[0].move=92;break;
        }
        const auto other=P::evaluate(g,retail);
        assert(other.status!=S::CandidateConsistent&&!other.globalInvalidAuthorized);++checks;
    }
    for(unsigned i=0;i<1024;++i) {
        auto p=transfer(splitShedinjaSibling());
        if(i%8==0)p.transferredCandidateId=2077;
        if(i%8==1)p.gba.edges[1].toCandidate=2077;
        if(i%8==2)p.gba.edges[1].partyCapacityAndPokeballAttested=false;
        if(i%8==3)p.gba.phases[2].history.initialMoves={10,210,141,170};
        if(i%8==4)p.gba.edges[0].offerCount=2;
        if(i%8==5)p.gba.phases[2].history.steps[0].move=103;
        if(i%8==6)p.gba.edges[1].level=21;
        if(i%8==7)p.gba.edges[1].fromCandidate=2078;
        assert(P::evaluate(p,retail).status!=S::CandidateConsistent);++checks;
        auto early=bulbaTransfer();
        early.observedLevel=21; // cannot silently skip the original native offer
        assert(P::evaluate(early,retail).status==S::CandidateContradicted);++checks;
    }
    for(unsigned i=0;i<2048;++i) {
        auto p=trained();p.gba.phases[0].history.steps[0].move=99;
        p.steps[3].exactActionVerified=false;
        assert(P::evaluate(p,retail).status==S::Unknown);++checks;
        P::Alternatives a{};a.count=2;
        a.candidates[0]=p;a.candidates[1]=trained();
        assert(P::evaluate(a,retail)==S::CandidateConsistent);++checks;
        a.candidates[1]=p;
        assert(P::evaluate(a,retail)==S::Unknown);++checks;
    }
    assert(checks==15+1024*5+4096*2+1024*2+2048*3);
    std::cout<<"L3 Stage5 original multi-cartridge Pal Park candidate continuity: "
             <<checks<<" adversarial groups PASS\n";
}
