#pragma once
// L4 V07/V08/V10/V11. Conditional event-source provenance; NEVER a global
// Invalid verdict. A PKHeX WC4 projection and a card ID are not original
// wondercard authentication. All evidence here is supplied independently.
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4EventArchiveAttestation {
using Digest = std::array<std::byte, 32>;
enum class ArchiveGrade : uint8_t {
    Unverified, OriginalPCD, ConvertedOriginalPCD,
    ReconstructedFromPGT, Altered, Unreleased
};
enum class FieldOrigin : uint8_t {
    Unavailable, ProjectionOnly, OriginalPayload
};
enum class Family : uint8_t { InitialMoves, Ribbons, RawText };
struct Field {
    FieldOrigin origin = FieldOrigin::Unavailable;
    Digest payloadSha256{};
};
struct Source {
    std::size_t projectionIndex = static_cast<std::size_t>(-1);
    uint16_t cardId = 0;
    uint16_t species = 0;
    ArchiveGrade grade = ArchiveGrade::Unverified;
    Digest originalPayloadSha256{};
    bool digestIndependentlyChecked = false;
    // Original bytes alone cannot establish this event was distributed.
    bool releaseIndependentlyCorroborated = false;
    std::array<Field, 3> fields{};
};
enum class Status : uint8_t {
    UnknownProjection, IdentityMismatch, UnknownArchive,
    NonOriginalArchive, UnknownRelease, MissingField,
    IncoherentFieldPayload, ConditionalOriginalWitness
};
struct Result {
    Status status = Status::UnknownArchive;
    uint8_t missingFields = 0;
    uint8_t incoherentFields = 0;
};
constexpr bool nonzero(Digest d) noexcept {
    for (auto b : d) if (b != std::byte{0}) return true;
    return false;
}
constexpr Result evaluate(const Source& source, std::size_t projectionCount,
                          uint16_t actualCardId, uint16_t actualSpecies) noexcept {
    Result result{};
    if (source.projectionIndex >= projectionCount) {
        result.status = Status::UnknownProjection; return result;
    }
    if (source.cardId != actualCardId || source.species != actualSpecies) {
        result.status = Status::IdentityMismatch; return result;
    }
    switch (source.grade) {
    case ArchiveGrade::Unverified:
        result.status = Status::UnknownArchive; return result;
    case ArchiveGrade::ReconstructedFromPGT:
    case ArchiveGrade::Altered:
    case ArchiveGrade::Unreleased:
        result.status = Status::NonOriginalArchive; return result;
    case ArchiveGrade::OriginalPCD:
    case ArchiveGrade::ConvertedOriginalPCD: break;
    }
    if (!source.digestIndependentlyChecked || !nonzero(source.originalPayloadSha256)) {
        result.status = Status::UnknownArchive; return result;
    }
    if (!source.releaseIndependentlyCorroborated) {
        result.status = Status::UnknownRelease; return result;
    }
    for (std::size_t i=0; i<source.fields.size(); ++i) {
        if (source.fields[i].origin != FieldOrigin::OriginalPayload)
            result.missingFields |= static_cast<uint8_t>(1u << i);
        else if (source.fields[i].payloadSha256 != source.originalPayloadSha256)
            result.incoherentFields |= static_cast<uint8_t>(1u << i);
    }
    if (result.missingFields) result.status = Status::MissingField;
    else if (result.incoherentFields)
        result.status = Status::IncoherentFieldPayload;
    else result.status = Status::ConditionalOriginalWitness;
    return result;
}
} // namespace Legality::Gen4EventArchiveAttestation
