#pragma once
// L2 Stage 2: original Diamond/Pearl ordinary-egg four-slot move witness.
// Method order: base -> paternal egg -> compatible paternal machine ->
// mutual level-up -> Light Ball Pichu; delegated to BREED-001 constructor.
// This module NEVER infers original parent snapshots, item history or TM
// availability from a detached PK4. Method legality requires separate
// source attestations and is ONLY conditional, not globally Legal/Invalid.
#include "Legality/Gen34OrdinaryEggMoveConstruction.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen4DiamondPearlEggMoveStage {
namespace FIFO=Gen34OrdinaryEggMoveConstruction;
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalEgg, UnknownOriginalParents,
    UnknownSharedDaycareEvent, IncoherentSharedDaycareEvent,
    UnknownPhysicalParentRoles, IncoherentPhysicalParentRoles,
    UnknownOriginalMoveMethods, IncoherentMethodWitness,
    UnknownCreationItems, ConditionalMoveHistoryMatched,
    LocalMoveHistoryContradiction
};
struct Parent {
    uint64_t snapshotIdentity=0;
    uint16_t species=0;
    // Verified gender at actual deposit, not derived from modern save.
    uint8_t gender=2; // 0 male, 1 female, 2 genderless
    std::array<uint16_t,4> originalMoves{};
    bool originalSnapshotIndependentlyAttested=false;
    bool originalMoveSlotsIndependentlyAttested=false;
};
struct Input {
    std::string_view game{};
    uint64_t originalEggEventIdentity=0,originalParentStageIdentity=0;
    uint64_t originalPhysicalPairIdentity=0;
    bool sharedOriginalEventIndependentlyAttested=false;
    bool physicalDepositOrderIndependentlyAttested=false;
    std::array<Parent,2> physicalSlots{};
    uint8_t fatherPhysicalSlot=0;
    bool fatherRoleIndependentlyAttested=false;
    uint16_t originalEggSpecies=0;
    bool originalUnhatchedEggIdentityIndependentlyAttested=false;
    std::array<uint16_t,4> originalUnhatchedEggMoves{};
    bool originalUnhatchedEggMovesIndependentlyAttested=false;
    std::array<uint16_t,4> base{},paternalEgg{},paternalMachine{},mutualLevel{};
    // These must refer to the SAME original source snapshot and verified
    // game-specific method witnesses. Never assume a move is TM-compatible
    // just because it appears in a parent's current moveset.
    bool originalBaseMovesIndependentlyAttested=false;
    bool paternalEggMethodIndependentlyAttested=false;
    bool paternalMachineMethodIndependentlyAttested=false;
    bool mutualLevelMethodIndependentlyAttested=false;
    bool originalCreationItemHistoryIndependentlyAttested=false;
    bool originalEligibleLightBallHolder=false;
};
struct Result {
    Status status=Status::UnknownOriginalEgg;
    FIFO::Result construction{};
    uint64_t originalFatherSnapshot=0,originalMotherSnapshot=0;
};
constexpr bool supported(std::string_view game) noexcept {
    return game=="diamond_nds" || game=="pearl_nds";
}
constexpr bool has(const std::array<uint16_t,4>& moves,uint16_t move) noexcept {
    for(const auto m:moves)if(move && m==move)return true;
    return false;
}
constexpr bool subset(const std::array<uint16_t,4>& desired,
                      const std::array<uint16_t,4>& parent) noexcept {
    for(const auto m:desired)if(m && !has(parent,m))return false;
    return true;
}
constexpr bool validMoves(const std::array<uint16_t,4>& moves) noexcept {
    for(const auto m:moves)if(m>467)return false;
    return true;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!supported(in.game)){out.status=Status::UnsupportedGame;return out;}
    if(!in.originalEggSpecies || in.originalEggSpecies>493 ||
       !in.originalUnhatchedEggIdentityIndependentlyAttested ||
       !in.originalUnhatchedEggMovesIndependentlyAttested ||
       !validMoves(in.originalUnhatchedEggMoves))return out;
    if(!in.sharedOriginalEventIndependentlyAttested ||
       !in.originalEggEventIdentity || !in.originalParentStageIdentity ||
       !in.originalPhysicalPairIdentity){
        out.status=Status::UnknownSharedDaycareEvent;return out;
    }
    if(in.originalEggEventIdentity!=in.originalParentStageIdentity){
        out.status=Status::IncoherentSharedDaycareEvent;return out;
    }
    if(!in.physicalDepositOrderIndependentlyAttested ||
       !in.physicalSlots[0].originalSnapshotIndependentlyAttested ||
       !in.physicalSlots[1].originalSnapshotIndependentlyAttested ||
       !in.physicalSlots[0].originalMoveSlotsIndependentlyAttested ||
       !in.physicalSlots[1].originalMoveSlotsIndependentlyAttested ||
       !in.physicalSlots[0].snapshotIdentity ||
       !in.physicalSlots[1].snapshotIdentity ||
       in.physicalSlots[0].snapshotIdentity==in.physicalSlots[1].snapshotIdentity ||
       !in.physicalSlots[0].species || !in.physicalSlots[1].species ||
       in.physicalSlots[0].species>493 || in.physicalSlots[1].species>493 ||
       !validMoves(in.physicalSlots[0].originalMoves) ||
       !validMoves(in.physicalSlots[1].originalMoves)) {
        out.status=Status::UnknownOriginalParents;return out;
    }
    if(!in.fatherRoleIndependentlyAttested || in.fatherPhysicalSlot>1){
        out.status=Status::UnknownPhysicalParentRoles;return out;
    }
    const auto& father=in.physicalSlots[in.fatherPhysicalSlot];
    const auto& mother=in.physicalSlots[in.fatherPhysicalSlot^1u];
    if(father.gender>2 || mother.gender>2 ||
       (father.species==132 && mother.species==132) ||
       (father.species!=132 && mother.species!=132 &&
        (father.gender!=0 || mother.gender!=1)) ||
       (father.species==132 && mother.gender!=1) ||
       (mother.species==132 && father.gender==1)){
        out.status=Status::IncoherentPhysicalParentRoles;return out;
    }
    out.originalFatherSnapshot=father.snapshotIdentity;
    out.originalMotherSnapshot=mother.snapshotIdentity;
    if(!in.originalBaseMovesIndependentlyAttested ||
       !in.paternalEggMethodIndependentlyAttested ||
       !in.paternalMachineMethodIndependentlyAttested ||
       !in.mutualLevelMethodIndependentlyAttested) {
        out.status=Status::UnknownOriginalMoveMethods;return out;
    }
    if(!validMoves(in.base) || !validMoves(in.paternalEgg) ||
       !validMoves(in.paternalMachine) || !validMoves(in.mutualLevel) ||
       !subset(in.paternalEgg,father.originalMoves) ||
       !subset(in.paternalMachine,father.originalMoves) ||
       !subset(in.mutualLevel,father.originalMoves) ||
       !subset(in.mutualLevel,mother.originalMoves)) {
        out.status=Status::IncoherentMethodWitness;return out;
    }
    if(in.originalEggSpecies==172 &&
       !in.originalCreationItemHistoryIndependentlyAttested) {
        out.status=Status::UnknownCreationItems;return out;
    }
    FIFO::Input moves{};
    moves.gameId=in.game;moves.species=in.originalEggSpecies;
    moves.baseMoves=in.base;moves.fatherEggMoves=in.paternalEgg;
    moves.fatherMachineMoves=in.paternalMachine;
    moves.sharedLevelMoves=in.mutualLevel;
    moves.lightBallStateKnown=in.originalCreationItemHistoryIndependentlyAttested;
    moves.eligibleParentHeldLightBall=in.originalEligibleLightBallHolder;
    out.construction=FIFO::construct(moves);
    if(!out.construction.constructed()){
        out.status=Status::UnknownOriginalMoveMethods;return out;
    }
    out.status=out.construction.moves==in.originalUnhatchedEggMoves ?
        Status::ConditionalMoveHistoryMatched : Status::LocalMoveHistoryContradiction;
    return out;
}
static_assert(supported("pearl_nds") && !supported("platinum_nds"));
} // namespace Legality::Gen4DiamondPearlEggMoveStage
