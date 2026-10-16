// Stage 4 native source-joined regressions. A full exact-head repository
// checkout is required; never claim this test passed from header-only shims.
#include "Legality/Gen3GbaExceptionalHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace X=Legality::Gen3GbaExceptionalHistoryCore;
namespace E=Legality::Gen3GbaExceptionalHistory;
namespace H=X::H;
using G=X::Game;using A=H::Action;using S=X::Status;
constexpr std::array<G,5> kGames{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen};
constexpr H::History h(G g,std::uint16_t sp,std::uint32_t id,std::uint8_t low,
                       std::uint8_t high,std::array<std::uint16_t,4> a,
                       std::array<std::uint16_t,4> b) {
    H::History r{};r.game=g;r.species=sp;r.candidateId=id;
    r.initialLevel=low;r.observedLevel=high;r.initialMoves=a;r.observedMoves=b;
    r.initialSnapshotVerified=r.samePokemonAndGameVerified=r.allPlayerActionsVerified=true;
    return r;
}
constexpr X::Edge e(X::EdgeKind kind,G from,G to,std::uint16_t a,std::uint16_t b,
                    std::uint8_t level,std::uint32_t fromId,std::uint32_t toId) {
    X::Edge r{};r.kind=kind;r.fromGame=from;r.toGame=to;
    r.fromSpecies=a;r.toSpecies=b;r.level=level;
    r.fromCandidate=fromId;r.toCandidate=toId;
    r.originalTransitionAttested=r.moveSlotsImmutableAcrossEdgeAttested=true;
    r.samePokemonAttested=kind!=X::EdgeKind::ShedinjaSibling;
    r.allImmediateOffersAttested=true;
    return r;
}
constexpr E::OriginalWitness starter(G game) {
    E::OriginalWitness w{};w.phaseCount=2;w.entireClaimedHistoryAttested=true;
    w.phases[0]=h(game,1,997,5,16,{33,45,0,0},{33,45,73,22});
    w.phases[0].stepCount=4;
    w.phases[0].steps[0]={game,7,A::LevelLearn,73,2,0,true};
    w.phases[0].steps[1]={game,10,A::LevelLearn,22,3,0,true};
    w.phases[0].steps[2]={game,15,A::LevelDecline,77,0,0,true};
    w.phases[0].steps[3]={game,15,A::LevelDecline,79,0,0,true};
    w.phases[1]=h(game,2,997,16,20,{33,45,73,22},{33,45,73,22});
    w.edges[0]=e(X::EdgeKind::Evolve,game,game,1,2,16,997,997);
    return w;
}
constexpr E::OriginalWitness shedinja(G game) {
    E::OriginalWitness w{};w.phaseCount=3;w.entireClaimedHistoryAttested=true;
    w.phases[0]=h(game,290,1009,19,20,{10,106,141,170},{10,106,141,170});
    w.phases[1]=h(game,291,1009,20,20,{104,210,141,170},{104,210,141,170});
    w.edges[0]=e(X::EdgeKind::Evolve,game,game,290,291,20,1009,1009);
    w.edges[0].offerCount=3;
    w.edges[0].immediateOffers[0]={20,104,X::OfferChoice::Learn,0,true};
    w.edges[0].immediateOffers[1]={20,210,X::OfferChoice::Learn,1,true};
    w.edges[0].immediateOffers[2]={20,103,X::OfferChoice::Decline,0,true};
    w.phases[2]=h(game,292,1010,20,25,{104,210,141,170},{104,210,180,170});
    w.phases[2].stepCount=1;
    w.phases[2].steps[0]={game,25,A::LevelLearn,180,2,0,true};
    w.edges[1]=e(X::EdgeKind::ShedinjaSibling,game,game,291,292,20,1009,1010);
    w.edges[1].nincadaToNinjaskAlreadyAttested=true;
    w.edges[1].partyCapacityAndPokeballAttested=true;
    w.edges[1].copiedAfterNinjaskOffersAttested=true;
    return w;
}
// Source-pinned method possibilities never authenticate user histories.
int main() {
    std::uint64_t checks=0;
    for(const auto game:kGames) {
        const auto a=starter(game),b=shedinja(game);
        const auto p=E::evaluate(a),q=E::evaluate(b);
        assert(p.status==S::CandidateConsistent);
        assert(q.status==S::CandidateConsistent);
        assert(p.evolutions==1&&p.phases==2);
        assert(q.evolutions==1&&q.siblings==1&&q.immediateOffers==3);
        checks+=4;
        auto malformed=a;
        malformed.edges[0].fromSpecies=94;
        assert(E::evaluate(malformed).status==S::Unknown);++checks;
        malformed=a;malformed.phases[0].steps[0].move=92;
        assert(E::evaluate(malformed).status==S::CandidateContradicted);++checks;
        malformed=b;malformed.edges[1].partyCapacityAndPokeballAttested=false;
        assert(E::evaluate(malformed).status==S::Unknown);++checks;
        malformed=b;malformed.phases[2].initialMoves[0]=10;
        assert(E::evaluate(malformed).status==S::CandidateContradicted);++checks;
    }
    // Every original version/species source used by Stage 4 is separately
    // checked against the originally pinned complete per-species hash.
    for(const G game:kGames)for(std::uint16_t sp=1;sp<=386;++sp) {
        const auto original=Legality::Gen3GbaJointMethodHistory::originalEvidence(game,sp);
        assert(original.pinned);
        assert(original.count>=1 && original.count<=64);
        ++checks;
    }
    assert(checks==1970);
    std::cout<<"L3 Stage4 native original cross-stage fingerprints: "<<checks
             <<" exact-source assertions PASS\n";
}
