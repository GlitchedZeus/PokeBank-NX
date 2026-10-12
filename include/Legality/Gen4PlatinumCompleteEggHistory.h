#pragma once
// BREED-005: one coherent original Platinum ordinary daycare egg history.
// Link SAME immutable original parent pair + unhatched child across
// move history, retail IV-stage, and bounded original MT/Everstone PID
// history. Never splice three independently compatible but unrelated
// candidates into one invented legal egg; never issue global Invalid.
#include "Legality/Gen4PlatinumEggPidStage.h"
#include "Legality/Gen4JointEggCandidateEvidence.h"
#include <algorithm>

namespace Legality::Gen4PlatinumCompleteEggHistory {
namespace Joint=Gen4JointEggCandidateEvidence;
namespace Pid=Gen4PlatinumEggPidStage;
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalWitness,
    IncoherentOriginalParent, IncoherentOriginalEgg,
    UnknownSharedStage, IncoherentSharedStage,
    UnknownMoveIvHistory, ContradictedMoveIvHistory,
    UnknownPidHistory, ContradictedPidHistory,
    KnownConditionalCombinedHistory
};
struct Input {
    Joint::Input moveAndIV{};
    Joint::NativeEggWitness originalEgg{};
    Pid::Witness originalPid{};
    // Independent attestation that the parent's original PID creation
    // and the observed child IV/move history describe ONE daycare event.
    bool allStagesSameOriginalDaycareIndependentlyVerified=false;
};
struct Result {
    Status status=Status::UnknownOriginalWitness;
    Joint::Result joint{};
    Pid::Result pid{};
};
inline bool sameOriginal(const Pokemon::Pokemon4ReadOnly* a,
                         const Pokemon::Pokemon4ReadOnly* b) noexcept {
    if(!a || !b || !a->valid() || !b->valid())return false;
    const auto ab=a->originalEncryptedBytes(),bb=b->originalEncryptedBytes();
    return ab.size()==bb.size() &&
        std::equal(ab.begin(),ab.end(),bb.begin());
}
inline Result evaluate(const Input& in) noexcept {
    Result out{};
    if(in.moveAndIV.parents.game!="platinum_nds" ||
       in.originalPid.game!="platinum_nds") {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.allStagesSameOriginalDaycareIndependentlyVerified) return out;
    if(!sameOriginal(in.moveAndIV.parents.slot0.native,
                     in.originalPid.physicalSlot0) ||
       !sameOriginal(in.moveAndIV.parents.slot1.native,
                     in.originalPid.physicalSlot1)) {
        out.status=Status::IncoherentOriginalParent;return out;
    }
    if(!sameOriginal(in.originalEgg.record,
                     in.originalPid.originalUnhatchedEgg)) {
        out.status=Status::IncoherentOriginalEgg;return out;
    }
    const auto common=in.moveAndIV.nativePairStageToken;
    if(!common || !in.originalEgg.originalEggStageToken ||
       !in.originalPid.parentPairToken || !in.originalPid.mtCreationToken ||
       !in.originalPid.eggCreationToken) {
        out.status=Status::UnknownSharedStage;return out;
    }
    if(common!=in.originalEgg.originalEggStageToken ||
       common!=in.originalPid.parentPairToken ||
       common!=in.originalPid.mtCreationToken ||
       common!=in.originalPid.eggCreationToken) {
        out.status=Status::IncoherentSharedStage;return out;
    }
    out.joint=Joint::evaluateWithNativeEgg(in.moveAndIV,in.originalEgg);
    switch(out.joint.status) {
        case Joint::Status::KnownCandidateConsistent:break;
        case Joint::Status::MoveCandidateContradicted:
        case Joint::Status::IVNecessaryPatternContradicted:
        case Joint::Status::IVCandidateContradicted:
        case Joint::Status::CandidatePairIncompatible:
        case Joint::Status::IncoherentNativeEggRecord:
        case Joint::Status::IncoherentParentIdentity:
        case Joint::Status::IncoherentMoveSnapshot:
        case Joint::Status::IncoherentSharedStage:
            out.status=Status::ContradictedMoveIvHistory;return out;
        default:
            out.status=Status::UnknownMoveIvHistory;return out;
    }
    out.pid=Pid::evaluate(in.originalPid);
    switch(out.pid.status) {
        case Pid::Status::KnownConditionalPidHistory:
            out.status=Status::KnownConditionalCombinedHistory;break;
        case Pid::Status::CandidatePidContradicted:
            out.status=Status::ContradictedPidHistory;break;
        case Pid::Status::IncoherentHistoricalStage:
            out.status=Status::IncoherentSharedStage;break;
        default:
            out.status=Status::UnknownPidHistory;break;
    }
    return out;
}
} // namespace Legality::Gen4PlatinumCompleteEggHistory
