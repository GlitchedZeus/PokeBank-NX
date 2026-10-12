#pragma once
// BREED-003 / RNG-003: one coherent, EXTERNALLY ATTESTED ordinary Gen IV
// egg parent snapshot supplies retail move inheritance and IV stage replay.
// Exact native PK4s are immutable inputs. This excludes only the stated
// candidate history and never implies a global Invalid Pokémon.
#include "Legality/Gen4NativeBreedingParentEvidence.h"
#include "Legality/Gen4EggIVInheritanceStage.h"
#include "Legality/Gen4EggIVStructuralBounds.h"
#include "Legality/Gen34SameParentEggHistory.h"
#include <array>
#include <cstdint>

namespace Legality::Gen4JointEggCandidateEvidence {
namespace Native=Gen4NativeBreedingParentEvidence;
namespace Pair=Gen34BreedingPairEligibility;
namespace Move=Gen34SameParentEggHistory;
namespace IV=Gen4EggIVInheritanceStage;
namespace Bounds=Gen4EggIVStructuralBounds;
enum class Status : uint8_t {
    UnsupportedGame, UnknownNativeParentEvidence, UnknownNativeEggRecord,
    IncoherentNativeEggRecord, UnknownPairEligibility,
    CandidatePairIncompatible, UnknownParentMoves, IncoherentParentIdentity,
    IncoherentMoveSnapshot, UnknownSharedStage, IncoherentSharedStage,
    UnknownInitialChildIVs, UnknownObservedChildIVs,
    UnknownPowerItemHistory, UnknownFinalRNGState, MalformedIVDomain,
    MoveCandidateContradicted, IVNecessaryPatternContradicted,
    IVCandidateContradicted, KnownCandidateConsistent
};
struct Input {
    Native::Input parents{};
    Move::Input moveHistory{};
    // All three attested time tokens must refer to the SAME historical
    // pair state, not merely two PK4 records with matching parent IDs.
    bool sharedStageIndependentlyVerified=false;
    uint64_t nativePairStageToken=0,moveParentStageToken=0,ivParentStageToken=0;
    bool initialChildIVsIndependentlyVerified=false;
    bool observedChildIVsIndependentlyVerified=false;
    std::array<uint8_t,6> initialChildIVs{},observedChildIVs{};
    // Source-attested held items at the IV stage; present PK4 held items
    // do not by themselves establish historical collection-time holders.
    bool heldItemsAtStageIndependentlyVerified=false;
    bool finalRNGStateIndependentlyVerified=false;
    uint32_t knownFinalRNGState=0;
};
struct Result {
    Status status=Status::UnknownNativeParentEvidence;
    Native::Result parents{};
    Move::Result moves{};
    IV::CandidateCheck ivs=IV::CandidateCheck::UnknownParents;
    Bounds::Result necessaryIVs{};
};
inline Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto game=in.parents.game;
    if(IV::family(game)==IV::Family::Unknown) {
        out.status=Status::UnsupportedGame;return out;
    }
    out.parents=Native::evaluate(in.parents);
    if(out.parents.status!=Native::Status::Projected ||
       !out.parents.pairEvaluated) {
        out.status=Status::UnknownNativeParentEvidence;return out;
    }
    const auto& pair=out.parents.eligibility;
    if(pair.status==Pair::Status::Incompatible) {
        out.status=Status::CandidatePairIncompatible;return out;
    }
    if(pair.status!=Pair::Status::Compatible || !pair.donorRolesKnown) {
        out.status=Status::UnknownPairEligibility;return out;
    }
    const auto& m=in.moveHistory;
    if(!m.snapshotsKnown || !m.fatherId || !m.motherId ||
       !m.fatherSpecies || !m.motherSpecies) {
        out.status=Status::UnknownParentMoves;return out;
    }
    if(m.game!=game || m.fatherId!=pair.fatherId ||
       m.motherId!=pair.motherId) {
        out.status=Status::IncoherentParentIdentity;return out;
    }
    const auto& slot0=*in.parents.slot0.native;
    const auto& slot1=*in.parents.slot1.native;
    const bool fatherInSlot0=in.parents.slot0.snapshotId==pair.fatherId;
    const bool motherInSlot0=in.parents.slot0.snapshotId==pair.motherId;
    if(fatherInSlot0==motherInSlot0) {
        out.status=Status::IncoherentParentIdentity;return out;
    }
    const auto& father=fatherInSlot0?slot0:slot1;
    const auto& mother=fatherInSlot0?slot1:slot0;
    if(m.fatherSpecies!=father.species() ||
       m.motherSpecies!=mother.species()) {
        out.status=Status::IncoherentParentIdentity;return out;
    }
    if(m.father!=father.moves() || m.mother!=mother.moves()) {
        out.status=Status::IncoherentMoveSnapshot;return out;
    }
    if(!in.sharedStageIndependentlyVerified ||
       !in.nativePairStageToken || !in.moveParentStageToken ||
       !in.ivParentStageToken) {
        out.status=Status::UnknownSharedStage;return out;
    }
    if(in.nativePairStageToken!=in.moveParentStageToken ||
       in.nativePairStageToken!=in.ivParentStageToken) {
        out.status=Status::IncoherentSharedStage;return out;
    }
    // Only the source-verified pair may authorize the four-slot move proof.
    Move::Input exact=m;
    exact.pairProof=Move::PairProof::IndependentlyCompatible;
    out.moves=Move::evaluate(exact);
    switch(out.moves.status) {
        case Move::Status::Unsupported:
            out.status=Status::UnsupportedGame;return out;
        case Move::Status::UnknownParents:
            out.status=Status::UnknownParentMoves;return out;
        case Move::Status::UnknownPairEligibility:
            out.status=Status::UnknownPairEligibility;return out;
        case Move::Status::UnknownLightBall:
            out.status=Status::UnknownParentMoves;return out;
        case Move::Status::CandidateMismatch:
            out.status=Status::MoveCandidateContradicted;return out;
        case Move::Status::CandidateCompatible:break;
    }
    if(!in.observedChildIVsIndependentlyVerified) {
        out.status=Status::UnknownObservedChildIVs;return out;
    }
    // Necessary inheritance constraints require historical source-linked
    // native parent IVs + original child IVs, but NOT a known initial
    // random-IV roll or retained RNG seed. Distinct HGSS inherited stats
    // and independently sourced Power-item donors can exclude a candidate.
    // Passing is only NOT EXCLUDED, never proof of an actual RNG history.
    Bounds::Input bound{};
    bound.game=game;
    bound.physicalSlot0=slot0.ivs();
    bound.physicalSlot1=slot1.ivs();
    bound.observedChild=in.observedChildIVs;
    bound.heldItem0=slot0.heldItem();
    bound.heldItem1=slot1.heldItem();
    bound.historicalParentIVsIndependentlyVerified=true;
    bound.historicalPhysicalOrderIndependentlyVerified=true;
    bound.originalChildIVsIndependentlyVerified=true;
    bound.heldItemsAtIVStageIndependentlyVerified=
        in.heldItemsAtStageIndependentlyVerified;
    out.necessaryIVs=Bounds::evaluate(bound);
    switch(out.necessaryIVs.status) {
        case Bounds::Status::CandidateContradiction:
            out.status=Status::IVNecessaryPatternContradicted;return out;
        case Bounds::Status::MalformedIVDomain:
            out.status=Status::MalformedIVDomain;return out;
        case Bounds::Status::UnknownHeldItemHistory:
            out.status=Status::UnknownPowerItemHistory;return out;
        case Bounds::Status::NecessaryPatternNotExcluded:break;
        case Bounds::Status::UnsupportedGame:
            out.status=Status::UnsupportedGame;return out;
        case Bounds::Status::UnknownOriginalChildIVs:
            out.status=Status::UnknownObservedChildIVs;return out;
        case Bounds::Status::UnknownPhysicalSlots:
        case Bounds::Status::UnknownHistoricalParents:
            out.status=Status::UnknownNativeParentEvidence;return out;
    }
    if(!in.initialChildIVsIndependentlyVerified) {
        out.status=Status::UnknownInitialChildIVs;return out;
    }
    if(!in.finalRNGStateIndependentlyVerified) {
        out.status=Status::UnknownFinalRNGState;return out;
    }
    IV::KnownFinalStage stage{};
    stage.game=game;
    stage.initialChild=in.initialChildIVs;
    stage.observedChild=in.observedChildIVs;
    stage.physicalSlot0=slot0.ivs();
    stage.physicalSlot1=slot1.ivs();
    stage.heldItem0=slot0.heldItem();
    stage.heldItem1=slot1.heldItem();
    stage.finalSeed=in.knownFinalRNGState;
    stage.parentSnapshotsIndependentlyVerified=true;
    stage.depositOrderIndependentlyVerified=true;
    stage.initialChildIVsIndependentlyVerified=true;
    stage.heldItemHistoryIndependentlyVerified=
        in.heldItemsAtStageIndependentlyVerified;
    stage.finalRNGStateIndependentlyVerified=true;
    out.ivs=IV::verifyKnownFinalStage(stage);
    switch(out.ivs) {
        case IV::CandidateCheck::Matched:
            out.status=Status::KnownCandidateConsistent;break;
        case IV::CandidateCheck::CandidateMismatch:
            out.status=Status::IVCandidateContradicted;break;
        case IV::CandidateCheck::MalformedIVDomain:
            out.status=Status::MalformedIVDomain;break;
        case IV::CandidateCheck::UnsupportedGame:
            out.status=Status::UnsupportedGame;break;
        case IV::CandidateCheck::UnknownInitialChildIVs:
            out.status=Status::UnknownInitialChildIVs;break;
        case IV::CandidateCheck::UnknownHeldItemHistory:
            out.status=Status::UnknownPowerItemHistory;break;
        case IV::CandidateCheck::UnknownRNGStageBoundary:
            out.status=Status::UnknownFinalRNGState;break;
        case IV::CandidateCheck::UnknownPhysicalSlots:
        case IV::CandidateCheck::UnknownParents:
            out.status=Status::UnknownNativeParentEvidence;break;
    }
    return out;
}
// Bind the observed child IVs, four moves and egg species to ONE valid,
// immutable native encrypted PK4 at the independently attested original
// UNHATCHED stage. A present-day hatched/evolved record cannot be used as
// proof of its unhatched moves or a historical collection-time state.
struct NativeEggWitness {
    const Pokemon::Pokemon4ReadOnly* record=nullptr;
    bool originalUnhatchedStageIndependentlyVerified=false;
    bool containingGameIndependentlyVerified=false;
    // This unhatched child must belong to the SAME source-attested
    // daycare event as the verified parent/move/IV stage.
    uint64_t originalEggStageToken=0;
};
inline Result evaluateWithNativeEgg(const Input& in,
                                     const NativeEggWitness& witness) noexcept {
    Result out{};
    if(IV::family(in.parents.game)==IV::Family::Unknown) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!witness.record || !witness.originalUnhatchedStageIndependentlyVerified ||
       !witness.containingGameIndependentlyVerified) {
        out.status=Status::UnknownNativeEggRecord;return out;
    }
    if(!witness.originalEggStageToken || !in.nativePairStageToken) {
        out.status=Status::UnknownNativeEggRecord;return out;
    }
    if(witness.originalEggStageToken!=in.nativePairStageToken) {
        out.status=Status::IncoherentNativeEggRecord;return out;
    }
    const auto& egg=*witness.record;
    if(!egg.valid() || egg.empty() || !egg.isEgg() ||
       egg.sourceGroup()!=Native::requiredGroup(in.parents.game)) {
        out.status=Status::UnknownNativeEggRecord;return out;
    }
    if(!in.observedChildIVsIndependentlyVerified) {
        out.status=Status::UnknownObservedChildIVs;return out;
    }
    if(egg.species()!=in.moveHistory.species ||
       egg.moves()!=in.moveHistory.observed ||
       egg.ivs()!=in.observedChildIVs) {
        out.status=Status::IncoherentNativeEggRecord;return out;
    }
    return evaluate(in);
}
} // namespace Legality::Gen4JointEggCandidateEvidence
