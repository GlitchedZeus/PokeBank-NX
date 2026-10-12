#pragma once
// L2 B12/B13: native Diamond/Pearl original-unhatched egg and original
// daycare-parent PK4 stage binding. No save/staged writes or inferred
// forgotten provenance. Conditional IV and PID histories remain LOCAL.
#include "Legality/Gen4DiamondPearlEggIVStage.h"
#include "Legality/Gen4DiamondPearlEggPidHistory.h"
#include "Legality/Gen4DiamondPearlEggMoveStage.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4DiamondPearlNativeEggHistory {
namespace IV=Gen4DiamondPearlEggIVStage;
namespace PID=Gen4DiamondPearlEggPidHistory;
enum class Status : uint8_t {
    UnsupportedGame, MissingNativeSource, InvalidNativeRecord,
    UnknownOriginalEggStage, UnknownOriginalParentSnapshots,
    UnknownSourceGame, UnknownSharedEvent,
    IncoherentSharedEvent, StageHistoryUnresolved,
    LocalCandidateContradiction, BothConditionalStagesMatched,
    IncoherentNativeMoveSnapshots, AllThreeConditionalStagesMatched
};
struct Input {
    std::string_view game{};
    const Pokemon::Pokemon4ReadOnly* physicalSlot0=nullptr;
    const Pokemon::Pokemon4ReadOnly* physicalSlot1=nullptr;
    const Pokemon::Pokemon4ReadOnly* originalEgg=nullptr;
    // A validated native record does NOT itself prove snapshot chronology.
    bool verifiedOriginalParentSnapshots=false;
    bool verifiedOriginalPhysicalDaycareOrder=false;
    bool verifiedOriginalUnhatchedEggSnapshot=false;
    bool verifiedContainingGame=false;
    bool independentlyVerifiedSameCreationAndCollectionEvent=false;
    uint64_t parentStageToken=0,originalEggStageToken=0;
    IV::OriginalStage iv{};
    PID::Input pid{};
    // Optional original MOVE stage. Native PK4 move fields bind to the
    // exact attested original-parent/child snapshots; separate method,
    // egg-stage and creation-item attestations remain mandatory.
    bool originalMoveHistorySupplied=false;
    Gen4DiamondPearlEggMoveStage::Input moves{};
};
struct Result {
    Status status=Status::MissingNativeSource;
    IV::Check iv=IV::Check::UnknownParentIVs;
    PID::Result pid{};
    Gen4DiamondPearlEggMoveStage::Result moves{};
};
inline Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!IV::supported(in.game)) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.physicalSlot0||!in.physicalSlot1||!in.originalEgg)return out;
    const auto& p0=*in.physicalSlot0;
    const auto& p1=*in.physicalSlot1;
    const auto& child=*in.originalEgg;
    if(!p0.valid()||!p1.valid()||!child.valid()||
       p0.empty()||p1.empty()||child.empty()) {
        out.status=Status::InvalidNativeRecord;return out;
    }
    if(!in.verifiedContainingGame ||
       p0.sourceGroup()!=Enums::GameVersion::DP ||
       p1.sourceGroup()!=Enums::GameVersion::DP ||
       child.sourceGroup()!=Enums::GameVersion::DP) {
        out.status=Status::UnknownSourceGame;return out;
    }
    if(!in.verifiedOriginalUnhatchedEggSnapshot||!child.isEgg()) {
        out.status=Status::UnknownOriginalEggStage;return out;
    }
    if(!in.verifiedOriginalParentSnapshots ||
       !in.verifiedOriginalPhysicalDaycareOrder ||
       in.physicalSlot0==in.physicalSlot1 || p0.isEgg() || p1.isEgg()) {
        out.status=Status::UnknownOriginalParentSnapshots;return out;
    }
    if(!in.independentlyVerifiedSameCreationAndCollectionEvent ||
       !in.parentStageToken||!in.originalEggStageToken) {
        out.status=Status::UnknownSharedEvent;return out;
    }
    if(in.parentStageToken!=in.originalEggStageToken ||
       (in.pid.parentStageToken &&
        in.pid.parentStageToken!=in.parentStageToken) ||
       (in.pid.mtStageToken && in.pid.mtStageToken!=in.parentStageToken) ||
       (in.pid.collectionStageToken &&
        in.pid.collectionStageToken!=in.parentStageToken) ||
       (!in.pid.game.empty()&&in.pid.game!=in.game) ||
       (!in.iv.game.empty()&&in.iv.game!=in.game)) {
        out.status=Status::IncoherentSharedEvent;return out;
    }
    auto iv=in.iv;
    iv.game=in.game;
    iv.physicalSlot0=p0.ivs();
    iv.physicalSlot1=p1.ivs();
    iv.originalEggIVs=child.ivs();
    iv.historicalParentIVsVerified=true;
    iv.physicalDepositOrderVerified=true;
    iv.originalUnhatchedEggIVsVerified=true;
    // Initial random child IVs and final RNG seed are never backfilled.
    out.iv=IV::verify(iv);
    auto pid=in.pid;
    pid.game=in.game;
    pid.originalParentSnapshotsVerified=true;
    pid.originalDaycareSlotOrderVerified=true;
    pid.parents[0].species=p0.species();
    pid.parents[1].species=p1.species();
    pid.parents[0].gender=p0.gender();
    pid.parents[1].gender=p1.gender();
    pid.parents[0].pid=p0.pid();
    pid.parents[1].pid=p1.pid();
    pid.parents[0].pidIndependentlyVerified=true;
    pid.parents[1].pidIndependentlyVerified=true;
    // CRITICAL: creation-time held items and collection-time languages
    // come ONLY from caller's independent historical source attestations.
    // p0.heldItem(), p1.heldItem(), p0.language(), p1.language()
    // do not prove that original-daycare history.
    pid.parentStageToken=in.parentStageToken;
    pid.mtStageToken=in.parentStageToken;
    pid.collectionStageToken=in.parentStageToken;
    pid.originalUnhatchedEggPidIndependentlyVerified=true;
    pid.originalUnhatchedEggPid=child.pid();
    out.pid=PID::evaluate(pid);
    namespace Move=Gen4DiamondPearlEggMoveStage;
    if(in.originalMoveHistorySupplied) {
        const auto& m=in.moves;
        if(m.game!=in.game ||
           m.originalEggEventIdentity!=in.parentStageToken ||
           m.originalParentStageIdentity!=in.parentStageToken ||
           m.physicalSlots[0].species!=p0.species() ||
           m.physicalSlots[1].species!=p1.species() ||
           m.physicalSlots[0].gender!=p0.gender() ||
           m.physicalSlots[1].gender!=p1.gender() ||
           m.physicalSlots[0].originalMoves!=p0.moves() ||
           m.physicalSlots[1].originalMoves!=p1.moves() ||
           m.originalEggSpecies!=child.species() ||
           m.originalUnhatchedEggMoves!=child.moves() ||
           (m.originalDaycarePairEligibilityIndependentlyAttested &&
            pid.originalDaycarePairEligibilityVerified &&
            m.eligibleOriginalDaycarePair!=pid.eligibleOrdinaryDaycarePair) ||
           (m.originalEggSpecies==172 &&
            m.originalCreationItemHistoryIndependentlyAttested &&
            pid.creationTimeItemsIndependentlyVerified &&
            m.originalEligibleLightBallHolder!=
              (pid.parents[0].heldItem==236 || pid.parents[1].heldItem==236))) {
            out.status=Status::IncoherentNativeMoveSnapshots;return out;
        }
        // Native records are still NOT proof of original chronology:
        // evaluate() insists upon independently attested pair identity,
        // parent roles, four move methods and source creation-time items.
        out.moves=Move::evaluate(m);
    }
    const bool ivBad=out.iv==IV::Check::Contradicted;
    const bool pidBad=out.pid.status==PID::Status::OriginalEggPIDContradicted ||
                      out.pid.status==PID::Status::IncompatibleDaycarePair ||
                      out.pid.status==PID::Status::MTConsumptionContradicted;
    // Original candidate-local contradiction does not prove whole PK4
    // impossibility across other unobserved parents, events or transfers.
    const bool movesBad=in.originalMoveHistorySupplied &&
        out.moves.status==Move::Status::LocalMoveHistoryContradiction;
    if(ivBad||pidBad||movesBad)
        out.status=Status::LocalCandidateContradiction;
    else if(out.iv==IV::Check::Matched &&
            out.pid.status==PID::Status::ConditionalOriginalEggPIDMatched) {
        if(!in.originalMoveHistorySupplied)
            out.status=Status::BothConditionalStagesMatched;
        else if(out.moves.status==Move::Status::ConditionalMoveHistoryMatched)
            out.status=Status::AllThreeConditionalStagesMatched;
        else out.status=Status::StageHistoryUnresolved;
    } else out.status=Status::StageHistoryUnresolved;
    return out;
}
} // namespace Legality::Gen4DiamondPearlNativeEggHistory
