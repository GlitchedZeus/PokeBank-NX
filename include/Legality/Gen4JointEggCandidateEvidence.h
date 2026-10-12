#pragma once
// Conditional ordinary Gen IV egg history. Moves, IVs and held items
// refer to one independently attested collection-stage parent pair.
// Neither a match nor a mismatch authenticates or rejects the child globally.
#include "Legality/Gen34SameParentEggHistory.h"
#include "Legality/Gen4EggIVInheritanceStage.h"

namespace Legality::Gen4JointEggCandidateEvidence {
namespace Move=Gen34SameParentEggHistory;
namespace IV=Gen4EggIVInheritanceStage;
enum class Status : uint8_t {
    UnsupportedGame, UnknownParentSnapshots, IncoherentParents,
    UnknownDaycareOrder, UnknownSharedStage, IncoherentStage,
    MalformedMoveInput, MoveHistoryUnresolved, MoveCandidateContradicted,
    IVHistoryUnresolved, MalformedIVInput, IVCandidateContradicted,
    KnownCandidateConsistent
};
struct Parent {
    uint32_t id=0;
    bool ivsKnown=false;
    IV::IVs ivs{};
    bool heldItemKnown=false;
    uint16_t heldItem=0;
};
struct Input {
    Move::Input moves{};
    // Ordered physical slots. IDs must cover exactly the move parent pair.
    std::array<Parent,2> daycareParents{};
    bool daycareOrderVerified=false;
    // Attestation covers parent moves, IVs, items AND deposit order at
    // the same stage. Tokens are external evidence, never child PK4 fields.
    bool sharedCollectionStageVerified=false;
    uint64_t moveStageToken=0,ivStageToken=0;
    bool initialIVsKnown=false;
    IV::IVs initialIVs{},observedIVs{};
    bool finalRNGStateKnown=false;
    uint32_t finalRNGState=0;
};
struct Result {
    Status status=Status::UnknownParentSnapshots;
    Move::Result moves{};
    IV::CandidateCheck ivs=IV::CandidateCheck::UnknownParentSlots;
};
constexpr bool structuredMoves(const std::array<uint16_t,4>& moves) noexcept {
    bool empty=false;
    for(unsigned i=0;i<4;++i) {
        if(!moves[i]){empty=true;continue;}
        if(empty||moves[i]>467)return false;
        for(unsigned j=0;j<i;++j)if(moves[j]==moves[i])return false;
    }
    return true;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(IV::family(in.moves.game)==IV::Family::Unsupported) {
        out.status=Status::UnsupportedGame;return out;
    }
    const auto& a=in.daycareParents[0];const auto& b=in.daycareParents[1];
    if(!in.moves.snapshotsKnown||!in.moves.fatherId||!in.moves.motherId||!a.id||!b.id)
        return out;
    const bool samePair=(a.id==in.moves.fatherId&&b.id==in.moves.motherId)||
                        (a.id==in.moves.motherId&&b.id==in.moves.fatherId);
    if(a.id==b.id||in.moves.fatherId==in.moves.motherId||!samePair) {
        out.status=Status::IncoherentParents;return out;
    }
    if(!in.daycareOrderVerified){out.status=Status::UnknownDaycareOrder;return out;}
    if(!in.sharedCollectionStageVerified||!in.moveStageToken||!in.ivStageToken) {
        out.status=Status::UnknownSharedStage;return out;
    }
    if(in.moveStageToken!=in.ivStageToken){out.status=Status::IncoherentStage;return out;}
    if(!structuredMoves(in.moves.base)||!structuredMoves(in.moves.father)||
       !structuredMoves(in.moves.mother)||!structuredMoves(in.moves.observed)||
       (in.moves.pairProof!=Move::PairProof::Unknown&&
        in.moves.pairProof!=Move::PairProof::IndependentlyCompatible&&
        in.moves.pairProof!=Move::PairProof::IndependentlyIncompatible)) {
        out.status=Status::MalformedMoveInput;return out;
    }
    auto moves=in.moves;
    // Light Ball must come from these SAME parent item snapshots. Do not
    // accept a separate caller-provided item hypothesis for the move pass.
    if(moves.species==172) {
        moves.lightBallKnown=a.heldItemKnown&&b.heldItemKnown;
        moves.lightBall=moves.lightBallKnown&&(a.heldItem==236||b.heldItem==236);
    }
    out.moves=Move::evaluate(moves);
    if(out.moves.status==Move::Status::CandidateMismatch) {
        out.status=Status::MoveCandidateContradicted;return out;
    }
    if(out.moves.status!=Move::Status::CandidateCompatible) {
        out.status=Status::MoveHistoryUnresolved;return out;
    }
    IV::Input stage{};stage.game=moves.game;
    stage.parentSlotsKnown=a.ivsKnown&&b.ivsKnown;
    stage.parents={a.ivs,b.ivs};
    stage.initialIVsKnown=in.initialIVsKnown;stage.initialIVs=in.initialIVs;
    stage.heldItemsKnown=a.heldItemKnown&&b.heldItemKnown;
    stage.heldItems={a.heldItem,b.heldItem};
    stage.rngStateKnown=in.finalRNGStateKnown;stage.rngState=in.finalRNGState;
    out.ivs=IV::verifyFinalState(stage,in.observedIVs);
    switch(out.ivs) {
        case IV::CandidateCheck::Matched:out.status=Status::KnownCandidateConsistent;break;
        case IV::CandidateCheck::CandidateMismatch:out.status=Status::IVCandidateContradicted;break;
        case IV::CandidateCheck::MalformedIVDomain:out.status=Status::MalformedIVInput;break;
        default:out.status=Status::IVHistoryUnresolved;break;
    }
    return out;
}
} // namespace Legality::Gen4JointEggCandidateEvidence
