#ifndef CONVERSION_ROUTE_EVIDENCE_H
#define CONVERSION_ROUTE_EVIDENCE_H

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "Conversion/Fidelity.h"
#include "Enums/GameVersion.h"
#include "Utils/MoveTransaction.h"

namespace Conversion {

inline constexpr uint16_t kRouteEvidenceVersion = 1;

enum class EvidenceConversionResult : uint8_t {
    Ok = 0,
    SameGroup = 1,
    Unsupported = 2,
    NotInDex = 3,
    Blocked = 4,
    TraitPreservationFailed = 5,
    AbilityNotRepresentable = 6,
    TextNotRepresentable = 7,
    LanguageNotRepresentable = 8,
    BallNotRepresentable = 9,
    FormNotTransferable = 10,
    RibbonMarkNotRepresentable = 11,
};

enum class ProvenanceRelation : uint8_t {
    Conversion = 1,
    Move = 2,
    Copy = 3,
    ExactCloneOf = 4,
    DerivedFrom = 5,
    ArchiveRecovery = 6,
};

struct EntityIdentity {
    uint16_t species = 0;
    uint8_t form = 0;
    uint32_t pid = 0;
    uint32_t encryptionConstant = 0;
};

struct RouteEvidence {
    uint16_t schemaVersion = kRouteEvidenceVersion;
    std::string transactionId;
    std::string sourceGameIdentity;
    std::string destinationGameIdentity;
    Enums::GameVersion sourceFormat = Enums::GameVersion::Invalid;
    Enums::GameVersion destinationFormat = Enums::GameVersion::Invalid;
    uint8_t historicalOriginVersion = 0;
    Report fidelity{};
    PokeBank::Storage::MoveTx::StoreDescriptor sourceStore{};
    PokeBank::Storage::MoveTx::StoreDescriptor destinationStore{};
    PokeBank::Storage::MoveTx::Digest sourcePayload{};
    PokeBank::Storage::MoveTx::Digest destinationPayload{};
    EntityIdentity sourceEntity{};
    EntityIdentity destinationEntity{};
    EvidenceConversionResult conversionResult = EvidenceConversionResult::Unsupported;
    bool candidateAvailable = false;
    bool lossesShownToUser = false;
    bool lossesAcknowledged = false;
    uint64_t createdAtUnix = 0;
    uint64_t acknowledgedAtUnix = 0;
    PokeBank::Storage::MoveTx::Digest acknowledgementBinding{};
    ProvenanceRelation relation = ProvenanceRelation::Conversion;

    bool requiresLossAcknowledgement() const noexcept { return fidelity.hasLosses(); }
    bool lossPolicySatisfied() const noexcept {
        return !requiresLossAcknowledgement() || (lossesShownToUser && lossesAcknowledged);
    }
};

struct PresentationItem {
    uint32_t bit = 0;
    const char* key = nullptr;
    const char* message = nullptr;
};

struct FidelitySummary {
    std::vector<std::string> losses;
    std::vector<std::string> adaptations;
    bool unknownLossBits = false;
    bool unknownAdaptationBits = false;
};

std::span<const PresentationItem> lossPresentationCatalog() noexcept;
std::span<const PresentationItem> adaptationPresentationCatalog() noexcept;
uint32_t knownLossMask() noexcept;
uint32_t knownAdaptationMask() noexcept;
FidelitySummary summarizeFidelity(const Report& report);

PokeBank::Storage::MoveTx::Digest computeAcknowledgementBinding(const RouteEvidence& evidence);
bool markLossesAcknowledged(RouteEvidence& evidence, uint64_t acknowledgedAtUnix, std::string& error);

enum class EvidenceLoadStatus : uint8_t {
    Ok,
    NotFound,
    Corrupt,
    UnsupportedVersion,
};

struct EvidenceLoadResult {
    EvidenceLoadStatus status = EvidenceLoadStatus::NotFound;
    RouteEvidence evidence{};
    std::string error;
};

class EvidenceStore {
public:
    EvidenceStore();
    explicit EvidenceStore(std::string transactionRoot);

    bool ensure(std::string& error) const;
    std::string evidencePath(const std::string& transactionId) const;
    bool persist(const RouteEvidence& evidence, std::string& error) const;
    EvidenceLoadResult load(const std::string& transactionId) const;

private:
    std::string root_;
    std::string recordsRoot_;
};

struct RetirementAuthorization {
    bool allowed = false;
    std::string reason;
};

RetirementAuthorization authorizeSourceRetirement(
    const RouteEvidence& evidence,
    const PokeBank::Storage::MoveTx::Transaction& transaction,
    bool routeEnabled);

// Product policy for this tranche: every cross-game route is still disabled.
bool routeEnabledForTrueMove(const RouteEvidence& evidence) noexcept;

// Builds the recovery-time gate consumed by MoveTx::Engine. The routeEnabled argument is
// explicit so tests can exercise evidence/restart mechanics without changing product policy.
PokeBank::Storage::MoveTx::RetirementGate makeEvidenceRetirementGate(
    EvidenceStore store,
    bool routeEnabled);

struct ProvenanceNode {
    uint64_t sequence = 0;
    ProvenanceRelation relation = ProvenanceRelation::Conversion;
    std::string transactionId;
    uint8_t historicalOriginVersion = 0;
    std::string sourceGameIdentity;
    std::string destinationGameIdentity;
    PokeBank::Storage::MoveTx::StoreDescriptor sourceStore{};
    PokeBank::Storage::MoveTx::StoreDescriptor destinationStore{};
    PokeBank::Storage::MoveTx::Digest sourcePayload{};
    PokeBank::Storage::MoveTx::Digest destinationPayload{};
    Report fidelity{};
};

ProvenanceNode makeProvenanceNode(const RouteEvidence& evidence, uint64_t sequence);
const char* provenanceRelationName(ProvenanceRelation relation) noexcept;

} // namespace Conversion

#endif
