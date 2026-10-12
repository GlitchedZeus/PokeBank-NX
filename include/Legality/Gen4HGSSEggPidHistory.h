#pragma once
// A conditional HGSS egg PID chronology. It joins the exact retail pending
// PID creation replay with the later SetBreedEggStats Masuda transform only
// when independently attested tokens identify one daycare/collection event.
#include "Legality/Gen4HGSSMasudaPidTransform.h"
#include "Legality/Gen4HGSSPendingEggPid.h"

namespace Legality::Gen4HGSSEggPidHistory {
namespace Creation=Gen4HGSSPendingEggPid;
namespace Collection=Gen4HGSSMasudaPidTransform;
enum class Status : uint8_t {
    CreationStageNotReplayed, UnknownCollectionStage,
    IncoherentGame, UnknownEggEventIdentity, IncoherentEggEvent,
    IncoherentPendingPid, ReplayedFinalPid, CandidatePidContradicted
};
struct Input {
    Creation::Input creation{};
    Collection::Input collection{};
    // Native PK4 records do not carry these identities; they must come from
    // independent historical observation.
    uint64_t creationEggEventToken=0,collectionEggEventToken=0;
};
struct Result {
    Status status=Status::CreationStageNotReplayed;
    Creation::Result creation{};
    Collection::Result collection{};
};
constexpr bool creationReplayed(Creation::Status status) noexcept {
    return status==Creation::Status::ReplayedPendingPid;
}
constexpr Result replay(const Input& in) noexcept {
    Result out{};
    out.creation=Creation::replay(in.creation);
    if(!creationReplayed(out.creation.status))return out;
    // The second helper is only meaningful for the same original HGSS
    // cartridge family that created the pending daycare PID.
    if(in.creation.game!=in.collection.game) {
        out.status=Status::IncoherentGame;return out;
    }
    if(!in.collection.parentLanguagesAtCollectionVerified ||
       !in.collection.trainerIdAtCollectionVerified) {
        out.status=Status::UnknownCollectionStage;return out;
    }
    if(!in.creationEggEventToken || !in.collectionEggEventToken) {
        out.status=Status::UnknownEggEventIdentity;return out;
    }
    if(in.creationEggEventToken!=in.collectionEggEventToken) {
        out.status=Status::IncoherentEggEvent;return out;
    }
    if(in.collection.pendingPid!=out.creation.pendingPid) {
        out.status=Status::IncoherentPendingPid;return out;
    }
    out.collection=Collection::replay(in.collection);
    out.status=out.collection.status==Collection::Status::ReplayedFinalPid ?
        Status::ReplayedFinalPid:Status::CandidatePidContradicted;
    return out;
}
} // namespace Legality::Gen4HGSSEggPidHistory
