#pragma once
// L2 Stage 4: original Platinum / HGSS daycare PID+IV+four-move
// alternatives. Original three-stage proof must belong to ONE identified
// physical parent pair, held-item snapshot, game and acquisition event.
// An internally contradicted candidate NEVER proves global Pokemon Invalid.
#include "Legality/Gen4PlatinumHGSSJointEggHistories.h"
#include "Legality/Gen4PlatinumHGSSOriginalEggMoves.h"
#include <array>
#include <cstdint>
namespace Legality::Gen4PlatinumHGSSFullEggHistories {
namespace Two=Gen4PlatinumHGSSJointEggHistories;
namespace Moves=Gen4PlatinumHGSSOriginalEggMoves;
namespace PID=Gen4PlatinumHGSSOriginalEggPIDHistory;
namespace IV=Gen4EggIVInheritanceStage;
enum class Status : uint8_t {
    UnsupportedGame, NoCandidates, CapacityExceededUnknown,
    UnknownOriginalEgg, OneConditionallyConsistentFullHistory,
    DuplicateOrMissingRouteIdentity, UnverifiedCandidateProvenance,
    UnresolvedCandidateHistory, UnknownOrdinaryDaycareEnumeration,
    AllAttestedOrdinaryDaycareRoutesContradicted
};
struct Input {
    Two::Input original{};
    bool originalUnhatchedMovesIndependentlyAttested=false;
    uint16_t originalEggSpecies=0;
    std::array<uint16_t,4> originalEggMoves{};
    std::array<Moves::Input,8> moveStages{};
    bool allJointOrdinaryDaycareRoutesIndependentlyEnumerated=false;
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,matched=0,contradicted=0,unknown=0,unverified=0;
    std::array<Two::Status,8> pidIVChecks{};
    std::array<Moves::Status,8> moveChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto& src=in.original;
    if(!Moves::supported(src.game)){
        out.status=Status::UnsupportedGame;return out;
    }
    if(!src.count)return out;
    if(src.count>src.alternatives.size()){
        out.status=Status::CapacityExceededUnknown;return out;
    }
    if(!src.originalUnhatchedEggIndependentlyVerified ||
       !src.originalEggEventIdentity ||
       !in.originalUnhatchedMovesIndependentlyAttested ||
       !in.originalEggSpecies || in.originalEggSpecies>493 ||
       !Moves::moveDomain(in.originalEggMoves) ||
       !IV::validIVs(src.originalEggIVs)){
        out.status=Status::UnknownOriginalEgg;return out;
    }
    bool badId=false;
    for(uint8_t i=0;i<src.count;++i){
        ++out.considered;
        const auto& route=src.alternatives[i];
        const auto& move=in.moveStages[i];
        if(!route.routeIdentity){badId=true;++out.unverified;continue;}
        bool duplicate=false,priorPositive=false;
        for(uint8_t j=0;j<i;++j){
            if(src.alternatives[j].routeIdentity!=route.routeIdentity)continue;
            duplicate=true;
            if(out.pidIVChecks[j]==Two::Status::OneConditionallyConsistentJointHistory &&
               out.moveChecks[j]==Moves::Status::ConditionalMoveHistoryMatched)
                priorPositive=true;
        }
        if(duplicate){badId=true;++out.unverified;if(priorPositive)continue;}
        const auto& p=route.pid;
        if(!route.originalSourceHistoryIndependentlyAttested ||
           !route.originalDaycarePairIdentity ||
           !route.originalIVParentIdentityIndependentlyVerified ||
           !route.originalHeldItemsSameAcrossStagesIndependentlyVerified ||
           route.originalIVEventToken!=src.originalEggEventIdentity ||
           route.ivPhysicalSlot0SnapshotId!=p.parents[0].snapshotId ||
           route.ivPhysicalSlot1SnapshotId!=p.parents[1].snapshotId ||
           move.game!=src.game ||
           move.originalEggEventIdentity!=src.originalEggEventIdentity ||
           move.originalParentStageIdentity!=src.originalEggEventIdentity ||
           move.originalPhysicalPairIdentity!=route.originalDaycarePairIdentity ||
           !move.independentlyAttestedSharedSourceEvent ||
           !move.independentlyAttestedPhysicalDepositOrder ||
           move.originalEggSpecies!=in.originalEggSpecies ||
           move.originalUnhatchedEggMoves!=in.originalEggMoves ||
           move.physicalSlots[0].snapshotIdentity!=p.parents[0].snapshotId ||
           move.physicalSlots[1].snapshotIdentity!=p.parents[1].snapshotId ||
           move.physicalSlots[0].species!=p.parents[0].species ||
           move.physicalSlots[1].species!=p.parents[1].species ||
           move.physicalSlots[0].gender!=p.parents[0].gender ||
           move.physicalSlots[1].gender!=p.parents[1].gender ||
           move.physicalSlots[0].originalHeldItem!=p.parents[0].heldItem ||
           move.physicalSlots[1].originalHeldItem!=p.parents[1].heldItem ||
           (move.independentlyAttestedDaycareEligibility &&
            p.ordinaryPairEligibilityIndependentlyVerified &&
            move.eligibleOrdinaryDaycarePair!=p.eligibleOrdinaryPair)){
            ++out.unverified;continue;
        }
        // Evaluate the underlying attested PID+IV route as an isolated
        // candidate. It must independently match BEFORE any move result.
        auto one=src;
        one.count=1;
        one.alternatives[0]=route;
        one.allOrdinaryDaycareAlternativesIndependentlyEnumerated=true;
        const auto pidIV=Two::evaluate(one);
        out.pidIVChecks[i]=pidIV.status;
        const auto moves=Moves::evaluate(move);
        out.moveChecks[i]=moves.status;
        if(pidIV.status==Two::Status::OneConditionallyConsistentJointHistory &&
           moves.status==Moves::Status::ConditionalMoveHistoryMatched){
            ++out.matched;continue;
        }
        if(duplicate)continue;
        // Contradictions are counted ONLY when this individual route is
        // attested as fully covered, never if metadata or a method is
        // missing. Whole-Pokemon validity is outside this local scope.
        const bool badPIDIV=pidIV.status==
            Two::Status::AllEnumeratedLocalDaycareCandidatesContradicted;
        const bool badMoves=moves.status==
            Moves::Status::LocalMoveHistoryContradicted ||
            moves.status==Moves::Status::IncompatibleDaycareCandidate;
        if((badPIDIV||badMoves) &&
           route.fullyAttestedForCandidateLocalExclusion &&
           move.independentlyAttestedDaycareEligibility &&
           move.originalEggSpeciesOriginIndependentlyAttested &&
           move.baseLearnsetAtOriginalStageIndependentlyAttested &&
           move.originalEggMoveCompatibilityIndependentlyAttested &&
           move.originalTMHMCompatibilityIndependentlyAttested &&
           move.originalSharedLearnsetIndependentlyAttested &&
           (move.originalEggSpecies!=172 ||
             (move.physicalSlots[0].heldItemAtCreationIndependentlyAttested &&
              move.physicalSlots[1].heldItemAtCreationIndependentlyAttested)))
            ++out.contradicted;
        else ++out.unknown;
    }
    if(out.matched)out.status=Status::OneConditionallyConsistentFullHistory;
    else if(badId)out.status=Status::DuplicateOrMissingRouteIdentity;
    else if(out.unverified)out.status=Status::UnverifiedCandidateProvenance;
    else if(out.unknown)out.status=Status::UnresolvedCandidateHistory;
    else if(!in.allJointOrdinaryDaycareRoutesIndependentlyEnumerated)
        out.status=Status::UnknownOrdinaryDaycareEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllAttestedOrdinaryDaycareRoutesContradicted;
    else out.status=Status::UnresolvedCandidateHistory;
    return out;
}
} // namespace Legality::Gen4PlatinumHGSSFullEggHistories
