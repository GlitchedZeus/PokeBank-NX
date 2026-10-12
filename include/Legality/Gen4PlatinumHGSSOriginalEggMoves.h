#pragma once
// L2 Stage 4: attested Platinum/HGSS ordinary daycare FOUR MOVE history.
// pret/pokeplatinum@fe1ea418a7364c93daf0d07ae55e195452d8803c
//  src/overlay005/daycare.c Egg_BuildMoveset, Egg_TryGiveVoltTackle,
//  Egg_DetermineEggSpeciesAndParentSlots / Daycare_GiveEggFromDaycare.
// pret/pokeheartgold@9d8b7591 src/get_egg.c InheritMoves,
//  Daycare_GetEggSpecies, Daycare_LightBallCheck, GiveEggToPlayer.
// Source-proved stage ORDER: base -> paternal egg -> paternal TM/HM ->
// shared learnset -> Light Ball Volt Tackle. Method-specific source
// compatibility is independently attested, never invented from observed
// parent move bytes. No detached PK4 proves original daycare provenance.
#include "Legality/Gen34OrdinaryEggMoveConstruction.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen4PlatinumHGSSOriginalEggMoves {
namespace FIFO=Gen34OrdinaryEggMoveConstruction;
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalEgg, UnknownParentSources,
    UnknownOriginalEvent, IncoherentOriginalEvent,
    UnknownDaycareEligibility, IncompatibleDaycareCandidate,
    UnknownEggSpeciesOrigin, UnknownOriginalMoveMethods,
    IncoherentMethodAttestations, UnknownOriginalCreationItems,
    ConditionalMoveHistoryMatched, LocalMoveHistoryContradicted
};
struct Parent {
    uint64_t snapshotIdentity=0;
    uint16_t species=0;
    uint8_t gender=2;
    uint16_t originalHeldItem=0;
    std::array<uint16_t,4> originalMoves{};
    bool originalSnapshotIndependentlyAttested=false;
    bool originalMoveSlotsIndependentlyAttested=false;
    bool heldItemAtCreationIndependentlyAttested=false;
};
struct Input {
    std::string_view game{};
    uint64_t originalEggEventIdentity=0,originalParentStageIdentity=0;
    uint64_t originalPhysicalPairIdentity=0;
    bool independentlyAttestedSharedSourceEvent=false;
    bool independentlyAttestedPhysicalDepositOrder=false;
    bool independentlyAttestedDaycareEligibility=false;
    bool eligibleOrdinaryDaycarePair=false;
    std::array<Parent,2> physicalSlots{};
    uint8_t moveFatherPhysicalSlot=0;
    bool moveFatherRoleIndependentlyAttested=false;
    uint16_t originalEggSpecies=0;
    bool originalEggSpeciesOriginIndependentlyAttested=false;
    bool originalUnhatchedEggMovesIndependentlyAttested=false;
    std::array<uint16_t,4> originalUnhatchedEggMoves{};
    std::array<uint16_t,4> originalBaseMoves{},originalFatherEggMoves{};
    std::array<uint16_t,4> originalFatherMachineMoves{},originalSharedLevelMoves{};
    bool baseLearnsetAtOriginalStageIndependentlyAttested=false;
    bool originalEggMoveCompatibilityIndependentlyAttested=false;
    bool originalTMHMCompatibilityIndependentlyAttested=false;
    bool originalSharedLearnsetIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalEgg;
    FIFO::Result replay{};
    uint64_t fatherSnapshot=0,motherSnapshot=0;
};
constexpr bool supported(std::string_view game) noexcept {
    return game=="platinum_nds" || game=="heartgold_nds" ||
           game=="soulsilver_nds";
}
constexpr bool has(const std::array<uint16_t,4>& from,uint16_t move) noexcept {
    for(auto m:from)if(move && m==move)return true;
    return false;
}
constexpr bool subset(const std::array<uint16_t,4>& from,
                      const std::array<uint16_t,4>& in) noexcept {
    for(auto m:from)if(m && !has(in,m))return false;
    return true;
}
constexpr bool moveDomain(const std::array<uint16_t,4>& moves) noexcept {
    for(auto m:moves)if(m>467)return false;
    return true;
}
// Both Platinum and HGSS break the paternal egg-move and shared-level
// loops on the FIRST empty move slot. TM/HM processing instead scans all
// four slots. Each acquisition pass observes paternal slot ORDER, not an
// arbitrary permutation of historical move IDs supplied by the caller.
constexpr bool orderedFromFather(const std::array<uint16_t,4>& stages,
                                  const std::array<uint16_t,4>& father,
                                  bool stopsAtEmpty) noexcept {
    uint8_t pos=0;
    for(const uint16_t claimed:stages) {
        if(!claimed)continue;
        bool found=false;
        for(;pos<father.size();++pos) {
            if(father[pos]==0 && stopsAtEmpty)return false;
            if(father[pos]==claimed) {++pos;found=true;break;}
        }
        if(!found)return false;
    }
    return true;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!supported(in.game)){
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.originalEggSpecies || in.originalEggSpecies>493 ||
       !in.originalUnhatchedEggMovesIndependentlyAttested ||
       !moveDomain(in.originalUnhatchedEggMoves))return out;
    if(!in.independentlyAttestedSharedSourceEvent ||
       !in.originalEggEventIdentity || !in.originalParentStageIdentity ||
       !in.originalPhysicalPairIdentity){
        out.status=Status::UnknownOriginalEvent;return out;
    }
    if(in.originalEggEventIdentity!=in.originalParentStageIdentity){
        out.status=Status::IncoherentOriginalEvent;return out;
    }
    if(!in.independentlyAttestedPhysicalDepositOrder ||
       !in.physicalSlots[0].originalSnapshotIndependentlyAttested ||
       !in.physicalSlots[1].originalSnapshotIndependentlyAttested ||
       !in.physicalSlots[0].originalMoveSlotsIndependentlyAttested ||
       !in.physicalSlots[1].originalMoveSlotsIndependentlyAttested ||
       !in.physicalSlots[0].snapshotIdentity ||
       !in.physicalSlots[1].snapshotIdentity ||
       in.physicalSlots[0].snapshotIdentity==in.physicalSlots[1].snapshotIdentity ||
       !in.physicalSlots[0].species || !in.physicalSlots[1].species ||
       in.physicalSlots[0].species>493 || in.physicalSlots[1].species>493 ||
       in.physicalSlots[0].gender>2 || in.physicalSlots[1].gender>2 ||
       !moveDomain(in.physicalSlots[0].originalMoves) ||
       !moveDomain(in.physicalSlots[1].originalMoves)){
        out.status=Status::UnknownParentSources;return out;
    }
    if(!in.independentlyAttestedDaycareEligibility){
        out.status=Status::UnknownDaycareEligibility;return out;
    }
    if(!in.eligibleOrdinaryDaycarePair){
        out.status=Status::IncompatibleDaycareCandidate;return out;
    }
    if(!in.moveFatherRoleIndependentlyAttested ||
       in.moveFatherPhysicalSlot>1){
        out.status=Status::UnknownParentSources;return out;
    }
    const auto& dad=in.physicalSlots[in.moveFatherPhysicalSlot];
    const auto& mom=in.physicalSlots[in.moveFatherPhysicalSlot^1u];
    const bool dd=dad.species==132,md=mom.species==132;
    // Original egg species comes from the non-Ditto physical parent.
    // Female+Ditto: Ditto becomes move-father; male+Ditto: male becomes
    // move-father and Ditto is the species/physical mother role.
    if((dd && md) ||
       (dd && (mom.gender!=1 || dad.gender!=2)) ||
       (md && (dad.gender!=0 || mom.gender!=2)) ||
       (!dd && !md && (dad.gender!=0 || mom.gender!=1))){
        out.status=Status::IncompatibleDaycareCandidate;return out;
    }
    out.fatherSnapshot=dad.snapshotIdentity;
    out.motherSnapshot=mom.snapshotIdentity;
    if(!in.originalEggSpeciesOriginIndependentlyAttested){
        out.status=Status::UnknownEggSpeciesOrigin;return out;
    }
    if(!in.baseLearnsetAtOriginalStageIndependentlyAttested ||
       !in.originalEggMoveCompatibilityIndependentlyAttested ||
       !in.originalTMHMCompatibilityIndependentlyAttested ||
       !in.originalSharedLearnsetIndependentlyAttested){
        out.status=Status::UnknownOriginalMoveMethods;return out;
    }
    if(!moveDomain(in.originalBaseMoves) ||
       !moveDomain(in.originalFatherEggMoves) ||
       !moveDomain(in.originalFatherMachineMoves) ||
       !moveDomain(in.originalSharedLevelMoves) ||
       !orderedFromFather(in.originalFatherEggMoves,dad.originalMoves,true) ||
       !orderedFromFather(in.originalFatherMachineMoves,dad.originalMoves,false) ||
       !orderedFromFather(in.originalSharedLevelMoves,dad.originalMoves,true) ||
       !subset(in.originalSharedLevelMoves,mom.originalMoves)){
        out.status=Status::IncoherentMethodAttestations;return out;
    }
    if(in.originalEggSpecies==172 &&
       (!in.physicalSlots[0].heldItemAtCreationIndependentlyAttested ||
        !in.physicalSlots[1].heldItemAtCreationIndependentlyAttested)){
        out.status=Status::UnknownOriginalCreationItems;return out;
    }
    FIFO::Input stage{};
    stage.gameId=in.game;
    stage.species=in.originalEggSpecies;
    stage.baseMoves=in.originalBaseMoves;
    stage.fatherEggMoves=in.originalFatherEggMoves;
    stage.fatherMachineMoves=in.originalFatherMachineMoves;
    stage.sharedLevelMoves=in.originalSharedLevelMoves;
    stage.lightBallStateKnown=in.originalEggSpecies!=172 ||
        (in.physicalSlots[0].heldItemAtCreationIndependentlyAttested &&
         in.physicalSlots[1].heldItemAtCreationIndependentlyAttested);
    stage.eligibleParentHeldLightBall=
        in.physicalSlots[0].originalHeldItem==236 ||
        in.physicalSlots[1].originalHeldItem==236;
    out.replay=FIFO::construct(stage);
    if(!out.replay.constructed()){
        out.status=Status::UnknownOriginalMoveMethods;return out;
    }
    out.status=out.replay.moves==in.originalUnhatchedEggMoves?
        Status::ConditionalMoveHistoryMatched:
        Status::LocalMoveHistoryContradicted;
    return out;
}
} // namespace Legality::Gen4PlatinumHGSSOriginalEggMoves
