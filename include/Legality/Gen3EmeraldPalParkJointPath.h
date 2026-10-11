#pragma once
// MOVE-001/TRANSFER-001: a *single externally sourced* Emerald original
// 4-slot learning sequence must join the SAME immediately pre-Pal Park
// snapshot and immediately retained Gen IV arrival state. Without a
// shared source/stage proof there is no valid joint historical witness.
#include "Legality/Gen3EmeraldLevelUpTimeline.h"
#include "Legality/Gen34PalParkSnapshotTransition.h"
#include <cstdint>
namespace Legality::Gen3EmeraldPalParkJointPath {
namespace Emerald=Gen3EmeraldLevelUpTimeline;
namespace Transfer=Gen34PalParkSnapshotTransition;
enum class Status : uint8_t {
    UnknownSharedSource, IncoherentStageIdentity, IncoherentGameOrSpecies,
    UnresolvedEmeraldTraining, EmeraldTrainingContradicted,
    UnresolvedTransfer, TransferContradicted, KnownConditionalCombinedPath
};
struct Input {
    Emerald::Input learning{};
    Transfer::Input migration{};
    bool sameOriginalSourceSnapshotVerified=false;
    uint64_t originalLearningStageToken=0;
    uint64_t outboundGbaStageToken=0;
};
struct Result {
    Status status=Status::UnknownSharedSource;
    Emerald::Result learning{};
    Transfer::Result migration{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result result{};
    if(!in.sameOriginalSourceSnapshotVerified ||
       !in.originalLearningStageToken ||
       !in.outboundGbaStageToken)return result;
    if(in.originalLearningStageToken!=in.outboundGbaStageToken) {
        result.status=Status::IncoherentStageIdentity;return result;
    }
    if(in.learning.game!=in.migration.sourceGame ||
       in.learning.species!=in.migration.species) {
        result.status=Status::IncoherentGameOrSpecies;return result;
    }
    result.learning=Emerald::evaluate(in.learning);
    switch(result.learning.status) {
        case Emerald::Status::CandidateConsistent:break;
        case Emerald::Status::CandidateContradicted:
            result.status=Status::EmeraldTrainingContradicted;return result;
        default:
            result.status=Status::UnresolvedEmeraldTraining;return result;
    }
    if(result.learning.reconstructed!=in.migration.outboundGbaMoves){
        // A path with one identical pre-transfer source snapshot cannot
        // switch to another hypothetical moveset before migration.
        result.status=Status::IncoherentStageIdentity;return result;
    }
    result.migration=Transfer::evaluate(in.migration);
    switch(result.migration.status) {
        case Transfer::Status::CandidateConsistent:
            result.status=Status::KnownConditionalCombinedPath;break;
        case Transfer::Status::CandidateContradicted:
            result.status=Status::TransferContradicted;break;
        default:
            result.status=Status::UnresolvedTransfer;break;
    }
    return result;
}
} // namespace Legality::Gen3EmeraldPalParkJointPath
