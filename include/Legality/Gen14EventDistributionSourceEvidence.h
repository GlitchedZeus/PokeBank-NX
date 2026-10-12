#pragma once
// L4 V01-V06: Gen I / II / III and GameCube distribution provenance.
// An original retail event species/OT/PID match is NEVER proof the card was
// distributed or possessed. These are candidate-scoped history gates only.
// No ROM, original wondercard payload or licensed upstream table is embedded.
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen14EventDistributionSourceEvidence {
enum class Platform : uint8_t { Unknown, Gen1, Gen2, Gen3GBA, GameCube };
enum class Family : uint8_t {
    Unknown, StadiumGift, PCNYGift, MystryMew, WishmakerJirachi,
    ChannelJirachi, BerryFix, GBAWondercard, GameCubeGift, ShadowEncounter
};
enum class ArchiveGrade : uint8_t {
    Unknown, OriginalAuthenticated, OriginalConvertedAuthenticated,
    ReconstructedOrPlaceholder, Altered, Rumor
};
enum class Status : uint8_t {
    UnknownSourceIdentity, UnknownDistribution, UntrustedArchive,
    UnknownOriginalPayload, UnknownReleaseProof, UnknownHistoricalReceipt,
    UnsupportedHistoricalRoute, ConditionalOriginalSource
};
struct Source {
    Platform platform=Platform::Unknown;
    Family family=Family::Unknown;
    ArchiveGrade grade=ArchiveGrade::Unknown;
    bool exactSourceIdentityIndependentlyProven=false;
    bool sourceMechanismIndependentlyProven=false;
    bool originalPayloadDigestVerified=false;
    std::array<std::byte,32> originalPayloadSha256{};
    bool originalPayloadFieldsIndependentlyExtracted=false;
    bool distributionReleaseIndependentlyCorroborated=false;
    bool individualOriginalReceiptHistoryKnown=false;
    // A legality witness and the right to redistribute retail binaries or
    // copyrighted event datasets are DIFFERENT facts. This bit only governs
    // future optional *embedding*, never legality evaluation.
    bool rightsToRedistributeEstablished=false;
};
struct Result {
    Status status=Status::UnknownSourceIdentity;
    bool sourceFamilyCompatible=false;
    bool distributionConditional=false;
    bool payloadEmbeddingPermitted=false;
};
constexpr bool platformFamilyCompatible(Platform p, Family f) noexcept {
    switch(p) {
    case Platform::Gen1: return f==Family::StadiumGift;
    case Platform::Gen2: return f==Family::StadiumGift || f==Family::PCNYGift;
    case Platform::Gen3GBA:
        return f==Family::MystryMew || f==Family::WishmakerJirachi ||
               f==Family::ChannelJirachi || f==Family::BerryFix ||
               f==Family::GBAWondercard;
    case Platform::GameCube:
        return f==Family::GameCubeGift || f==Family::ShadowEncounter;
    case Platform::Unknown: return false;
    }
    return false;
}
constexpr bool digestNonzero(const std::array<std::byte,32>& hash) noexcept {
    for(const auto b:hash)if(b!=std::byte{0})return true;
    return false;
}
constexpr bool originalGrade(ArchiveGrade grade) noexcept {
    return grade==ArchiveGrade::OriginalAuthenticated ||
           grade==ArchiveGrade::OriginalConvertedAuthenticated;
}
constexpr Result evaluate(const Source& source) noexcept {
    Result out{};
    if(source.platform==Platform::Unknown || source.family==Family::Unknown ||
       !source.exactSourceIdentityIndependentlyProven)
        return out;
    out.sourceFamilyCompatible=platformFamilyCompatible(source.platform,source.family);
    if(!out.sourceFamilyCompatible || source.family==Family::ShadowEncounter) {
        // Do not reinterpret a normal GameCube Shadow encounter as an
        // event gift. Nor exclude all other potential encounter routes.
        out.status=Status::UnsupportedHistoricalRoute;
        return out;
    }
    if(!source.sourceMechanismIndependentlyProven) {
        out.status=Status::UnknownDistribution;return out;
    }
    if(!originalGrade(source.grade)) {
        out.status=Status::UntrustedArchive;return out;
    }
    if(!source.originalPayloadDigestVerified ||
       !digestNonzero(source.originalPayloadSha256) ||
       !source.originalPayloadFieldsIndependentlyExtracted) {
        out.status=Status::UnknownOriginalPayload;return out;
    }
    if(!source.distributionReleaseIndependentlyCorroborated) {
        out.status=Status::UnknownReleaseProof;return out;
    }
    if(!source.individualOriginalReceiptHistoryKnown) {
        out.status=Status::UnknownHistoricalReceipt;return out;
    }
    out.distributionConditional=true;
    out.payloadEmbeddingPermitted=source.rightsToRedistributeEstablished;
    out.status=Status::ConditionalOriginalSource;
    return out;
}
static_assert(platformFamilyCompatible(Platform::Gen2,Family::PCNYGift));
static_assert(!platformFamilyCompatible(Platform::Gen1,Family::PCNYGift));
static_assert(!platformFamilyCompatible(Platform::GameCube,Family::WishmakerJirachi));
static_assert(platformFamilyCompatible(Platform::GameCube,Family::ShadowEncounter));
} // namespace Legality::Gen14EventDistributionSourceEvidence
