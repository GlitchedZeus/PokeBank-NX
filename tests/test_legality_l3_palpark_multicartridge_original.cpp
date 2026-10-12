// Full repository-only exact retail data gate. No guessed Gen IV title source:
// only original Platinum's explicitly pinned 12 species can teach afterward.
#include "Legality/Gen34PalParkMultiCartridgeOriginal.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace O=Legality::Gen34PalParkMultiCartridgeOriginal;
namespace P=O::P;
namespace X=P::X;
namespace H=P::H;
using G=X::Game;using N=P::Game;using S=P::Status;using A=H::Action;
constexpr std::array<G,5> gba{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen};
constexpr std::array<N,5> nds{N::Diamond,N::Pearl,N::Platinum,N::HeartGold,N::SoulSilver};
constexpr H::History source(G game,std::uint16_t species,std::uint32_t id,
                            std::uint8_t start,std::uint8_t end,
                            std::array<std::uint16_t,4> from,
                            std::array<std::uint16_t,4> to) {
    H::History h{};h.game=game;h.species=species;h.candidateId=id;
    h.initialLevel=start;h.observedLevel=end;
    h.initialMoves=from;h.observedMoves=to;
    h.initialSnapshotVerified=h.samePokemonAndGameVerified=h.allPlayerActionsVerified=true;
    return h;
}
constexpr X::Edge evolution(X::EdgeKind k,G from,G to,
                             std::uint16_t s1,std::uint16_t s2,
                             std::uint32_t id1,std::uint32_t id2,
                             std::uint8_t level) {
    X::Edge e{};e.kind=k;e.fromSpecies=s1;e.toSpecies=s2;
    e.fromGame=from;e.toGame=to;e.fromCandidate=id1;e.toCandidate=id2;
    e.level=level;e.originalTransitionAttested=true;
    e.samePokemonAttested=k!=X::EdgeKind::ShedinjaSibling;
    e.moveSlotsImmutableAcrossEdgeAttested=true;e.allImmediateOffersAttested=true;
    return e;
}
constexpr P::Input crossing(G game,N destination,std::uint16_t species=1) {
    P::Input p{};
    p.gba.phaseCount=1;p.gba.entireClaimedHistoryAttested=true;
    p.gba.phases[0].history=source(game,species,4011,20,20,
                                  {33,45,73,22},{33,45,73,22});
    p.transferredCandidateId=4011;p.species=species;
    p.arrivalGame=p.observedGame=destination;
    p.arrivalLevel=p.observedLevel=20;
    p.outboundGba=p.immediateNds=p.observedMoves={33,45,73,22};
    p.formContinuityVerified=true;p.witnessedSameIndividualAtTransfer=true;
    p.witnessedOutboundSnapshot=true;p.witnessedImmediateArrivalSnapshot=true;
    p.witnessedWholeNativeActionHistory=true;return p;
}
constexpr P::Input platinumMethods() {
    auto p=crossing(G::Ruby,N::Platinum);
    p.observedLevel=21;p.stepCount=4;
    p.steps[0]={N::Platinum,21,P::Action::OfferDecline,230,0,0,0,true,true};
    p.steps[1]={N::Platinum,21,P::Action::TeachTM,412,0,53,103,true,true};
    p.steps[2]={N::Platinum,21,P::Action::Tutor,235,1,0,104,true,true};
    p.steps[3]={N::Platinum,21,P::Action::Reminder,230,2,0,105,true,true};
    p.observedMoves={412,235,230,22};return p;
}
constexpr P::Input multicart() {
    auto p=crossing(G::Ruby,N::Platinum,2);
    p.gba.phaseCount=3;
    p.gba.phases[0].history=source(G::Ruby,1,4011,5,16,
                                    {33,45,0,0},{33,45,73,22});
    auto& s=p.gba.phases[0].history;
    s.stepCount=4;
    s.steps[0]={G::Ruby,7,A::LevelLearn,73,2,0,true};
    s.steps[1]={G::Ruby,10,A::LevelLearn,22,3,0,true};
    s.steps[2]={G::Ruby,15,A::LevelDecline,77,0,0,true};
    s.steps[3]={G::Ruby,15,A::LevelDecline,79,0,0,true};
    p.gba.phases[1].history=source(G::Ruby,2,4011,16,20,
                                    {33,45,73,22},{33,45,73,22});
    p.gba.edges[0]=evolution(X::EdgeKind::Evolve,G::Ruby,G::Ruby,1,2,4011,4011,16);
    p.gba.phases[2].history=source(G::FireRed,2,4011,20,25,
                                    {33,45,73,22},{92,45,22,0});
    auto& t=p.gba.phases[2].history;
    t.stepCount=4;
    t.steps[0]={G::FireRed,22,A::LevelLearn,75,1,0,true};
    t.steps[1]={G::FireRed,23,A::TeachTM,92,0,6,true};
    t.steps[2]={G::FireRed,24,A::Relearn,45,2,0,true};
    t.steps[3]={G::FireRed,25,A::DeleteMove,0,1,0,true};
    p.gba.edges[1]=evolution(X::EdgeKind::CartridgeVisit,G::Ruby,G::FireRed,2,2,4011,4011,20);
    p.sourceTmReceiptCount=1;
    p.sourceTmReceipts[0]={2,1,6,G::FireRed,2,92,881,true};
    p.arrivalLevel=p.observedLevel=25;
    p.outboundGba=p.immediateNds=p.observedMoves={92,45,22,0};
    return p;
}
constexpr P::Input shedinja() {
    auto p=crossing(G::Ruby,N::Platinum,292);
    p.gba.phaseCount=3;
    p.gba.phases[0].history=source(G::Ruby,290,2077,19,20,
                                    {10,106,141,170},{10,106,141,170});
    p.gba.phases[1].history=source(G::Ruby,291,2077,20,20,
                                    {104,210,141,170},{104,210,141,170});
    p.gba.edges[0]=evolution(X::EdgeKind::Evolve,G::Ruby,G::Ruby,290,291,2077,2077,20);
    p.gba.edges[0].offerCount=3;
    p.gba.edges[0].immediateOffers[0]={20,104,X::OfferChoice::Learn,0,true};
    p.gba.edges[0].immediateOffers[1]={20,210,X::OfferChoice::Learn,1,true};
    p.gba.edges[0].immediateOffers[2]={20,103,X::OfferChoice::Decline,0,true};
    p.gba.phases[2].history=source(G::Ruby,292,2078,20,25,
                                    {104,210,141,170},{104,210,180,170});
    p.gba.phases[2].history.stepCount=1;
    p.gba.phases[2].history.steps[0]={G::Ruby,25,A::LevelLearn,180,2,0,true};
    p.gba.edges[1]=evolution(X::EdgeKind::ShedinjaSibling,G::Ruby,G::Ruby,291,292,2077,2078,20);
    p.gba.edges[1].nincadaToNinjaskAlreadyAttested=true;
    p.gba.edges[1].partyCapacityAndPokeballAttested=true;
    p.gba.edges[1].copiedAfterNinjaskOffersAttested=true;
    p.transferredCandidateId=2078;p.arrivalLevel=p.observedLevel=25;
    p.outboundGba=p.immediateNds=p.observedMoves={104,210,180,170};
    return p;
}
static_assert(O::originalPlatinum(1).pinned);
static_assert(O::originalPlatinum(1).offers[8].level==21);
static_assert(O::originalPlatinum(1).offers[8].move==230);
static_assert(O::originalPlatinum(1).machines[5].move==412);
static_assert(!O::originalPlatinum(292).pinned);
int main() {
    std::uint64_t checks=0,sourceOffers=0,compat=0;
    for(G original:gba)for(std::uint16_t sp=1;sp<=386;++sp) {
        const auto evidence=Legality::Gen3GbaJointMethodHistory::originalEvidence(original,sp);
        assert(evidence.pinned);++checks;
        sourceOffers+=evidence.count;
        for(auto g:nds) {
            auto p=crossing(original,g,sp);
            assert(O::evaluate(p).status==S::CandidateConsistent);
            ++checks;
        }
        for(std::uint8_t m=0;m<58;++m)
            compat+=(evidence.machines>>m)&1u;
    }
    assert(sourceOffers==19865);
    assert(compat==44635);
    for(auto original:gba)for(auto destination:nds) {
        auto p=crossing(original,destination,1);
        assert(O::evaluate(p).status==S::CandidateConsistent);++checks;
    }
    assert(O::evaluate(platinumMethods()).status==S::CandidateConsistent);++checks;
    assert(O::evaluate(multicart()).status==S::CandidateConsistent);++checks;
    assert(O::evaluate(shedinja()).status==S::CandidateConsistent);++checks;
    auto wrong=multicart();wrong.sourceTmReceipts[0].game=G::Ruby;
    assert(O::evaluate(wrong).status==S::Unknown);++checks;
    wrong=multicart();wrong.gba.edges[1].toGame=G::Ruby;
    assert(O::evaluate(wrong).status!=S::CandidateConsistent);++checks;
    auto p=platinumMethods();p.steps[0].move=75;
    assert(O::evaluate(p).status==S::CandidateContradicted);++checks;
    p=platinumMethods();p.steps[1].number=20;
    assert(O::evaluate(p).status==S::Unknown);++checks; // TM20 not independently pinned
    p=platinumMethods();p.steps[3].exactActionVerified=false;
    assert(O::evaluate(p).status==S::Unknown);++checks;
    p=shedinja();p.transferredCandidateId=2077;
    assert(O::evaluate(p).status==S::Unknown);++checks;
    std::cout<<"L3 Stage5 source-original 1930 GBA species rows + 111940 machine checks + 25 destinations: "
             <<checks<<" candidate snapshot checks PASS\n";
}
