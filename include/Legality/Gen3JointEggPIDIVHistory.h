#pragma once
// L2 B07 + B04/B05/B06/B11 joint original Gen III ordinary egg history.
// Binds independent retail PID creation/collection witness to the accepted
// six-draw egg IV stage via the SAME attested unhatched egg, same original
// deposit-pair slots, same source game and same original daycare event.
// This does NOT infer forgotten parents, IV RNG or creation-time RNG
// from a detached PK3; only local stage history can be contradicted.
#include "Legality/Gen3EggPIDCreationHistory.h"
#include "Legality/Gen3EggIVInheritanceStage.h"
#include <array>
#include <cstdint>

namespace Legality::Gen3JointEggPIDIVHistory {
namespace PID=Gen3EggPIDCreationHistory;
namespace IV=Gen3EggIVInheritanceStage;
enum class Status : uint8_t {
    UnsupportedGame, UnknownSameOriginalEgg, UnknownOriginalPIDHistory,
    UnknownOriginalIVHistory, IncoherentOriginalParentSnapshots,
    UnknownSharedOriginalDaycareEvent, IncoherentOriginalDaycareEvent,
    MalformedOriginalIVCandidate, PIDCandidateContradicted,
    IVCandidateContradicted, BothOriginalStagesConditionallyMatched
};
struct Input {
    PID::Input pid{};
    uint64_t originalEggEventIdentity=0;
    bool originalPIDAndIVSameEggIndependentlyVerified=false;
    bool sameOriginalParentSnapshotsForPIDAndIVIndependentlyVerified=false;
    uint32_t ivPhysicalSlot0SnapshotIdentity=0;
    uint32_t ivPhysicalSlot1SnapshotIdentity=0;
    bool originalIVParentStageIdentityIndependentlyVerified=false;
    uint64_t originalIVParentStageIdentity=0;
    bool originalEggIVsIndependentlyVerified=false;
    std::array<uint8_t,6> originalUnhatchedEggIVs{};
    bool originalPreInheritanceChildIVsIndependentlyVerified=false;
    std::array<uint8_t,6> originalPreInheritanceChildIVs{};
    bool originalParentIVsIndependentlyVerified=false;
    std::array<uint8_t,6> ivPhysicalSlot0{},ivPhysicalSlot1{};
    bool finalIVLCRNGStateIndependentlyVerified=false;
    uint32_t finalIVLCRNGState=0;
};
struct Result {
    Status status=Status::UnknownSameOriginalEgg;
    PID::Result pid{};
    IV::CandidateCheck iv=IV::CandidateCheck::UnknownParentHistory;
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(PID::family(in.pid.game)==PID::Family::Unsupported ||
       IV::family(in.pid.game)==IV::Family::Unknown) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.originalPIDAndIVSameEggIndependentlyVerified ||
       !in.pid.originalUnhatchedEggIndependentlyVerified ||
       !in.originalEggIVsIndependentlyVerified) {
        out.status=Status::UnknownSameOriginalEgg;return out;
    }
    if(!in.sameOriginalParentSnapshotsForPIDAndIVIndependentlyVerified ||
       !in.ivPhysicalSlot0SnapshotIdentity ||
       !in.ivPhysicalSlot1SnapshotIdentity ||
       in.ivPhysicalSlot0SnapshotIdentity!=in.pid.physicalSlots[0].snapshotIdentity ||
       in.ivPhysicalSlot1SnapshotIdentity!=in.pid.physicalSlots[1].snapshotIdentity) {
        out.status=Status::IncoherentOriginalParentSnapshots;return out;
    }
    if(!in.originalEggEventIdentity ||
       !in.originalIVParentStageIdentityIndependentlyVerified ||
       !in.originalIVParentStageIdentity ||
       !in.pid.originalCreationStageIndependentlyVerified ||
       !in.pid.originalCollectionStageIndependentlyVerified ||
       !in.pid.originalParentStageIdentity) {
        out.status=Status::UnknownSharedOriginalDaycareEvent;return out;
    }
    if(in.originalEggEventIdentity!=in.pid.originalCreationStageIdentity ||
       in.originalEggEventIdentity!=in.pid.originalCollectionStageIdentity ||
       in.originalEggEventIdentity!=in.pid.originalParentStageIdentity ||
       in.originalEggEventIdentity!=in.originalIVParentStageIdentity) {
        out.status=Status::IncoherentOriginalDaycareEvent;return out;
    }
    if(!in.originalPreInheritanceChildIVsIndependentlyVerified ||
       !in.originalParentIVsIndependentlyVerified ||
       !in.finalIVLCRNGStateIndependentlyVerified) {
        out.status=Status::UnknownOriginalIVHistory;return out;
    }
    out.pid=PID::evaluate(in.pid);
    if(out.pid.status!=PID::Status::ConditionallyMatchedOriginalEggPID &&
       out.pid.status!=PID::Status::OriginalEggPIDContradicted) {
        out.status=Status::UnknownOriginalPIDHistory;return out;
    }
    out.iv=IV::verifyKnownStage(
        in.pid.game,in.finalIVLCRNGState,
        in.originalUnhatchedEggIVs,in.originalPreInheritanceChildIVs,
        in.ivPhysicalSlot0,in.ivPhysicalSlot1,true);
    if(out.iv==IV::CandidateCheck::MalformedIVDomain) {
        out.status=Status::MalformedOriginalIVCandidate;return out;
    }
    if(out.iv!=IV::CandidateCheck::Matched &&
       out.iv!=IV::CandidateCheck::CandidateMismatch) {
        out.status=Status::UnknownOriginalIVHistory;return out;
    }
    if(out.pid.status==PID::Status::OriginalEggPIDContradicted) {
        out.status=Status::PIDCandidateContradicted;return out;
    }
    if(out.iv==IV::CandidateCheck::CandidateMismatch) {
        out.status=Status::IVCandidateContradicted;return out;
    }
    out.status=Status::BothOriginalStagesConditionallyMatched;
    return out;
}
} // namespace Legality::Gen3JointEggPIDIVHistory
