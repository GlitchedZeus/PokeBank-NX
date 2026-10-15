#pragma once
// L4 Stage 7: end-to-end candidate-only original source -> record -> native
// provenance gate. This is independent of the production legality verdict.
// No file writes, original retail bytes, or redistribution authorization.
#include "Legality/Gen14OriginalCurrentChainEvidence.h"
#include "Legality/Gen14OriginalSourceRightsEvidence.h"
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen14FinalProvenanceGateEvidence {
namespace H=Gen14OriginalCurrentChainEvidence;
namespace R=Gen14OriginalSourceRightsEvidence;
enum class Status : uint8_t {
    UnknownNative, UnknownCrossLaneResearch, CrossLaneSourceDrift,
    UnknownOriginalSourceOrReceipt, UnknownSourceRecordBinding,
    UnknownHistoricalRecordChain, CandidateRouteExcluded,
    CandidateFieldExcluded, ConditionalCandidate
};
struct SourceToRecordLink {
    // The original distribution archive and original PKM are DIFFERENT
    // original objects: each must be independently identified and hashed.
    R::Family originalDistributionFamily=R::Family::Unknown;
    uint32_t originalDistributionIndex=0;
    R::Digest originalDistributionArchiveDigest{};
    H::Digest originalPokemonRecordDigest{};
    H::Digest finalPokemonRecordDigest{};
    bool sourceRecordPairIndependentlyAttested=false;
    bool originalRecordWasExtractedFromAttestedSource=false;
    bool originalToCurrentIndividualIndependentlyAttested=false;
};
struct Input {
    R::Research research{};
    R::Original original{};
    R::DistributionRights rights{};
    H::Input historical{};
    SourceToRecordLink binding{};
};
struct Result {
    Status status=Status::UnknownNative;
    R::Status researchSourceStatus=R::Status::UnknownResearchIdentity;
    H::Status historicalStatus=H::Status::UnknownNative;
    uint32_t candidateExcludedFields=0, unknownHistoricalFields=0;
    std::size_t independentlyVerifiedHops=0;
    bool researchCatalogsAgreed=false;
    bool candidateOnly=true;
    bool originalEventAuthenticated=false, individualReceiptAuthenticated=false;
    bool payloadEmbeddingPermitted=false, thirdPartyImportPermitted=false;
    bool globallyLegal=false,globallyInvalid=false;
};
constexpr Result evaluate(const Input& input) noexcept {
    Result out{};
    if(!input.historical.finalNativeIndependentlyValidated)return out;
    const auto source=R::evaluate(input.research,input.original,input.rights);
    out.researchSourceStatus=source.status;
    if(source.status==R::Status::CrossLaneSourceDrift) {
        out.status=Status::CrossLaneSourceDrift;return out;
    }
    if(!source.researchOnly) {
        out.status=Status::UnknownCrossLaneResearch;return out;
    }
    out.researchCatalogsAgreed=true;
    if(source.status!=R::Status::ConditionalSourceCandidate) {
        out.status=Status::UnknownOriginalSourceOrReceipt;return out;
    }
    const auto& b=input.binding;
    const auto& c=input.historical;
    if(!b.sourceRecordPairIndependentlyAttested ||
       !b.originalRecordWasExtractedFromAttestedSource ||
       !b.originalToCurrentIndividualIndependentlyAttested ||
       b.originalDistributionFamily!=input.original.family ||
       b.originalDistributionIndex!=input.original.independentOriginalSourceIndex ||
       !R::nonzero(b.originalDistributionArchiveDigest) ||
       !H::digestNonzero(b.originalPokemonRecordDigest) ||
       !H::digestNonzero(b.finalPokemonRecordDigest) ||
       b.originalDistributionArchiveDigest!=input.original.originalArchiveSha256 ||
       b.originalPokemonRecordDigest!=c.originalRecordDigest ||
       b.finalPokemonRecordDigest!=c.finalRecordDigest ||
       !c.originalToFinalIndividualIndependentlyLinked) {
        out.status=Status::UnknownSourceRecordBinding;return out;
    }
    const auto hist=H::evaluate(c);
    out.historicalStatus=hist.status;
    out.independentlyVerifiedHops=hist.inspectedHops;
    out.candidateExcludedFields=hist.contradicted;
    out.unknownHistoricalFields=hist.unknown;
    switch(hist.status) {
    case H::Status::CandidateRouteExcluded:
        out.status=Status::CandidateRouteExcluded;break;
    case H::Status::CandidateFieldExcluded:
        out.status=Status::CandidateFieldExcluded;break;
    case H::Status::ConditionalCandidate:
        out.status=Status::ConditionalCandidate;break;
    default:
        out.status=Status::UnknownHistoricalRecordChain;break;
    }
    return out;
}
enum class AggregateStatus : uint8_t {
    NoIndependentlyBoundCandidates, ConditionalCandidatesExist,
    AllProvidedCandidatesExcluded, AmbiguousOrUnknown
};
struct Aggregate {
    AggregateStatus status=AggregateStatus::NoIndependentlyBoundCandidates;
    std::size_t inspected=0,conditional=0,excluded=0,unknown=0;
    bool originalEventAuthenticated=false,globallyInvalid=false;
    bool payloadEmbeddingPermitted=false;
};
inline constexpr std::size_t kMaxCandidates=64;
constexpr Aggregate evaluateAll(std::span<const Input> candidates) noexcept {
    Aggregate a{};
    if(candidates.empty()||candidates.size()>kMaxCandidates)return a;
    for(const auto& candidate:candidates) {
        ++a.inspected;
        const auto r=evaluate(candidate);
        if(r.status==Status::ConditionalCandidate)++a.conditional;
        else if(r.status==Status::CandidateFieldExcluded ||
                r.status==Status::CandidateRouteExcluded)++a.excluded;
        else ++a.unknown;
    }
    if(a.conditional)a.status=AggregateStatus::ConditionalCandidatesExist;
    else if(a.unknown)a.status=AggregateStatus::AmbiguousOrUnknown;
    else if(a.excluded==a.inspected)
        a.status=AggregateStatus::AllProvidedCandidatesExcluded;
    return a;
}
static_assert(!Result{}.globallyLegal && !Result{}.globallyInvalid);
static_assert(!Result{}.payloadEmbeddingPermitted);
static_assert(!Aggregate{}.originalEventAuthenticated);
} // namespace Legality::Gen14FinalProvenanceGateEvidence
