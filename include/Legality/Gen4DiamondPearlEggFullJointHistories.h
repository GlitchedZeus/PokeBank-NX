#pragma once
// L2 Stage 2: bounded same-route Diamond/Pearl PID + IV + all four moves.
// A matching move set for candidate B can NEVER rescue PID/IV history A.
// Uses previously source-pinned original DP replay and the separately
// attested four-slot move stage; no detached PK4 backfills parent history.
// Every rejection here is candidate LOCAL, never global Pokemon Invalid.
#include "Legality/Gen4DiamondPearlEggAlternativeHistories.h"
#include "Legality/Gen4DiamondPearlEggMoveStage.h"
#include "Legality/Gen34OriginalSameRouteProof.h"
#include <array>
#include <cstdint>
namespace Legality::Gen4DiamondPearlEggFullJointHistories {
namespace Core=Gen4DiamondPearlEggAlternativeHistories;
namespace Move=Gen4DiamondPearlEggMoveStage;
enum class Status : uint8_t {
    UnsupportedGame, NoCandidates, CapacityExceededUnknown,
    UnknownOriginalEgg, UnknownOrContradictoryRouteIdentity,
    OneConditionallyConsistentFullHistory, UnverifiedOriginalParentSources,
    UnresolvedLocalHistory, UnknownJointEnumeration,
    AllEnumeratedLocalDaycareHistoriesContradicted
};
struct Input {
    Core::Input original{};
    // Index i MUST be the identical independently identified original
    // daycare event and physical pair as original.alternatives[i].
    std::array<Move::Input,8> moveHistories{};
    bool allJointDaycareHistoriesIndependentlyEnumerated=false;
    uint64_t originalEggSnapshotId=0;
    bool originalEggSnapshotIndependentlyAttested=false;
    std::array<Gen34OriginalSameRouteProof::Input,8> sameRouteProofs{};
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,matched=0,contradicted=0,unknown=0,unverified=0;
    std::array<Core::Status,8> originalChecks{};
    std::array<Move::Status,8> moveChecks{};
    std::array<Gen34OriginalSameRouteProof::Status,8> sameRouteChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto& egg=in.original;
    if(!Move::supported(egg.game)){
        out.status=Status::UnsupportedGame;return out;
    }
    if(!egg.count)return out;
    if(egg.count>egg.alternatives.size()){
        out.status=Status::CapacityExceededUnknown;return out;
    }
    if(!egg.originalEggEventIdentity ||
       !egg.originalUnhatchedEggIdentityIndependentlyVerified ||
       !egg.originalEggPIDIndependentlyVerified ||
       !egg.originalEggIVsIndependentlyVerified ||
       !Gen4DiamondPearlEggIVStage::valid(egg.originalEggIVs)){
        out.status=Status::UnknownOriginalEgg;return out;
    }
    bool duplicateOrMissing=false;
    for(uint8_t i=0;i<egg.count;++i){
        ++out.considered;
        const auto& c=egg.alternatives[i];
        const auto& m=in.moveHistories[i];
        if(!c.routeIdentity){
            duplicateOrMissing=true;++out.unverified;continue;
        }
        bool repeated=false,priorJointMatch=false;
        for(uint8_t j=0;j<i;++j)if(egg.alternatives[j].routeIdentity==c.routeIdentity){
            repeated=true;
            if(out.originalChecks[j]==Core::Status::OneConditionallyConsistentHistory &&
               out.moveChecks[j]==Move::Status::ConditionalMoveHistoryMatched)
                priorJointMatch=true;
        }
        if(repeated){
            duplicateOrMissing=true;++out.unverified;
            if(priorJointMatch)continue;
        }
        // Require the SAME original event, same two physical deposited
        // parents, and original source game for move and PID/IV stages.
        if(!c.originalEggEventIdentity ||
           c.originalEggEventIdentity!=egg.originalEggEventIdentity ||
           !c.sharedPhysicalParentPairIndependentlyVerified ||
           !c.ivPhysicalParentPairIdentity ||
           c.ivPhysicalParentPairIdentity!=c.pidPhysicalParentPairIdentity ||
           m.game!=egg.game ||
           m.originalEggEventIdentity!=egg.originalEggEventIdentity ||
           m.originalParentStageIdentity!=egg.originalEggEventIdentity ||
           m.originalPhysicalPairIdentity!=c.ivPhysicalParentPairIdentity ||
           !m.physicalDepositOrderIndependentlyAttested ||
           !m.sharedOriginalEventIndependentlyAttested ||
           !m.originalUnhatchedEggIdentityIndependentlyAttested ||
           // When a full original source witness is supplied, independently
           // named move-parent snapshots MUST match the same ordered pair,
           // not just species/gender and an opaque pair token.
           (c.pid.originalParentWitness.supplied &&
            (m.physicalSlots[0].snapshotIdentity!=
                 c.pid.originalParentWitness.physicalSlots[0].snapshotIdentity ||
             m.physicalSlots[1].snapshotIdentity!=
                 c.pid.originalParentWitness.physicalSlots[1].snapshotIdentity)) ||
           m.physicalSlots[0].species!=c.pid.parents[0].species ||
           m.physicalSlots[1].species!=c.pid.parents[1].species ||
           m.physicalSlots[0].gender!=c.pid.parents[0].gender ||
           m.physicalSlots[1].gender!=c.pid.parents[1].gender) {
            ++out.unverified;continue;
        }
        // PID creation and move inheritance must describe the SAME
        // original daycare pair.  An independently verified eligibility
        // claim cannot be true at one stage and false at the other.  This
        // evidence conflict is Unknown, NOT an exhaustive local contradiction.
        if(m.originalDaycarePairEligibilityIndependentlyAttested &&
           c.pid.originalDaycarePairEligibilityVerified &&
           m.eligibleOriginalDaycarePair != c.pid.eligibleOrdinaryDaycarePair){
            ++out.unverified;continue;
        }
        // Original DP Light Ball is ITEM_LIGHT_BALL=236; items refer to
        // verified creation-stage holders, NEVER current native PK4.
        // An incompatible source assertion is Unknown, not global Invalid.
        if(m.originalEggSpecies==172 &&
           c.pid.creationTimeItemsIndependentlyVerified &&
           m.originalCreationItemHistoryIndependentlyAttested &&
           m.originalEligibleLightBallHolder !=
             (c.pid.parents[0].heldItem==236 ||
              c.pid.parents[1].heldItem==236)){
            ++out.unverified;continue;
        }
        namespace Route=Gen34OriginalSameRouteProof;
        if(in.sameRouteProofs[i].supplied){
            const auto& witness=c.pid.originalParentWitness;
            if(!in.originalEggSnapshotIndependentlyAttested ||
               !in.originalEggSnapshotId ||
               !witness.supplied ||
               !witness.originalParentSnapshotsIndependentlyVerified ||
               !c.pid.creationTimeItemsIndependentlyVerified){
                ++out.unverified;continue;
            }
            out.sameRouteChecks[i]=Route::evaluate(in.sameRouteProofs[i],{
                Route::gameFor(egg.game),egg.originalEggEventIdentity,
                in.originalEggSnapshotId,c.ivPhysicalParentPairIdentity,
                witness.physicalSlots[0].snapshotIdentity,
                witness.physicalSlots[1].snapshotIdentity,true,
                c.pid.parents[0].heldItem,c.pid.parents[1].heldItem});
            if(out.sameRouteChecks[i]!=Route::Status::ConditionallySameRoute){
                ++out.unverified;continue;
            }
        }
        auto one=egg;
        one.count=1;
        one.alternatives[0]=c;
        one.allDaycareStageHypothesesIndependentlyEnumerated=true;
        const auto stage=Core::evaluate(one);
        out.originalChecks[i]=stage.status;
        const auto moves=Move::evaluate(m);
        out.moveChecks[i]=moves.status;
        if(stage.status==Core::Status::OneConditionallyConsistentHistory &&
           moves.status==Move::Status::ConditionalMoveHistoryMatched){
            ++out.matched;continue;
        }
        if(repeated)continue; // colliding archive rows cannot prove absence
        const bool stageContradicted=
            stage.status==Core::Status::AllAttestedDaycareStageHypothesesContradicted;
        const bool movesContradicted=
            moves.status==Move::Status::LocalMoveHistoryContradiction;
        if((stageContradicted || movesContradicted) &&
           c.independentlyAttestedHypothesis &&
           c.allMethodsForThisCandidateIndependentlyCovered &&
           m.originalBaseMovesIndependentlyAttested &&
           m.paternalEggMethodIndependentlyAttested &&
           m.paternalMachineMethodIndependentlyAttested &&
           m.mutualLevelMethodIndependentlyAttested) {
            ++out.contradicted;
        } else ++out.unknown;
    }
    if(out.matched)out.status=Status::OneConditionallyConsistentFullHistory;
    else if(duplicateOrMissing)
        out.status=Status::UnknownOrContradictoryRouteIdentity;
    else if(out.unverified)
        out.status=Status::UnverifiedOriginalParentSources;
    else if(out.unknown)
        out.status=Status::UnresolvedLocalHistory;
    else if(!in.allJointDaycareHistoriesIndependentlyEnumerated)
        out.status=Status::UnknownJointEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllEnumeratedLocalDaycareHistoriesContradicted;
    else out.status=Status::UnresolvedLocalHistory;
    return out;
}
} // namespace Legality::Gen4DiamondPearlEggFullJointHistories
