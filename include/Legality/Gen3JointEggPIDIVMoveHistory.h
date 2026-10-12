#pragma once
// L2 B07/B08/B11/B18: one original Gen III ordinary daycare egg must use
// SAME physical parents and archived stage for PID, inherited IVs and the
// father/mother four-slot move construction. All results are candidate-local.
// Original source pins: pret/pokeruby@5784633c, pokeemerald@731ad5bf,
// pokefirered@037335f4 (src/daycare.c); accepted source-backed move + IV
// stage models are composed, not duplicated or edited here.
#include "Legality/Gen3JointEggCandidateEvidence.h"
#include "Legality/Gen3EggPIDCreationHistory.h"
#include <array>
#include <cstdint>

namespace Legality::Gen3JointEggPIDIVMoveHistory {
namespace MoveIV=Gen3JointEggCandidateEvidence;
namespace PID=Gen3EggPIDCreationHistory;
namespace Move=Gen34SameParentEggHistory;
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalEggEvidence,
    UnknownOriginalDaycareEvent, IncoherentOriginalDaycareEvent,
    UnknownOriginalPairEvidence, IncoherentPhysicalParentPair,
    IncoherentHistoricalDonorRoles, IncoherentOriginalEggSpecies,
    UnknownOriginalMoveEvidence, UnknownOriginalPIDHistory,
    UnknownOriginalIVHistory, OriginalMoveCandidateContradicted,
    OriginalIVCandidateContradicted, OriginalPIDCandidateContradicted,
    AllThreeStagesConditionallyMatched
};
struct Input {
    MoveIV::Input moveAndIV{};
    PID::Input pid{};
    uint64_t originalEggEventIdentity=0;
    bool sameOriginalEggAndEventIndependentlyVerified=false;
    bool originalEggSpeciesIndependentlyVerified=false;
    uint16_t originalEggSpecies=0;
    bool originalUnhatchedEggMovesIndependentlyVerified=false;
    bool moveAndPIDSameParentSnapshotsIndependentlyVerified=false;
    bool moveAndPIDSamePhysicalPairIndependentlyVerified=false;
};
struct Result {
    Status status=Status::UnknownOriginalEggEvidence;
    MoveIV::Result moveAndIV{};
    PID::Result pid{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto game=in.pid.game;
    if(PID::family(game)==PID::Family::Unsupported ||
       Gen3EggIVInheritanceStage::family(game)==
           Gen3EggIVInheritanceStage::Family::Unknown ||
       in.moveAndIV.moveHistory.game!=game) {
        out.status=Status::UnsupportedGame;return out;
    }
    const auto& joint=in.moveAndIV;
    const auto& m=joint.moveHistory;
    if(!in.sameOriginalEggAndEventIndependentlyVerified ||
       !in.pid.originalUnhatchedEggIndependentlyVerified ||
       !in.originalEggSpeciesIndependentlyVerified) {
        out.status=Status::UnknownOriginalEggEvidence;return out;
    }
    if(!in.originalEggSpecies || in.originalEggSpecies!=m.species) {
        out.status=Status::IncoherentOriginalEggSpecies;return out;
    }
    if(!in.originalEggEventIdentity ||
       !in.pid.originalCreationStageIndependentlyVerified ||
       !in.pid.originalCollectionStageIndependentlyVerified ||
       !joint.sharedParentStageIndependentlyVerified ||
       !joint.moveParentStageToken || !joint.ivParentStageToken) {
        out.status=Status::UnknownOriginalDaycareEvent;return out;
    }
    if(in.originalEggEventIdentity!=in.pid.originalParentStageIdentity ||
       in.originalEggEventIdentity!=in.pid.originalCreationStageIdentity ||
       in.originalEggEventIdentity!=in.pid.originalCollectionStageIdentity ||
       in.originalEggEventIdentity!=joint.moveParentStageToken ||
       in.originalEggEventIdentity!=joint.ivParentStageToken) {
        out.status=Status::IncoherentOriginalDaycareEvent;return out;
    }
    if(!in.moveAndPIDSameParentSnapshotsIndependentlyVerified ||
       !in.moveAndPIDSamePhysicalPairIndependentlyVerified ||
       !in.pid.ordinaryDaycarePairIndependentlyVerified ||
       !in.pid.originalPhysicalDepositOrderIndependentlyVerified ||
       !joint.daycareSlotOrderIndependentlyVerified ||
       !m.snapshotsKnown ||
       !joint.father.originalIVsKnown ||
       !joint.mother.originalIVsKnown) {
        out.status=Status::UnknownOriginalPairEvidence;return out;
    }
    const auto& slot0=in.pid.physicalSlots[0];
    const auto& slot1=in.pid.physicalSlots[1];
    if(!slot0.originalSnapshotIndependentlyVerified ||
       !slot1.originalSnapshotIndependentlyVerified ||
       !slot0.snapshotIdentity || !slot1.snapshotIdentity ||
       slot0.snapshotIdentity==slot1.snapshotIdentity) {
        out.status=Status::UnknownOriginalPairEvidence;return out;
    }
    if(joint.daycareSlot0ParentId!=slot0.snapshotIdentity ||
       joint.daycareSlot1ParentId!=slot1.snapshotIdentity ||
       !joint.father.id || !joint.mother.id ||
       joint.father.id==joint.mother.id ||
       joint.father.id!=m.fatherId || joint.mother.id!=m.motherId) {
        out.status=Status::IncoherentPhysicalParentPair;return out;
    }
    const bool fatherSlot0=m.fatherId==slot0.snapshotIdentity &&
                           m.motherId==slot1.snapshotIdentity;
    const bool fatherSlot1=m.fatherId==slot1.snapshotIdentity &&
                           m.motherId==slot0.snapshotIdentity;
    if(!fatherSlot0&&!fatherSlot1) {
        out.status=Status::IncoherentPhysicalParentPair;return out;
    }
    const PID::Parent& father=fatherSlot0?slot0:slot1;
    const PID::Parent& mother=fatherSlot0?slot1:slot0;
    if(!father.species || !mother.species ||
       father.species!=m.fatherSpecies ||
       mother.species!=m.motherSpecies ||
       father.gender>2 || mother.gender>2) {
        out.status=Status::IncoherentHistoricalDonorRoles;return out;
    }
    // In the original ordinary breeding pair, the move "father" is not
    // necessarily a male: Ditto is move father when paired with female.
    // If paired with male/genderless, that non-Ditto is move father.
    // Without Ditto, a female mother plus male father must be attested.
    constexpr uint16_t ditto=132;
    bool correctRoles=false;
    if(father.species==ditto && mother.species!=ditto)
        correctRoles=mother.gender==1;
    else if(mother.species==ditto && father.species!=ditto)
        correctRoles=father.gender==0 || father.gender==2;
    else if(father.species!=ditto && mother.species!=ditto)
        correctRoles=father.gender==0 && mother.gender==1;
    if(!correctRoles) {
        out.status=Status::IncoherentHistoricalDonorRoles;return out;
    }
    if(!in.originalUnhatchedEggMovesIndependentlyVerified) {
        out.status=Status::UnknownOriginalMoveEvidence;return out;
    }
    // The accepted same-parent move/IV evaluator uses ONE father moveset,
    // the same physical slot mapping and source game's retail IV replay.
    out.moveAndIV=MoveIV::evaluate(joint);
    switch(out.moveAndIV.status) {
        case MoveIV::Status::UnsupportedGame:
            out.status=Status::UnsupportedGame;return out;
        case MoveIV::Status::MoveCandidateContradicted:
            out.status=Status::OriginalMoveCandidateContradicted;return out;
        case MoveIV::Status::IVCandidateContradicted:
            out.status=Status::OriginalIVCandidateContradicted;return out;
        case MoveIV::Status::KnownCandidateConsistent:break;
        default:
            out.status=Status::UnknownOriginalIVHistory;return out;
    }
    // PID history must be separately original-source attested. This does
    // not assume the IV stage RNG state equals PID collection-time RNG.
    out.pid=PID::evaluate(in.pid);
    if(out.pid.status==PID::Status::OriginalEggPIDContradicted) {
        out.status=Status::OriginalPIDCandidateContradicted;return out;
    }
    if(out.pid.status!=PID::Status::ConditionallyMatchedOriginalEggPID) {
        out.status=Status::UnknownOriginalPIDHistory;return out;
    }
    out.status=Status::AllThreeStagesConditionallyMatched;
    return out;
}
} // namespace Legality::Gen3JointEggPIDIVMoveHistory
