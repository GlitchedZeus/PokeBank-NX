#pragma once
// L4 Stage 6. Exact independently pinned L4/Q07/Q08 research identifiers
// are not the original archived distribution, a real individual's receipt,
// or authorization to copy copyrighted retail source bytes.
// Candidate-only source information; no payload bytes or write operations.
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen14OriginalSourceRightsEvidence {
using Blob = std::array<std::byte,20>;
using Digest = std::array<std::byte,32>;
enum class Family : uint8_t { Unknown, StadiumI, StadiumIIOrPCNY, GbaIII,
    GameCubeIII, MysteryGiftIV, RangerIV };
enum class Rights : uint8_t { Unreviewed, ExplicitlyDenied,
    ResearchReferenceOnly, SpecificGrantUnderIndependentLegalReview };
enum class Status : uint8_t { UnknownResearchIdentity,
    CrossLaneSourceDrift, UnknownOriginalArchive, UnknownHistoricalRelease,
    UnknownIndividualReceipt, ConditionalSourceCandidate };
template<size_t N> constexpr bool nonzero(const std::array<std::byte,N>& bytes) noexcept {
    for(auto b:bytes)if(b!=std::byte{0})return true;
    return false;
}
constexpr bool knownFamily(Family f) noexcept {
    switch(f) {
    case Family::StadiumI: case Family::StadiumIIOrPCNY:
    case Family::GbaIII: case Family::GameCubeIII:
    case Family::MysteryGiftIV: case Family::RangerIV:return true;
    case Family::Unknown:return false;
    }
    return false;
}
struct Research {
    // All of these are prevalidated EXTERNAL observations. The corresponding
    // Python gate independently checks the actual bytes, not these booleans.
    bool l4SourceProjectionIndependentlyChecked=false;
    bool q07AllIndexedRowsIndependentlyChecked=false;
    bool q08EverySourcePermissionIndependentlyChecked=false;
    Blob expectedL4SourceBlob{},verifiedL4SourceBlob{};
    Blob expectedQ07Blob{},verifiedQ07Blob{};
    Blob expectedQ08Blob{},verifiedQ08Blob{};
    uint16_t l4Rows=0,q07Rows=0;
    uint16_t q07AuthenticatedOriginalReceipts=0;
    uint16_t q07VerifiedOriginalArchiveDigests=0;
    uint8_t q08SourcesChecked=0;
    bool q08AllRetailPayloadImportsBlocked=false;
    bool q08AllThirdPartyCodeImportsBlocked=false;
    bool q08AllRetailTableImportsBlocked=false;
};
struct Original {
    Family family=Family::Unknown;
    uint32_t independentOriginalSourceIndex=0;
    bool independentOriginalSourceIdentityVerified=false;
    bool originalArchiveIndependentlyAuthenticated=false;
    Digest originalArchiveSha256{};
    bool originalDigestIndependentlyVerified=false;
    bool originalFieldsIndependentlyExtracted=false;
    bool historicalReleaseIndependentlyCorroborated=false;
    bool individualReceiptIndependentlyVerified=false;
    bool originalToNativeIndividualChainIndependentlyVerified=false;
};
struct DistributionRights {
    Rights purportedRights=Rights::Unreviewed;
    bool sourceSpecificWrittenGrantIndependentlyReviewed=false;
    bool copiedMaterialWithinSpecificGrant=false;
    bool receivingProjectLicenseCompatibilityReviewed=false;
    bool userOwnSaveConsentPresent=false;
};
struct Result {
    Status status=Status::UnknownResearchIdentity;
    bool researchOnly=false;
    bool sourceCandidateConditional=false;
    bool originalArchiveAuthenticated=false; // This evaluator never authenticates it.
    bool individualReceiptAuthenticated=false; // Nor an individual receipt.
    bool payloadEmbeddingPermitted=false;
    bool thirdPartyCodeImportPermitted=false;
    bool bulkRetailTableImportPermitted=false;
    bool globallyValid=false,globallyInvalid=false;
};
constexpr Result evaluate(const Research& research,const Original& original,
                          const DistributionRights& rights={}) noexcept {
    Result r{};
    // The L5 Q07/Q08 and L4 revisions are pinned separately. Matching
    // card IDs or plausible OT values cannot replace whole-file identities.
    if(!research.l4SourceProjectionIndependentlyChecked ||
       !research.q07AllIndexedRowsIndependentlyChecked ||
       !research.q08EverySourcePermissionIndependentlyChecked ||
       research.l4Rows!=247 || research.q07Rows!=247 ||
       research.q08SourcesChecked!=14 ||
       research.q07AuthenticatedOriginalReceipts!=0 ||
       research.q07VerifiedOriginalArchiveDigests!=0 ||
       !research.q08AllRetailPayloadImportsBlocked ||
       !research.q08AllThirdPartyCodeImportsBlocked ||
       !research.q08AllRetailTableImportsBlocked ||
       !nonzero(research.expectedL4SourceBlob) ||
       !nonzero(research.expectedQ07Blob) ||
       !nonzero(research.expectedQ08Blob))return r;
    if(research.expectedL4SourceBlob!=research.verifiedL4SourceBlob ||
       research.expectedQ07Blob!=research.verifiedQ07Blob ||
       research.expectedQ08Blob!=research.verifiedQ08Blob) {
        r.status=Status::CrossLaneSourceDrift;return r;
    }
    r.researchOnly=true;
    r.status=Status::UnknownOriginalArchive;
    if(!knownFamily(original.family) ||
       original.independentOriginalSourceIndex==0 ||
       !original.independentOriginalSourceIdentityVerified ||
       !original.originalArchiveIndependentlyAuthenticated ||
       !original.originalDigestIndependentlyVerified ||
       !nonzero(original.originalArchiveSha256) ||
       !original.originalFieldsIndependentlyExtracted)return r;
    // Evidence supplied as an Original hypothesis has to be audited in a
    // separate chain. The L5 research catalogs have zero actual originals.
    if(!original.historicalReleaseIndependentlyCorroborated) {
        r.status=Status::UnknownHistoricalRelease;return r;
    }
    if(!original.individualReceiptIndependentlyVerified ||
       !original.originalToNativeIndividualChainIndependentlyVerified) {
        r.status=Status::UnknownIndividualReceipt;return r;
    }
    r.status=Status::ConditionalSourceCandidate;
    r.sourceCandidateConditional=true;
    // The explicitly reviewed "future grant" tag must not grant an
    // automatic payload export while the CURRENT L5 Q08 baseline blocks
    // source imports. Consent to inspect one's own save is not a license.
    (void)rights;
    return r;
}
static_assert(!knownFamily(static_cast<Family>(255)));
static_assert(!Result{}.payloadEmbeddingPermitted);
static_assert(!Result{}.globallyValid && !Result{}.globallyInvalid);
} // namespace Legality::Gen14OriginalSourceRightsEvidence
