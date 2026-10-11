#pragma once
// EVENT-001/META-001: correlate one hypothetical Gen IV source card
// across initial/later move history, persistent ribbon masks and the
// original native PK4 nickname/OT bytes. Current PCD template projection
// does NOT provide initial moves, ribbons, or full trash byte rules.
// These must be independently sourced, not invented from a species match.
// A mismatch contradicts ONE asserted event origin, never global Invalid.
#include "Legality/Gen4EventJointMoveHistory.h"
#include "Legality/Gen4EventRibbonCandidate.h"
#include "Legality/Gen4RawTextEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4UnifiedEventCandidateEvidence {
namespace Moves=Gen4EventJointMoveHistory;
namespace Ribbons=Gen4EventRibbonCandidate;
namespace Text=Gen4RawTextEvidence;
enum class Status : uint8_t {
    UnknownCardIdentity, IdentityMismatch, AmbiguousCard,
    UnknownOriginalMoves, UnknownMoveRoutes,
    UnknownNativeRibbons, UnknownRibbonTemplate,
    InconsistentRibbonTemplate,
    UnknownNativeRawText, UnknownOriginalRawText,
    UnknownArchiveProvenance, IncoherentArchiveProvenance,
    CandidateContradicted, KnownConditionalCandidate
};
struct Input {
    Moves::Input moves{};
    bool nativeRibbonCapability=false;
    std::array<uint32_t,3> observedRibbonWords{};
    Ribbons::Requirement originalEventRibbonRules{};
    bool nativeRawTextCapability=false;
    std::array<std::byte,22> rawNickname{};
    std::array<std::byte,16> rawOT{};
    // Only compare byte-for-byte if independently verified source
    // fields (e.g. archived PCD material) are supplied. Decoded Unicode
    // and raw-terminator presence alone cannot prove event provenance.
    bool originalEventRawTextVerified=false;
    std::array<std::byte,22> originalEventNickname{};
    std::array<std::byte,16> originalEventOT{};
    // Card IDs are not globally unique across all distributions,
    // languages and archival revisions. Require all three externally
    // sourced evidence projections to identify the SAME original
    // archive payload (independently supplied 256-bit SHA-256 bytes).
    bool archiveDigestsIndependentlyVerified=false;
    std::array<std::byte,32> moveSourceArchiveSha256{};
    std::array<std::byte,32> ribbonSourceArchiveSha256{};
    std::array<std::byte,32> rawTextSourceArchiveSha256{};
};
struct Result {
    Status status=Status::UnknownCardIdentity;
    Moves::Result moveEvidence{};
    Ribbons::Result ribbonEvidence{};
    Text::Result rawTextEvidence{};
    uint16_t cardId=0;
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    out.cardId=in.moves.claimedCardId;
    if(!out.cardId || !in.moves.history.id)return out;
    out.moveEvidence=Moves::evaluate(in.moves);
    switch(out.moveEvidence.status) {
        case Moves::Status::IdentityMismatch:
            out.status=Status::IdentityMismatch;return out;
        case Moves::Status::AmbiguousCard:
            out.status=Status::AmbiguousCard;return out;
        case Moves::Status::UnknownInitialMoves:
            out.status=Status::UnknownOriginalMoves;return out;
        case Moves::Status::Unresolved:
            out.status=Status::UnknownMoveRoutes;return out;
        case Moves::Status::CandidateContradicted:
            out.status=Status::CandidateContradicted;return out;
        case Moves::Status::CompatibleCandidate:break;
    }
    out.ribbonEvidence=Ribbons::evaluate(
        in.moves.stored,out.cardId,in.nativeRibbonCapability,
        in.observedRibbonWords,in.originalEventRibbonRules);
    switch(out.ribbonEvidence.status) {
        case Ribbons::Status::IdentityMismatch:
            out.status=Status::IdentityMismatch;return out;
        case Ribbons::Status::UnavailableRibbonData:
            out.status=Status::UnknownNativeRibbons;return out;
        case Ribbons::Status::UnknownEventRibbonTemplate:
            out.status=Status::UnknownRibbonTemplate;return out;
        case Ribbons::Status::ConflictingRequirement:
            out.status=Status::InconsistentRibbonTemplate;return out;
        case Ribbons::Status::CandidateContradicted:
            out.status=Status::CandidateContradicted;return out;
        case Ribbons::Status::CandidateCompatible:break;
    }
    out.rawTextEvidence=Text::classify(
        in.nativeRawTextCapability,in.rawNickname,in.rawOT);
    if(out.rawTextEvidence.status!=Text::Status::Known) {
        out.status=Status::UnknownNativeRawText;return out;
    }
    if(!in.originalEventRawTextVerified) {
        out.status=Status::UnknownOriginalRawText;return out;
    }
    if(in.rawNickname!=in.originalEventNickname ||
       in.rawOT!=in.originalEventOT) {
        out.status=Status::CandidateContradicted;return out;
    }
    bool anyDigestByte=false;
    for(const auto value:in.moveSourceArchiveSha256)
        if(value!=std::byte{0}) anyDigestByte=true;
    if(!in.archiveDigestsIndependentlyVerified || !anyDigestByte) {
        out.status=Status::UnknownArchiveProvenance;return out;
    }
    if(in.moveSourceArchiveSha256!=in.ribbonSourceArchiveSha256 ||
       in.moveSourceArchiveSha256!=in.rawTextSourceArchiveSha256) {
        out.status=Status::IncoherentArchiveProvenance;return out;
    }
    out.status=Status::KnownConditionalCandidate;
    return out;
}
} // namespace Legality::Gen4UnifiedEventCandidateEvidence
