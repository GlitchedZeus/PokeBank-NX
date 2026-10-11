#include "Legality/Gen4EventJointMoveHistory.h"
#include <cassert>
#include <iostream>
namespace J=Legality::Gen4EventJointMoveHistory;
namespace M=Legality::Gen14CandidateMoveHistory;
// A pinned native Gen IV Charmander fixed-PID template at card 67.
// Initial moves are EXPLICITLY synthetic conditional witnesses; the
// current generated PCD projection does not contain raw initial moves.
constexpr auto source=Legality::Gen4EventTemplate::Candidate{
    4,7200,58364,0x0f199b50u,40,3053,16,0,0,1,7,0,true
};
constexpr M::Witness laterFlamethrower{
    53,1001,M::Method::Tutor,40,60,2,0,0,true
};
constexpr auto base=[]{
    J::Input in{};
    in.stored=source;in.claimedCardId=67;
    in.initialMovesIndependentlyVerified=true;
    in.initialMoves={10,52,0,0};
    in.observedMoves={10,52,53,0};
    in.history={1001,60,40,true,5};
    in.laterRoutes[2].move=53;
    in.laterRoutes[2].count=1;
    in.laterRoutes[2].options[0]=laterFlamethrower;
    in.laterRoutes[2].exhaustive=true;
    return in;
}();
static_assert(J::evaluate(base).status==J::Status::CompatibleCandidate);
static_assert(J::evaluate(base).retainedInitialMoves==2);
static_assert(J::evaluate(base).joint.coveredSlots==3);
constexpr auto missingInitial=[] {
    auto in=base;in.initialMovesIndependentlyVerified=false;return J::evaluate(in);
}();
static_assert(missingInitial.status==J::Status::UnknownInitialMoves);
constexpr auto mismatchedCard=[] {
    auto in=base;in.claimedCardId=68;return J::evaluate(in);
}();
static_assert(mismatchedCard.status==J::Status::IdentityMismatch);
constexpr auto swappedCardEvent=[] {
    auto in=base;
    in.laterRoutes[2].options[0]={
        53,1001,M::Method::Event,40,60,2,68,0,true};
    return J::evaluate(in);
}();
static_assert(swappedCardEvent.status==J::Status::CandidateContradicted);
constexpr auto unknownLater=[] {
    auto in=base;in.history.allRelevantRoutesEnumerated=false;
    in.laterRoutes[2].options[0]={
        53,1001,M::Method::BroadPool,40,60,2,0,0,false};
    return J::evaluate(in);
}();
static_assert(unknownLater.status==J::Status::Unresolved);
constexpr auto wrongIdentity=[] {
    auto in=base;in.stored.pid^=1u;return J::evaluate(in);
}();
static_assert(wrongIdentity.status==J::Status::IdentityMismatch);
constexpr auto evolved=[] {
    auto in=base;in.stored.species=5;return J::evaluate(in);
}();
static_assert(evolved.status==J::Status::CompatibleCandidate);
int main(){
    assert(J::evaluate(base).status==J::Status::CompatibleCandidate);
    assert(swappedCardEvent.status==J::Status::CandidateContradicted);
    std::cout<<"Gen IV conditional single-event four-slot witness: PASS\n";
}
