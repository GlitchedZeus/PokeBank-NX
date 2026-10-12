#pragma once
// L2 B16: read-only native PK4 source binding for the attested egg-IV
// hypothesis evaluator. Existing encrypted records remain immutable.
// Snapshot origin, stage chronology, and historical held items are
// independent source attestations; no detached PK4 can supply them.
#include "Legality/Gen4EggIVHistoricalCandidateSet.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen4NativeBreedingIVHistory {
namespace Hist=Gen4EggIVHistoricalCandidateSet;
enum class Status : uint8_t {
    UnsupportedGame, MissingNativeRecord, InvalidNativeRecord,
    UnknownOriginalUnhatchedEgg, UnknownParentSnapshot,
    UnknownSourceGame, UnknownStageLink,
    IncoherentStageLink, Evaluated
};
struct Input {
    std::string_view game{};
    const Pokemon::Pokemon4ReadOnly* slot0=nullptr;
    const Pokemon::Pokemon4ReadOnly* slot1=nullptr;
    const Pokemon::Pokemon4ReadOnly* originalEgg=nullptr;
    uint64_t slot0StageIdentity=0,slot1StageIdentity=0,eggStageIdentity=0;
    bool parentSnapshotsIndependentlyVerified=false;
    bool physicalDaycareOrderIndependentlyVerified=false;
    bool originalUnhatchedEggIndependentlyVerified=false;
    bool containingSourceGameIndependentlyVerified=false;
    bool sharedStageLinkIndependentlyVerified=false;
    // The original RANDOM pre-inheritance child IVs require a separate
    // source witness. They cannot be read back from final child.ivs().
    bool originalInitialChildIVsIndependentlyVerified=false;
    std::array<uint8_t,6> originalInitialChildIVs{};
    // Hypotheses provide EXTERNAL original-stage items and RNG endpoints.
    // Neither parent->heldItem() is used to replace that testimony.
    Hist::Input historicalAlternatives{};
};
struct Result {
    Status status=Status::MissingNativeRecord;
    Hist::Result histories{};
};
constexpr Enums::GameVersion sourceGroup(std::string_view game) noexcept {
    if(game=="platinum_nds")return Enums::GameVersion::PT;
    if(game=="heartgold_nds"||game=="soulsilver_nds")
        return Enums::GameVersion::HGSS;
    return Enums::GameVersion::Invalid;
}
inline Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto group=sourceGroup(in.game);
    if(group==Enums::GameVersion::Invalid) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.slot0 || !in.slot1 || !in.originalEgg)return out;
    if(!in.slot0->valid()||!in.slot1->valid()||!in.originalEgg->valid() ||
       in.slot0->empty()||in.slot1->empty()||in.originalEgg->empty()) {
        out.status=Status::InvalidNativeRecord;return out;
    }
    if(!in.containingSourceGameIndependentlyVerified ||
       in.slot0->sourceGroup()!=group || in.slot1->sourceGroup()!=group ||
       in.originalEgg->sourceGroup()!=group) {
        out.status=Status::UnknownSourceGame;return out;
    }
    if(!in.originalUnhatchedEggIndependentlyVerified ||
       !in.originalEgg->isEgg()) {
        out.status=Status::UnknownOriginalUnhatchedEgg;return out;
    }
    if(!in.parentSnapshotsIndependentlyVerified ||
       !in.physicalDaycareOrderIndependentlyVerified) {
        out.status=Status::UnknownParentSnapshot;return out;
    }
    if(!in.sharedStageLinkIndependentlyVerified ||
       !in.slot0StageIdentity || !in.slot1StageIdentity ||
       !in.eggStageIdentity) {
        out.status=Status::UnknownStageLink;return out;
    }
    if(in.slot0StageIdentity!=in.slot1StageIdentity ||
       in.slot0StageIdentity!=in.eggStageIdentity ||
       (in.historicalAlternatives.originalStageIdentity &&
        in.historicalAlternatives.originalStageIdentity!=in.eggStageIdentity) ||
       (!in.historicalAlternatives.game.empty() &&
        in.historicalAlternatives.game!=in.game)) {
        out.status=Status::IncoherentStageLink;return out;
    }
    auto linked=in.historicalAlternatives;
    linked.game=in.game;
    linked.originalStageIdentity=in.eggStageIdentity;
    linked.originalStageIdentityIndependentlyVerified=true;
    linked.initialChild=in.originalInitialChildIVs;
    linked.physicalSlot0=in.slot0->ivs();
    linked.physicalSlot1=in.slot1->ivs();
    linked.observedChild=in.originalEgg->ivs();
    linked.originalPhysicalParentSnapshotsVerified=true;
    linked.originalPhysicalSlotOrderVerified=true;
    linked.originalInitialChildIVsVerified=
        in.originalInitialChildIVsIndependentlyVerified;
    linked.originalUnhatchedChildIVsVerified=true;
    out.histories=Hist::evaluate(linked);
    out.status=Status::Evaluated;
    return out;
}
} // namespace Legality::Gen4NativeBreedingIVHistory
