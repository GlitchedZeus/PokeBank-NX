#pragma once
// EVENT-001/MOVE-001: one *hypothetical known* Gen IV event card supplies
// retained initial moves. A previously matched event template does not
// contain trustworthy initial moves in the current generated data: caller
// must independently supply and attest to that evidence. Neither a full
// PCD payload nor proof of real distribution receipt survives in PK4.
// Candidate failure NEVER means global Invalid.
#include "Legality/Gen4EventTemplate.h"
#include "Legality/Gen14CandidateMoveHistory.h"
#include <array>
#include <cstdint>
namespace Legality::Gen4EventJointMoveHistory {
namespace M=Gen14CandidateMoveHistory;
enum class Status : uint8_t {
    IdentityMismatch, UnknownInitialMoves, AmbiguousCard,
    CompatibleCandidate, Unresolved, CandidateContradicted
};
struct Input {
    Gen4EventTemplate::Candidate stored{};
    uint16_t claimedCardId=0;
    bool initialMovesIndependentlyVerified=false;
    std::array<uint16_t,4> initialMoves{};
    std::array<uint16_t,4> observedMoves{};
    std::array<M::Slot,4> laterRoutes{};
    M::Candidate history{};
};
struct Result {
    Status status=Status::Unresolved;
    uint8_t matchedCards=0;
    uint8_t retainedInitialMoves=0;
    M::Result joint{};
};
constexpr bool contains(const std::array<uint16_t,4>& moves,uint16_t target) noexcept {
    if(!target)return false;
    for(auto move:moves)if(move==target)return true;
    return false;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    // Zero card IDs are insufficient to assert *one* event identity.
    if(!in.claimedCardId || !in.history.id)
        {out.status=Status::IdentityMismatch;return out;}
    for(const auto& t:Gen4EventTemplate::kGen4EventTemplates) {
        if(t.cardId==in.claimedCardId &&
           (Gen4EventTemplate::matches(t,in.stored) ||
            Gen4EventTemplate::matchesEvolved(t,in.stored)))
            ++out.matchedCards;
    }
    if(!out.matchedCards)
        {out.status=Status::IdentityMismatch;return out;}
    if(out.matchedCards!=1)
        {out.status=Status::AmbiguousCard;return out;}
    if(!in.initialMovesIndependentlyVerified)
        {out.status=Status::UnknownInitialMoves;return out;}
    std::array<M::Slot,4> slots{};
    for(uint8_t i=0;i<4;++i) {
        const uint16_t observed=in.observedMoves[i];
        if(!observed)continue;
        auto& slot=slots[i];
        slot.move=observed;
        slot.exhaustive=in.history.allRelevantRoutesEnumerated;
        if(contains(in.initialMoves,observed)) {
            slot.options[slot.count++]={observed,in.history.id,
                M::Method::Event,1,100,0,in.claimedCardId,0,true};
            ++out.retainedInitialMoves;
        }
        const auto& later=in.laterRoutes[i];
        if(later.move!=observed && later.count)
            slot.exhaustive=false;
        else for(uint8_t k=0;k<later.count && k<later.options.size();++k) {
            const auto& w=later.options[k];
            // A separate event card cannot be spliced into the claimed
            // single event distribution. Other independently verified
            // later methods (level-up, machine, tutor etc.) may coexist.
            if(w.method==M::Method::Event && w.eventId!=in.claimedCardId)
                continue;
            if(!w.methodVerified)slot.exhaustive=false;
            if(slot.count<slot.options.size())slot.options[slot.count++]=w;
            else slot.exhaustive=false; // capacity exhausted != impossible
        }
        if(later.count>later.options.size())
            slot.exhaustive=false;
    }
    out.joint=M::evaluate(in.history,slots);
    switch(out.joint.status) {
        case M::Status::WitnessesCompatible:
            out.status=Status::CompatibleCandidate;break;
        case M::Status::CandidateContradicted:
            out.status=Status::CandidateContradicted;break;
        default: out.status=Status::Unresolved;break;
    }
    return out;
}
} // namespace Legality::Gen4EventJointMoveHistory
