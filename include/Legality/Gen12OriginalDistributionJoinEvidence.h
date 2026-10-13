#pragma once
// L4 Stage 2: independently pinned original Gen I/II gifts, NPC exchange,
// Stadium and PCNY provenance joined to *one* native gift-history candidate.
// Conditional match never authenticates a distribution, receipt or Pokémon.
// No copyrighted retail card/archive bytes or ROM-derived tables are stored.
#include "Legality/Gen12GiftHistoryEvidence.h"
#include "Legality/Gen14EventDistributionSourceEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen12OriginalDistributionJoinEvidence {
namespace Gift = Gen12GiftHistoryEvidence;
namespace Archive = Gen14EventDistributionSourceEvidence;
using Digest = std::array<std::byte,32>;
enum class Status : uint8_t {
    UnknownSource, UnknownArchiveIdentity, UnknownArchiveProof,
    UnknownFieldEncoding, UnknownHistory, CandidateExcluded,
    ConditionalCandidate
};
struct Candidate {
    Gift::GiftRow original{};
    Gift::RouteWitness route{};
    // Source edition, raw OT/nickname encoding and the archived distribution
    // family are three different facts; none can be inferred from one another.
    Gift::Game independentlyIdentifiedSourceEdition=Gift::Game::Unknown;
    bool editionIdentityIndependentlyVerified=false;
    bool originalRawCharsetLinkIndependentlyVerified=false;
    Archive::Source archive{};
    // A source archive may contain multiple candidate gift rows. Exact
    // original-file SHA-256 must pin THIS gift row to its archive, not merely
    // identify another event with the same numeric trainer ID or species.
    bool archiveRowLinkIndependentlyVerified=false;
    bool archiveRowOriginalDigestIndependentlyVerified=false;
    Digest archiveRowOriginalDigest{};
    // A converted copy is not the original archive. Both distinct digests
    // and an independently checked transformation chain must be attested.
    bool convertedOriginalChainIndependentlyVerified=false;
    bool convertedPayloadDigestIndependentlyVerified=false;
    Digest convertedPayloadDigest{};
};
struct Result {
    Status status=Status::UnknownSource;
    Gift::Result gift{};
    Archive::Result archive{};
    bool sourceIdentityAgreed=false;
    bool globallyInvalid=false;
    bool authenticated=false;
    bool copyrightedPayloadEmbeddingPermitted=false;
};
constexpr Archive::Platform platformFor(Gift::Game game) noexcept {
    return Gift::isGen1(game) ? Archive::Platform::Gen1 :
           Gift::isGen2(game) ? Archive::Platform::Gen2 :
           Archive::Platform::Unknown;
}
constexpr Archive::Family familyFor(Gift::Kind kind) noexcept {
    return kind==Gift::Kind::StadiumGift?Archive::Family::StadiumGift:
           kind==Gift::Kind::PCNYGift?Archive::Family::PCNYGift:
           Archive::Family::Unknown;
}
constexpr bool sourceGradeCompatible(const Candidate& c) noexcept {
    return c.original.originalMechanicIndependentlyVerified &&
           c.original.originalRowIndependentlyVerified &&
           Gift::supportedKind(c.original.game,c.original.kind) &&
           c.editionIdentityIndependentlyVerified &&
           c.independentlyIdentifiedSourceEdition==c.original.game;
}
constexpr Result evaluate(const Gift::Observation& native,
                          const Candidate& candidate) noexcept {
    Result out{};
    if(!sourceGradeCompatible(candidate))return out;
    const bool distribution=Gift::requiresDistributionProof(candidate.original.kind);
    if(distribution) {
        const auto platform=platformFor(candidate.original.game);
        const auto family=familyFor(candidate.original.kind);
        if(candidate.archive.platform!=platform || candidate.archive.family!=family ||
           !candidate.archiveRowLinkIndependentlyVerified ||
           !candidate.archiveRowOriginalDigestIndependentlyVerified ||
           !Archive::digestNonzero(candidate.archiveRowOriginalDigest) ||
           candidate.archiveRowOriginalDigest!=candidate.archive.originalPayloadSha256) {
            out.status=Status::UnknownArchiveIdentity;return out;
        }
        out.sourceIdentityAgreed=true;
        if(candidate.archive.grade==Archive::ArchiveGrade::OriginalConvertedAuthenticated &&
           (!candidate.convertedOriginalChainIndependentlyVerified ||
            !candidate.convertedPayloadDigestIndependentlyVerified ||
            !Archive::digestNonzero(candidate.convertedPayloadDigest))) {
            out.status=Status::UnknownArchiveProof;return out;
        }
        out.archive=Archive::evaluate(candidate.archive);
        if(out.archive.status!=Archive::Status::ConditionalOriginalSource) {
            out.status=Status::UnknownArchiveProof;return out;
        }
        // The archive link is bound to a specific independently identified
        // original version and row. A template alone cannot prove receipt.
    }
    // Never compare raw game-font names if an independent encoding link is
    // missing. Even coincidentally identical bytes are not original evidence.
    auto row=candidate.original;
    if(!candidate.originalRawCharsetLinkIndependentlyVerified) {
        row.trainerName.known=false;
        row.nickname.known=false;
    }
    out.gift=Gift::evaluate(row,native,candidate.route);
    if(!candidate.originalRawCharsetLinkIndependentlyVerified) {
        out.status=Status::UnknownFieldEncoding;return out;
    }
    if(out.gift.disposition==Gift::Disposition::CandidateContradicted)
        out.status=Status::CandidateExcluded;
    else if(out.gift.disposition==Gift::Disposition::ConditionalCandidate)
        out.status=Status::ConditionalCandidate;
    else out.status=Status::UnknownHistory;
    return out;
}

// An auditor can supply multiple independently verified hypotheses. A
// contradiction of one row cannot exclude another and NEVER means globally
// Invalid even if every currently supplied row is excluded.
enum class AggregateStatus : uint8_t {
    UnknownCandidates, HasConditionalCandidate, AllProvidedCandidatesExcluded
};
struct Aggregate {
    AggregateStatus status=AggregateStatus::UnknownCandidates;
    size_t examined=0, conditional=0, unknown=0, excluded=0;
    bool authenticated=false;
    bool globallyInvalid=false;
};
inline constexpr size_t kMaxCandidates=128;
constexpr Aggregate evaluateAll(const Gift::Observation& native,
                                std::span<const Candidate> candidates) noexcept {
    Aggregate out{};
    if(candidates.empty() || candidates.size()>kMaxCandidates)return out;
    for(const auto& c:candidates) {
        const auto verdict=evaluate(native,c);
        ++out.examined;
        if(verdict.status==Status::ConditionalCandidate)++out.conditional;
        else if(verdict.status==Status::CandidateExcluded)++out.excluded;
        else ++out.unknown;
    }
    if(out.conditional)out.status=AggregateStatus::HasConditionalCandidate;
    else if(!out.unknown && out.excluded==out.examined)
        out.status=AggregateStatus::AllProvidedCandidatesExcluded;
    return out;
}
static_assert(platformFor(Gift::Game::Yellow)==Archive::Platform::Gen1);
static_assert(platformFor(Gift::Game::Crystal)==Archive::Platform::Gen2);
static_assert(familyFor(Gift::Kind::NPCTrade)==Archive::Family::Unknown);
static_assert(!Result{}.globallyInvalid && !Aggregate{}.authenticated);
} // namespace Legality::Gen12OriginalDistributionJoinEvidence
