#pragma once
// BREED-003: project externally attested Gen IV daycare parent snapshots
// from strict immutable encrypted native PK4 records. PK4 does not store
// parental breeding histories or egg-group values: never infer those fields,
// deposited slot order, or the historical daycare stage from a detached PK4.
// This evidence helper does not issue global legality verdicts.
#include "Legality/Gen34BreedingPairEligibility.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen4NativeBreedingParentEvidence {
namespace Pair=Gen34BreedingPairEligibility;

enum class Status : uint8_t {
    UnsupportedGame, MissingParentRecord, InvalidNativeRecord,
    EggOrEmptyRecord, UnknownNativeSource, UnknownHistoricalStage,
    UnknownEggGroupResource, EggGroupIdentityMismatch,
    UnknownNativeGender, Projected
};
struct EggGroupAttestation {
    // Exact original personal-data source domain and species/form row.
    std::string_view game{};
    uint16_t species=0;
    uint8_t form=0,group1=0,group2=0;
    bool independentlyVerified=false;
};
struct ParentWitness {
    const Pokemon::Pokemon4ReadOnly* native=nullptr;
    uint32_t snapshotId=0;
    // Independent attestation that this preserved immutable PK4 record
    // corresponds to the parent AS IT EXISTED at the proposed daycare stage.
    bool stageSnapshotIndependentlyVerified=false;
    bool nativeSourceGameIndependentlyVerified=false;
    EggGroupAttestation eggGroups{};
};
struct ParentResult {
    Status status=Status::MissingParentRecord;
    Pair::Parent parent{};
};
struct Input {
    std::string_view game{};
    ParentWitness slot0{},slot1{};
    // An independently attested simultaneous daycare deposit order,
    // NOT inferred from stored parent genders or move-parent roles.
    bool orderedPairStageIndependentlyVerified=false;
};
struct Result {
    Status status=Status::MissingParentRecord;
    ParentResult slot0{},slot1{};
    bool pairEvaluated=false;
    Pair::Input evaluatedInput{};
    Pair::Result eligibility{};
};
constexpr Enums::GameVersion requiredGroup(std::string_view game) noexcept {
    if(game=="platinum_nds")return Enums::GameVersion::PT;
    if(game=="heartgold_nds" || game=="soulsilver_nds")
        return Enums::GameVersion::HGSS;
    return Enums::GameVersion::Invalid;
}
inline ParentResult project(std::string_view game,
                            const ParentWitness& witness) noexcept {
    ParentResult out{};
    const auto family=requiredGroup(game);
    if(family==Enums::GameVersion::Invalid) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!witness.native)return out;
    const auto& native=*witness.native;
    if(!native.valid()) {
        out.status=Status::InvalidNativeRecord;return out;
    }
    if(native.empty() || native.isEgg()) {
        out.status=Status::EggOrEmptyRecord;return out;
    }
    if(!witness.nativeSourceGameIndependentlyVerified ||
       native.sourceGroup()!=family) {
        out.status=Status::UnknownNativeSource;return out;
    }
    if(!witness.snapshotId ||
       !witness.stageSnapshotIndependentlyVerified) {
        out.status=Status::UnknownHistoricalStage;return out;
    }
    if(!witness.eggGroups.independentlyVerified) {
        out.status=Status::UnknownEggGroupResource;return out;
    }
    if(witness.eggGroups.game!=game ||
       witness.eggGroups.species!=native.species() ||
       witness.eggGroups.form!=native.form()) {
        out.status=Status::EggGroupIdentityMismatch;return out;
    }
    // The current Gen IV personal table preserves game-specific gender
    // ratio/stats but not egg groups. Do not borrow modern egg groups or
    // pretend they are stored in PK4. Require externally verified values.
    if(native.personal().hp==0 ||
       !Pair::validGroup(witness.eggGroups.group1) ||
       (witness.eggGroups.group2 &&
        !Pair::validGroup(witness.eggGroups.group2))) {
        out.status=Status::UnknownEggGroupResource;return out;
    }
    const auto gender=native.gender(); // persisted, NOT assumed from PID
    if(gender>2) {
        out.status=Status::UnknownNativeGender;return out;
    }
    out.parent.snapshotId=witness.snapshotId;
    out.parent.species=native.species();
    out.parent.eggGroup1=witness.eggGroups.group1;
    out.parent.eggGroup2=witness.eggGroups.group2;
    out.parent.gender=gender==0?Pair::Gender::Male:
        gender==1?Pair::Gender::Female:Pair::Gender::Genderless;
    out.parent.fullOTId=native.id32(); // both TID and SID
    out.parent.personalDataIndependentlyVerified=true;
    out.parent.genderIndependentlyVerified=true;
    out.parent.fullOTIdKnown=true;
    out.status=Status::Projected;
    return out;
}
inline Result evaluate(const Input& in) noexcept {
    Result out{};
    if(requiredGroup(in.game)==Enums::GameVersion::Invalid) {
        out.status=Status::UnsupportedGame;return out;
    }
    out.slot0=project(in.game,in.slot0);
    out.slot1=project(in.game,in.slot1);
    if(out.slot0.status!=Status::Projected) {
        out.status=out.slot0.status;return out;
    }
    if(out.slot1.status!=Status::Projected) {
        out.status=out.slot1.status;return out;
    }
    if(!in.orderedPairStageIndependentlyVerified) {
        out.status=Status::UnknownHistoricalStage;return out;
    }
    out.evaluatedInput.game=in.game;
    out.evaluatedInput.slot0=out.slot0.parent;
    out.evaluatedInput.slot1=out.slot1.parent;
    out.evaluatedInput.depositOrderIndependentlyVerified=true;
    out.eligibility=Pair::evaluate(out.evaluatedInput);
    out.pairEvaluated=true;
    out.status=Status::Projected;
    return out;
}
} // namespace Legality::Gen4NativeBreedingParentEvidence
