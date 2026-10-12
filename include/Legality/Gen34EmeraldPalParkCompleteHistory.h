#pragma once
// MOVE-001 / TRANSFER-001: one bounded, externally attested
// Emerald -> immediate Pal Park -> Gen IV teaching -> encrypted PK4
// candidate history. Historical continuity is NOT stored by detached PK3/PK4.
// Never use a conditional path or failed path as global Valid/Invalid.
#include "Legality/Gen3EmeraldPalParkJointPath.h"
#include "Legality/Gen34PostPalParkLearning.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen34EmeraldPalParkCompleteHistory {
namespace Pre=Gen3EmeraldPalParkJointPath;
namespace Post=Gen34PostPalParkLearning;
enum class Status : uint8_t {
    UnsupportedDestination, UnknownSharedCreature,
    UnknownArrivalStage, IncoherentArrivalStage,
    IncoherentMigrationSnapshot, UnknownObservedStage,
    IncoherentObservedStage, UnknownObservedPK4,
    UnknownNativeSource, IncoherentNativePokemon,
    UnresolvedOriginalTraining, OriginalTrainingContradicted,
    UnresolvedPostArrivalTraining, PostArrivalTrainingContradicted,
    KnownConditionalFullPath
};
struct Input {
    Pre::Input originalAndMigration{};
    Post::Input afterArrival{};
    const Pokemon::Pokemon4ReadOnly* nativeObserved=nullptr;
    // Independently source-attested one-creature continuity; neither
    // file content nor common PID/TID can prove historical continuity.
    bool sameCreatureAcrossAllStagesIndependentlyVerified=false;
    bool sourceGameOfObservedPK4IndependentlyVerified=false;
    bool observedNativeStageIndependentlyVerified=false;
    // Distinct records of the SAME immediate NDS arrival boundary.
    uint64_t transferArrivalToken=0,postArrivalToken=0;
    // Distinct records of the SAME post-training final snapshot.
    uint64_t postTrainingEndToken=0,nativeObservedToken=0;
};
struct Result {
    Status status=Status::UnknownSharedCreature;
    Pre::Result pre{};
    Post::Result post{};
    bool nativeObservationCompared=false;
};
constexpr Enums::GameVersion requiredGroup(std::string_view game) noexcept {
    if(game=="diamond_nds" || game=="pearl_nds")
        return Enums::GameVersion::DP;
    if(game=="platinum_nds")return Enums::GameVersion::PT;
    if(game=="heartgold_nds" || game=="soulsilver_nds")
        return Enums::GameVersion::HGSS;
    return Enums::GameVersion::Invalid;
}
constexpr bool sameMigration(const Gen34PalParkSnapshotTransition::Input& a,
                             const Gen34PalParkSnapshotTransition::Input& b) noexcept {
    return a.sourceGame==b.sourceGame &&
        a.destinationGame==b.destinationGame &&
        a.species==b.species &&
        a.independentlyVerifiedPreTransfer==b.independentlyVerifiedPreTransfer &&
        a.independentlyVerifiedImmediateArrival==b.independentlyVerifiedImmediateArrival &&
        a.unchangedSpeciesThroughArrivalVerified==b.unchangedSpeciesThroughArrivalVerified &&
        a.outboundGbaMoves==b.outboundGbaMoves &&
        a.immediateNdsMoves==b.immediateNdsMoves;
}
inline Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto& migration=in.originalAndMigration.migration;
    if(migration.sourceGame!="emerald_gba" ||
       requiredGroup(migration.destinationGame)==Enums::GameVersion::Invalid) {
        out.status=Status::UnsupportedDestination;return out;
    }
    if(!in.sameCreatureAcrossAllStagesIndependentlyVerified)return out;
    if(!in.transferArrivalToken || !in.postArrivalToken) {
        out.status=Status::UnknownArrivalStage;return out;
    }
    if(in.transferArrivalToken!=in.postArrivalToken) {
        out.status=Status::IncoherentArrivalStage;return out;
    }
    if(!sameMigration(migration,in.afterArrival.crossing)) {
        out.status=Status::IncoherentMigrationSnapshot;return out;
    }
    if(!in.postTrainingEndToken || !in.nativeObservedToken) {
        out.status=Status::UnknownObservedStage;return out;
    }
    if(in.postTrainingEndToken!=in.nativeObservedToken) {
        out.status=Status::IncoherentObservedStage;return out;
    }
    out.pre=Pre::evaluate(in.originalAndMigration);
    switch(out.pre.status) {
        case Pre::Status::KnownConditionalCombinedPath:break;
        case Pre::Status::EmeraldTrainingContradicted:
        case Pre::Status::TransferContradicted:
        case Pre::Status::IncoherentStageIdentity:
        case Pre::Status::IncoherentGameOrSpecies:
            out.status=Status::OriginalTrainingContradicted;return out;
        default:
            out.status=Status::UnresolvedOriginalTraining;return out;
    }
    out.post=Post::evaluate(in.afterArrival);
    switch(out.post.status) {
        case Post::Status::CandidateConsistent:break;
        case Post::Status::CandidateContradicted:
            out.status=Status::PostArrivalTrainingContradicted;return out;
        default:
            out.status=Status::UnresolvedPostArrivalTraining;return out;
    }
    if(!in.nativeObserved || !in.observedNativeStageIndependentlyVerified) {
        out.status=Status::UnknownObservedPK4;return out;
    }
    const auto& native=*in.nativeObserved;
    if(!native.valid() || native.empty() || native.isEgg()) {
        out.status=Status::UnknownObservedPK4;return out;
    }
    if(!in.sourceGameOfObservedPK4IndependentlyVerified ||
       native.sourceGroup()!=requiredGroup(migration.destinationGame)) {
        out.status=Status::UnknownNativeSource;return out;
    }
    out.nativeObservationCompared=true;
    if(native.species()!=migration.species ||
       native.moves()!=out.post.rebuilt ||
       native.moves()!=in.afterArrival.observedMoves) {
        out.status=Status::IncoherentNativePokemon;return out;
    }
    out.status=Status::KnownConditionalFullPath;
    return out;
}
} // namespace Legality::Gen34EmeraldPalParkCompleteHistory
