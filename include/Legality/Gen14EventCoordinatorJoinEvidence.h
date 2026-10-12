#pragma once
// L4 Stage 5. Candidate-local, proof-preserving join across independent
// original archive, research (L5) projection, and native historical route.
// This does not call the production legality engine or assert authenticity.
#include "Legality/Gen14HistoricalEventPathEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen14EventCoordinatorJoinEvidence {
namespace Path = Gen14HistoricalEventPathEvidence;
using Digest = std::array<std::byte,32>;
enum class Family : uint8_t { Unknown, Gen1Stadium, Gen2StadiumPCNY,
    Gen3Distribution, Gen4MysteryGift, Gen4Ranger };
enum class Outcome : uint8_t { NativeUnavailable, UnknownSourceIdentity,
    CrossLaneIdentityDrift, UnknownOriginalArchive, IncoherentArchive,
    UnknownRelease, UnknownIndividualReceipt, UnknownOriginalFields,
    UnknownTransferHistory, CandidateRouteExcluded, CandidateFieldExcluded,
    ConditionalCandidate };
struct SourceKey {
    Family family=Family::Unknown;
    // Original archived distribution identity, not a globally unique card ID.
    uint32_t sourceIndex=0;
    uint16_t cardId=0, species=0, tid=0, sid=0;
    uint8_t version=0, language=0;
    constexpr bool operator==(const SourceKey&) const noexcept = default;
};
struct Archive {
    SourceKey key{};
    bool sourceKeyIndependentlyIdentified=false;
    bool originalArchiveIndependentlyAuthenticated=false;
    bool digestIndependentlyVerified=false;
    Digest originalDigest{};
    bool releaseIndependentlyCorroborated=false;
    bool individualReceiptIndependentlyCorroborated=false;
    bool originalFieldsIndependentlyExtracted=false;
};
struct L5Projection {
    SourceKey key{};
    // L5 Q07 structural projections currently have zero proven retail cards.
    // Both projection identity and original archive evidence must be external.
    bool pinnedStructuralProjectionIndependentlyChecked=false;
    bool originalArchiveIndependentlyAuthenticated=false;
    bool originalDigestIndependentlyVerified=false;
    Digest originalDigest{};
};
struct Input {
    bool nativeRecordChecksumValidated=false;
    Archive original{};
    L5Projection crossLane{};
    Path::Generation nativeGeneration=Path::Generation::Unknown;
    Path::Snapshot observed{};
    Path::Source originalHistory{};
    std::span<const Path::Step> verifiedSteps{};
};
struct Result {
    Outcome outcome=Outcome::NativeUnavailable;
    Path::Result path{};
    bool sourceIdentityAgreed=false;
    bool originalPayloadAgreed=false;
    bool authenticated=false;
    bool globallyLegal=false;
    bool globallyInvalid=false;
    uint32_t excludedFieldMask=0;
    uint32_t unknownFieldMask=0;
};
constexpr bool nonzero(const Digest& digest) noexcept {
    for(const auto b:digest)if(b!=std::byte{0})return true;
    return false;
}
constexpr bool keyWellFormed(const SourceKey& key) noexcept {
    if(key.family==Family::Unknown||key.species==0)return false;
    if(key.family==Family::Gen4MysteryGift || key.family==Family::Gen4Ranger)
        // Eleven of the 247 frozen WC4 projections have card ID zero.
        // A zero card ID is NOT a missing source identity; the INDEX matters.
        return key.sourceIndex<247;
    // A documented source ID is needed for Gen I-III, not an invented WC4 index.
    return key.sourceIndex!=0;
}
constexpr bool generationFits(Family family,Path::Generation generation) noexcept {
    switch(family) {
    case Family::Gen1Stadium: return generation==Path::Generation::I;
    case Family::Gen2StadiumPCNY: return generation==Path::Generation::II;
    case Family::Gen3Distribution: return generation==Path::Generation::III;
    case Family::Gen4MysteryGift: case Family::Gen4Ranger:
        return generation==Path::Generation::IV;
    case Family::Unknown: return false;
    }
    return false;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.nativeRecordChecksumValidated)return out;
    out.outcome=Outcome::UnknownSourceIdentity;
    const auto& a=in.original;
    const auto& l5=in.crossLane;
    if(!keyWellFormed(a.key)||!a.sourceKeyIndependentlyIdentified ||
       !l5.pinnedStructuralProjectionIndependentlyChecked ||
       !keyWellFormed(l5.key) ||
       !generationFits(a.key.family,in.originalHistory.originalGeneration))return out;
    if(a.key!=l5.key) {
        out.outcome=Outcome::CrossLaneIdentityDrift;return out;
    }
    out.sourceIdentityAgreed=true;
    if(!a.originalArchiveIndependentlyAuthenticated ||
       !l5.originalArchiveIndependentlyAuthenticated ||
       !a.digestIndependentlyVerified || !l5.originalDigestIndependentlyVerified ||
       !nonzero(a.originalDigest)||!nonzero(l5.originalDigest)) {
        out.outcome=Outcome::UnknownOriginalArchive;return out;
    }
    if(a.originalDigest!=l5.originalDigest) {
        out.outcome=Outcome::IncoherentArchive;return out;
    }
    out.originalPayloadAgreed=true;
    if(!a.releaseIndependentlyCorroborated) {
        out.outcome=Outcome::UnknownRelease;return out;
    }
    if(!a.individualReceiptIndependentlyCorroborated) {
        out.outcome=Outcome::UnknownIndividualReceipt;return out;
    }
    if(!a.originalFieldsIndependentlyExtracted ||
       !in.originalHistory.originalFieldsIndependentlyExtracted ||
       !in.originalHistory.exactOriginalSourceIndependentlyAttested) {
        out.outcome=Outcome::UnknownOriginalFields;return out;
    }
    out.path=Path::evaluate(true,in.nativeGeneration,in.observed,
        in.originalHistory,in.verifiedSteps);
    out.excludedFieldMask=out.path.contradicted;
    out.unknownFieldMask=out.path.unknown;
    switch(out.path.status) {
    case Path::Status::CandidateRouteContradicted:
        out.outcome=Outcome::CandidateRouteExcluded;break;
    case Path::Status::CandidateFieldContradicted:
        out.outcome=Outcome::CandidateFieldExcluded;break;
    case Path::Status::ConditionalCandidate:
        out.outcome=Outcome::ConditionalCandidate;break;
    default:out.outcome=Outcome::UnknownTransferHistory;break;
    }
    return out;
}
static_assert(!keyWellFormed({Family::Gen4MysteryGift,247,67,4,7200,58364,7,1}));
static_assert(!generationFits(Family::Gen3Distribution,Path::Generation::IV));
static_assert(nonzero([]{Digest x{};x[0]=std::byte{1};return x;}()));
} // namespace Legality::Gen14EventCoordinatorJoinEvidence
