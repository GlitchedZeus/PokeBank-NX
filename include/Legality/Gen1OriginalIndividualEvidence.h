#pragma once
// L4 Gen I: source archive, original individual and current native specimen
// are THREE distinct evidence identities. No PK1 mutation, no global verdict.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen1OriginalIndividualEvidence {
using Digest=std::array<std::byte,32>;
enum class Bound:uint8_t { UnknownSource, UnknownOriginalRecord,
    UnknownCurrentRecord, UnknownHistoricalLineage, ConditionalIndividual };
struct Link {
    bool preciseSourceRecordIndexIndependentlyVerified=false;
    uint32_t originalRecordIndex=0;
    bool distributionArchiveSha256IndependentlyVerified=false;
    Digest distributionArchiveSha256{};
    bool originalPK1RecordSha256IndependentlyVerified=false;
    Digest originalPK1RecordSha256{};
    bool nativePK1RecordSha256IndependentlyVerified=false;
    Digest nativePK1RecordSha256{};
    bool originalRecordBytesIndependentlyVerified=false;
    bool nativeRecordBytesIndependentlyVerified=false;
    bool originalToCurrentSameIndividualIndependentlyVerified=false;
    bool historicalOperationsExhaustivelyAttested=false;
    bool originalGameFontNameBytesIndependentlyIncluded=false;
    bool nativeGameFontNameBytesIndependentlyIncluded=false;
};
constexpr bool nonzero(const Digest& hash) noexcept {
    for(auto x:hash)if(x!=std::byte{0})return true;
    return false;
}
constexpr Bound evaluate(const Link& link) noexcept {
    if(!link.preciseSourceRecordIndexIndependentlyVerified ||
       !link.distributionArchiveSha256IndependentlyVerified ||
       !nonzero(link.distributionArchiveSha256))return Bound::UnknownSource;
    if(!link.originalPK1RecordSha256IndependentlyVerified ||
       !link.originalRecordBytesIndependentlyVerified ||
       !link.originalGameFontNameBytesIndependentlyIncluded ||
       !nonzero(link.originalPK1RecordSha256))return Bound::UnknownOriginalRecord;
    if(!link.nativePK1RecordSha256IndependentlyVerified ||
       !link.nativeRecordBytesIndependentlyVerified ||
       !link.nativeGameFontNameBytesIndependentlyIncluded ||
       !nonzero(link.nativePK1RecordSha256))return Bound::UnknownCurrentRecord;
    // SHA equality alone is NOT a proof of distribution, receipt or lineage;
    // after evolution, nickname or Time Capsule tradeback it may differ.
    if(!link.originalToCurrentSameIndividualIndependentlyVerified ||
       !link.historicalOperationsExhaustivelyAttested)
        return Bound::UnknownHistoricalLineage;
    return Bound::ConditionalIndividual;
}
// An independently verified Stadium source archive must have the SAME
// independently pinned archive identity as the one used by the existing
// L4 Gen I/II original distribution join. These are not individual PK1 hashes.
constexpr bool archiveIdentityMatches(const Link& link,
    const Digest& externallyPinnedArchive) noexcept {
    return link.distributionArchiveSha256IndependentlyVerified &&
           nonzero(externallyPinnedArchive) &&
           link.distributionArchiveSha256==externallyPinnedArchive;
}
enum class Verdict:uint8_t { Unknown, CandidateExcluded, ConditionalCandidate };
enum class Group:uint8_t { NoCandidates, AllProvidedExcluded,
    UnknownAlternatives, HasConditionalCandidate };
struct Counts {
    Group group=Group::NoCandidates;
    std::size_t examined=0,unknown=0,excluded=0,conditional=0;
    bool globallyInvalid=false,globallyLegal=false;
    bool sourceAuthenticated=false,originalDistributionReceiptVerified=false;
};
inline constexpr std::size_t kMaxCandidates=128;
constexpr Counts summarize(std::span<const Verdict> candidates) noexcept {
    Counts out{};
    if(candidates.empty() || candidates.size()>kMaxCandidates)return out;
    for(auto v:candidates) {
        ++out.examined;
        if(v==Verdict::ConditionalCandidate)++out.conditional;
        else if(v==Verdict::CandidateExcluded)++out.excluded;
        else ++out.unknown; // Unknown enum tags stay Unknown; no false negative.
    }
    if(out.conditional)out.group=Group::HasConditionalCandidate;
    else if(out.unknown)out.group=Group::UnknownAlternatives;
    else out.group=Group::AllProvidedExcluded;
    return out;
}
static_assert(evaluate(Link{})==Bound::UnknownSource);
static_assert(!Counts{}.globallyInvalid && !Counts{}.globallyLegal);
} // namespace Legality::Gen1OriginalIndividualEvidence
