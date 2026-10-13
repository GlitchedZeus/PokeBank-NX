#pragma once
// L4 V14 / EVENT-001 / META-001. Original Gen IV PCD vs native PK4 text
// evidence: byte-level comparison requires both original payload and proof
// the native string has not been rewritten. A Pokémon may evolve, get
// traded/renamed or migrate, and no detached PK4 proves its whole history.
#include "Legality/Gen4RawTextEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4EventTextHistoryEvidence {
enum class Status : uint8_t {
    UnavailableNative, UnknownOriginal, UnknownTransformation,
    CandidateBytesMatch, CandidateOTMismatch, CandidateNicknameMismatch
};
struct Source {
    bool originalPCDIndependentlyVerified=false;
    bool originalOTBytesKnown=false;
    bool originalNicknameBytesKnown=false;
    bool nativeOTStillComparable=false;
    bool nativeNicknameStillComparable=false;
    // For PCD events, evolution rewrites nickname raw bytes.
    // For Pal Park, use a different original-stage transformation model:
    // the following direct PCD comparison MUST NOT be used.
    bool evolvedFromCardSpecies=false;
    bool palParkOrOtherReencodingPossible=false;
    std::array<std::byte,16> sourceOT{};
    std::array<std::byte,22> sourceNickname{};
};
struct Result {
    Status status=Status::UnknownOriginal;
    bool originalTrainerCompared=false;
    bool nicknameCompared=false;
};
constexpr Result evaluate(
        bool nativeAvailable,
        const std::array<std::byte,16>& nativeOT,
        const std::array<std::byte,22>& nativeNickname,
        const Source& source) noexcept {
    Result r{};
    if(!nativeAvailable){r.status=Status::UnavailableNative;return r;}
    if(!source.originalPCDIndependentlyVerified ||
       !source.originalOTBytesKnown) return r;
    if(source.palParkOrOtherReencodingPossible ||
       !source.nativeOTStillComparable) {
        r.status=Status::UnknownTransformation;return r;
    }
    r.originalTrainerCompared=true;
    if(nativeOT!=source.sourceOT) {
        r.status=Status::CandidateOTMismatch;return r;
    }
    // Pinned PKHeX TrashByteVerifier.VerifyTrashBytesPCD:
    // after evolution the original nickname buffer is rewritten.
    if(source.evolvedFromCardSpecies ||
       !source.originalNicknameBytesKnown ||
       !source.nativeNicknameStillComparable) {
        r.status=Status::UnknownTransformation;return r;
    }
    r.nicknameCompared=true;
    r.status=nativeNickname==source.sourceNickname
        ? Status::CandidateBytesMatch : Status::CandidateNicknameMismatch;
    return r;
}
} // namespace Legality::Gen4EventTextHistoryEvidence
