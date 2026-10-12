#pragma once
// L4 V08/V11/V12: immutable, strict native PK4 -> attested event-history
// projection. Only independently sourced *historical* card evidence may
// enter from the caller. Present PK4 identity, moves, ribbons and byte-level
// strings come exclusively from the checksum-validated native parser.
// Missing archive proofs remain Unknown; no global Invalid verdicts.
#include "Legality/Gen4EventNativeFieldEvidence.h"
#include "Legality/Gen4UnifiedEventCandidateEvidence.h"
#include <cstdint>

namespace Legality::Gen4EventNativeAttestedEvidence {
namespace U=Gen4UnifiedEventCandidateEvidence;
namespace A=Gen4EventArchiveAttestation;
enum class Status : uint8_t {
    UnavailableNative, UnknownArchive, UntrustedArchive,
    UnknownRelease, UnknownOriginalFields, IncoherentArchive,
    IdentityMismatch, UnresolvedCandidate, CandidateContradicted,
    ConditionalWitness
};
struct Result {
    Status status=Status::UnavailableNative;
    U::AttestedResult joint{};
    Gen4EventNativeFieldEvidence::Snapshot observed{};
};
inline Result evaluate(const Pokemon::Pokemon4ReadOnly& native,
                       const U::Input& originalCandidateEvidence,
                       const A::Source& originalSource) noexcept {
    Result result{};
    result.observed=Gen4EventNativeFieldEvidence::project(native);
    if(!result.observed.available)return result;
    // Caller controls hypothetical original-stage history (only).
    // Caller CANNOT override retained final-stage Pokémon evidence.
    auto input=originalCandidateEvidence;
    input.moves.stored=result.observed.identity;
    input.moves.observedMoves=result.observed.observedMoves;
    input.observedRibbonWords=result.observed.nativeRibbonWords;
    input.nativeRibbonCapability=true;
    input.rawNickname=result.observed.rawNickname;
    input.rawOT=result.observed.rawOT;
    input.nativeRawTextCapability=true;
    result.joint=U::evaluateAttested(input,originalSource);
    switch(result.joint.status) {
    case U::AttestedStatus::UnknownArchive:
        result.status=Status::UnknownArchive;break;
    case U::AttestedStatus::UntrustedArchive:
        result.status=Status::UntrustedArchive;break;
    case U::AttestedStatus::UnknownRelease:
        result.status=Status::UnknownRelease;break;
    case U::AttestedStatus::UnknownOriginalFields:
        result.status=Status::UnknownOriginalFields;break;
    case U::AttestedStatus::IncoherentArchive:
        result.status=Status::IncoherentArchive;break;
    case U::AttestedStatus::IdentityMismatch:
        result.status=Status::IdentityMismatch;break;
    case U::AttestedStatus::UnresolvedCandidate:
        result.status=Status::UnresolvedCandidate;break;
    case U::AttestedStatus::CandidateContradicted:
        result.status=Status::CandidateContradicted;break;
    case U::AttestedStatus::ConditionalWitness:
        result.status=Status::ConditionalWitness;break;
    }
    return result;
}
} // namespace Legality::Gen4EventNativeAttestedEvidence
