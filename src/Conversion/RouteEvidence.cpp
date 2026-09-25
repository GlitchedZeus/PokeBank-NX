#include "Conversion/RouteEvidence.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <span>
#include <string_view>
#include <sys/stat.h>
#include <utility>
#include <vector>

#include "Utils/DurableFile.h"
#include "Utils/PokeBankPaths.h"

namespace Conversion {
namespace {

using PokeBank::Storage::MoveTx::Digest;
using PokeBank::Storage::MoveTx::StoreDescriptor;
using PokeBank::Storage::MoveTx::StoreType;
using PokeBank::Storage::MoveTx::Transaction;

constexpr size_t kMaxEvidenceBytes = 64u * 1024u;
constexpr size_t kMaxStringBytes = 128;
constexpr std::array<uint8_t, 8> kEvidenceMagic{'P','B','C','E','V','0','1',0};
constexpr std::array<uint8_t, 8> kBindingMagic{'P','B','C','E','B','0','1',0};

constexpr std::array<PresentationItem, 14> kLossPresentation{{
    {static_cast<uint32_t>(Loss::Gen3EVClamped), "Gen3EVClamped",
     "Gen III EV values above 252 will be clamped to 252."},
    {static_cast<uint32_t>(Loss::TeraDataDropped), "TeraDataDropped",
     "Tera data cannot be stored in the destination format and will be removed."},
    {static_cast<uint32_t>(Loss::ZAAlphaDropped), "ZAAlphaDropped",
     "Alpha status cannot be stored in the destination format and will be removed."},
    {static_cast<uint32_t>(Loss::DivergentGameDataDropped), "DivergentGameDataDropped",
     "Destination-incompatible game-specific data will be removed."},
    {static_cast<uint32_t>(Loss::OriginGameRestamped), "OriginGameRestamped",
     "Origin-game representation must be restamped for the destination format."},
    {static_cast<uint32_t>(Loss::MoveDropped), "MoveDropped",
     "One or more moves cannot exist in the destination and will be removed."},
    {static_cast<uint32_t>(Loss::RelearnMoveDropped), "RelearnMoveDropped",
     "One or more relearn moves cannot exist in the destination and will be removed."},
    {static_cast<uint32_t>(Loss::HeldItemDropped), "HeldItemDropped",
     "The held item cannot be carried into the destination and will be removed."},
    {static_cast<uint32_t>(Loss::StatTrainingReset), "StatTrainingReset",
     "Stat-training values cannot be preserved in this conversion and will be reset."},
    {static_cast<uint32_t>(Loss::PLAExclusiveDataDropped), "PLAExclusiveDataDropped",
     "Legends: Arceus-exclusive data cannot be represented in the destination and will be removed."},
    {static_cast<uint32_t>(Loss::RibbonDataDropped), "RibbonDataDropped",
     "One or more ribbons cannot be represented in the destination and will be removed."},
    {static_cast<uint32_t>(Loss::AbilitySlotNormalized), "AbilitySlotNormalized",
     "The native ability-slot selector cannot be preserved exactly and will be normalized."},
    {static_cast<uint32_t>(Loss::HomeTrackerDropped), "HomeTrackerDropped",
     "The HOME tracker cannot be stored in the destination format and will be removed."},
    {static_cast<uint32_t>(Loss::MarkDataDropped), "MarkDataDropped",
     "One or more marks cannot be represented in the destination and will be removed."},
}};

constexpr std::array<PresentationItem, 5> kAdaptationPresentation{{
    {static_cast<uint32_t>(Adaptation::PidAdjustedForShinyThreshold),
     "PidAdjustedForShinyThreshold",
     "PID representation will be adjusted to preserve shiny status across generation rules."},
    {static_cast<uint32_t>(Adaptation::TargetDefaultTeraSynthesized),
     "TargetDefaultTeraSynthesized",
     "A destination-native default Tera type will be synthesized."},
    {static_cast<uint32_t>(Adaptation::MovePPClamped),
     "MovePPClamped",
     "Move PP will be clamped to the destination game's supported maximum."},
    {static_cast<uint32_t>(Adaptation::Gen3TransferDateSynthesized),
     "Gen3TransferDateSynthesized",
     "A transfer date will be synthesized because the source format does not store one."},
    {static_cast<uint32_t>(Adaptation::DefaultNicknameCanonicalized),
     "DefaultNicknameCanonicalized",
     "The default species name will be canonicalized for the destination text format."},
}};

bool zeroDigest(const Digest& digest) noexcept {
    return std::all_of(digest.begin(), digest.end(), [](uint8_t b) { return b == 0; });
}

bool validTransactionId(std::string_view id) noexcept {
    return id.rfind("tx-", 0) == 0 && id.size() <= 96 && PokeBank::Paths::isSafeComponent(id);
}

bool validGameIdentity(std::string_view id) noexcept {
    return !id.empty() && id.size() <= kMaxStringBytes && PokeBank::Paths::isSafeComponent(id);
}

bool validRelation(ProvenanceRelation relation) noexcept {
    const auto v = static_cast<uint8_t>(relation);
    return v >= static_cast<uint8_t>(ProvenanceRelation::Conversion) &&
           v <= static_cast<uint8_t>(ProvenanceRelation::ArchiveRecovery);
}

bool validResult(EvidenceConversionResult result) noexcept {
    switch (result) {
        case EvidenceConversionResult::Ok:
        case EvidenceConversionResult::SameGroup:
        case EvidenceConversionResult::Unsupported:
        case EvidenceConversionResult::NotInDex:
        case EvidenceConversionResult::Blocked:
        case EvidenceConversionResult::TraitPreservationFailed:
        case EvidenceConversionResult::AbilityNotRepresentable:
        case EvidenceConversionResult::TextNotRepresentable:
        case EvidenceConversionResult::LanguageNotRepresentable:
            return true;
    }
    return false;
}

bool descriptorMatchesGame(const StoreDescriptor& descriptor, const std::string& exactGame) noexcept {
    return descriptor.type == StoreType::Bank || descriptor.gameId == exactGame;
}

bool validateEvidence(const RouteEvidence& e, std::string& error) {
    if (e.schemaVersion != kRouteEvidenceVersion) {
        error = "unsupported conversion-evidence schema";
        return false;
    }
    if (!validTransactionId(e.transactionId)) {
        error = "invalid conversion-evidence transaction id";
        return false;
    }
    if (!validGameIdentity(e.sourceGameIdentity) || !validGameIdentity(e.destinationGameIdentity) ||
        e.sourceGameIdentity == e.destinationGameIdentity) {
        error = "invalid or identical exact game identities";
        return false;
    }
    if (e.sourceFormat == Enums::GameVersion::Invalid ||
        e.destinationFormat == Enums::GameVersion::Invalid) {
        error = "invalid native conversion format";
        return false;
    }
    if (!e.sourceStore.valid() || !e.destinationStore.valid() || e.sourceStore == e.destinationStore ||
        !descriptorMatchesGame(e.sourceStore, e.sourceGameIdentity) ||
        !descriptorMatchesGame(e.destinationStore, e.destinationGameIdentity)) {
        error = "conversion evidence store/game identity mismatch";
        return false;
    }
    if (!validRelation(e.relation) || !validResult(e.conversionResult)) {
        error = "invalid conversion result or provenance relation";
        return false;
    }
    if ((e.fidelity.losses & ~knownLossMask()) != 0 ||
        (e.fidelity.adaptations & ~knownAdaptationMask()) != 0) {
        error = "conversion evidence contains unknown fidelity bits";
        return false;
    }
    if (e.lossesAcknowledged && !e.lossesShownToUser) {
        error = "loss acknowledgement cannot exist before the loss summary was shown";
        return false;
    }
    if (e.lossesAcknowledged && e.acknowledgedAtUnix == 0) {
        error = "acknowledged conversion evidence has no acknowledgement timestamp";
        return false;
    }
    if (!e.lossesAcknowledged && (e.acknowledgedAtUnix != 0 || !zeroDigest(e.acknowledgementBinding))) {
        error = "unacknowledged conversion evidence carries stale acknowledgement state";
        return false;
    }
    return true;
}

void appendU8(std::vector<uint8_t>& out, uint8_t value) { out.push_back(value); }
void appendU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xffu));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xffu));
}
void appendU32(std::vector<uint8_t>& out, uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}
void appendU64(std::vector<uint8_t>& out, uint64_t value) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}
bool appendString(std::vector<uint8_t>& out, const std::string& value) {
    if (value.size() > kMaxStringBytes) return false;
    appendU16(out, static_cast<uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
    return true;
}
void appendDigest(std::vector<uint8_t>& out, const Digest& digest) {
    out.insert(out.end(), digest.begin(), digest.end());
}
bool appendDescriptor(std::vector<uint8_t>& out, const StoreDescriptor& d) {
    appendU8(out, static_cast<uint8_t>(d.type));
    return appendString(out, d.profile) && appendString(out, d.gameId) &&
           appendString(out, d.workspace) && appendString(out, d.fileId);
}
void appendEntity(std::vector<uint8_t>& out, const EntityIdentity& entity) {
    appendU16(out, entity.species);
    appendU8(out, entity.form);
    appendU8(out, 0);
    appendU32(out, entity.pid);
    appendU32(out, entity.encryptionConstant);
}

struct Reader {
    std::span<const uint8_t> bytes;
    size_t pos = 0;

    bool u8(uint8_t& out) {
        if (pos + 1 > bytes.size()) return false;
        out = bytes[pos++];
        return true;
    }
    bool u16(uint16_t& out) {
        if (pos + 2 > bytes.size()) return false;
        out = static_cast<uint16_t>(bytes[pos]) |
              (static_cast<uint16_t>(bytes[pos + 1]) << 8);
        pos += 2;
        return true;
    }
    bool u32(uint32_t& out) {
        if (pos + 4 > bytes.size()) return false;
        out = 0;
        for (int i = 0; i < 4; ++i)
            out |= static_cast<uint32_t>(bytes[pos + static_cast<size_t>(i)]) << (8 * i);
        pos += 4;
        return true;
    }
    bool u64(uint64_t& out) {
        if (pos + 8 > bytes.size()) return false;
        out = 0;
        for (int i = 0; i < 8; ++i)
            out |= static_cast<uint64_t>(bytes[pos + static_cast<size_t>(i)]) << (8 * i);
        pos += 8;
        return true;
    }
    bool str(std::string& out) {
        uint16_t n = 0;
        if (!u16(n) || n > kMaxStringBytes || pos + n > bytes.size()) return false;
        out.assign(reinterpret_cast<const char*>(bytes.data() + pos), n);
        pos += n;
        return true;
    }
    bool digest(Digest& out) {
        if (pos + out.size() > bytes.size()) return false;
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(pos), out.size(), out.begin());
        pos += out.size();
        return true;
    }
};

bool readDescriptor(Reader& r, StoreDescriptor& d) {
    uint8_t type = 0;
    if (!r.u8(type) ||
        type < static_cast<uint8_t>(StoreType::Bank) ||
        type > static_cast<uint8_t>(StoreType::MutableWorkspaceFileSet)) return false;
    d.type = static_cast<StoreType>(type);
    return r.str(d.profile) && r.str(d.gameId) && r.str(d.workspace) && r.str(d.fileId);
}

bool readEntity(Reader& r, EntityIdentity& entity) {
    uint8_t pad = 0;
    return r.u16(entity.species) && r.u8(entity.form) && r.u8(pad) && pad == 0 &&
           r.u32(entity.pid) && r.u32(entity.encryptionConstant);
}

std::vector<uint8_t> bindingBytes(const RouteEvidence& e) {
    std::vector<uint8_t> out;
    out.insert(out.end(), kBindingMagic.begin(), kBindingMagic.end());
    appendU16(out, e.schemaVersion);
    appendString(out, e.transactionId);
    appendString(out, e.sourceGameIdentity);
    appendString(out, e.destinationGameIdentity);
    appendU16(out, static_cast<uint16_t>(e.sourceFormat));
    appendU16(out, static_cast<uint16_t>(e.destinationFormat));
    appendU8(out, e.historicalOriginVersion);
    appendU8(out, static_cast<uint8_t>(e.relation));
    appendDescriptor(out, e.sourceStore);
    appendDescriptor(out, e.destinationStore);
    appendDigest(out, e.sourcePayload);
    appendDigest(out, e.destinationPayload);
    appendEntity(out, e.sourceEntity);
    appendEntity(out, e.destinationEntity);
    appendU8(out, static_cast<uint8_t>(e.conversionResult));
    appendU8(out, e.candidateAvailable ? 1u : 0u);
    appendU32(out, e.fidelity.losses);
    appendU32(out, e.fidelity.adaptations);
    appendU8(out, e.fidelity.sourceOriginVersion);
    appendU8(out, e.fidelity.destinationEntityOriginVersion);
    return out;
}

std::vector<uint8_t> serializeEvidence(const RouteEvidence& e, std::string& error) {
    if (!validateEvidence(e, error)) return {};

    std::vector<uint8_t> out;
    out.reserve(512);
    out.insert(out.end(), kEvidenceMagic.begin(), kEvidenceMagic.end());
    appendU16(out, e.schemaVersion);
    appendU8(out, static_cast<uint8_t>(e.relation));

    uint8_t flags = 0;
    if (e.candidateAvailable) flags |= 0x01u;
    if (e.lossesShownToUser) flags |= 0x02u;
    if (e.lossesAcknowledged) flags |= 0x04u;
    appendU8(out, flags);

    if (!appendString(out, e.transactionId) ||
        !appendString(out, e.sourceGameIdentity) ||
        !appendString(out, e.destinationGameIdentity)) {
        error = "conversion evidence text exceeds supported size";
        return {};
    }

    appendU16(out, static_cast<uint16_t>(e.sourceFormat));
    appendU16(out, static_cast<uint16_t>(e.destinationFormat));
    appendU8(out, e.historicalOriginVersion);
    appendU8(out, static_cast<uint8_t>(e.conversionResult));
    appendU16(out, 0);

    if (!appendDescriptor(out, e.sourceStore) || !appendDescriptor(out, e.destinationStore)) {
        error = "conversion evidence store descriptor exceeds supported size";
        return {};
    }

    appendDigest(out, e.sourcePayload);
    appendDigest(out, e.destinationPayload);
    appendEntity(out, e.sourceEntity);
    appendEntity(out, e.destinationEntity);

    appendU32(out, e.fidelity.losses);
    appendU32(out, e.fidelity.adaptations);
    appendU8(out, e.fidelity.sourceOriginVersion);
    appendU8(out, e.fidelity.destinationEntityOriginVersion);
    appendU16(out, 0);

    appendU64(out, e.createdAtUnix);
    appendU64(out, e.acknowledgedAtUnix);
    appendDigest(out, e.acknowledgementBinding);

    const Digest integrity = PokeBank::Storage::MoveTx::sha256(out);
    appendDigest(out, integrity);
    if (out.size() > kMaxEvidenceBytes) {
        error = "conversion evidence exceeds maximum supported size";
        return {};
    }
    return out;
}

EvidenceLoadResult parseEvidence(std::span<const uint8_t> bytes) {
    EvidenceLoadResult result;
    result.status = EvidenceLoadStatus::Corrupt;

    if (bytes.size() < kEvidenceMagic.size() + 2 + 32) {
        result.error = "conversion evidence is truncated";
        return result;
    }
    if (!std::equal(kEvidenceMagic.begin(), kEvidenceMagic.end(), bytes.begin())) {
        result.error = "conversion evidence magic mismatch";
        return result;
    }

    const uint16_t version = static_cast<uint16_t>(bytes[8]) |
                             (static_cast<uint16_t>(bytes[9]) << 8);
    if (version != kRouteEvidenceVersion) {
        result.status = EvidenceLoadStatus::UnsupportedVersion;
        result.error = "conversion evidence schema is newer/unsupported";
        return result;
    }
    if (bytes.size() > kMaxEvidenceBytes) {
        result.error = "conversion evidence size is invalid";
        return result;
    }

    const size_t bodySize = bytes.size() - 32;
    Digest storedIntegrity{};
    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(bodySize),
                storedIntegrity.size(), storedIntegrity.begin());
    if (PokeBank::Storage::MoveTx::sha256(bytes.first(bodySize)) != storedIntegrity) {
        result.error = "conversion evidence integrity hash mismatch";
        return result;
    }

    Reader r{bytes.first(bodySize)};
    r.pos = 10;
    RouteEvidence e;
    e.schemaVersion = version;
    uint8_t relation = 0, flags = 0;
    uint16_t sourceFormat = 0, destinationFormat = 0, reserved = 0;
    uint8_t conversionResult = 0;
    if (!r.u8(relation) || !r.u8(flags) || (flags & ~0x07u) != 0 ||
        !r.str(e.transactionId) || !r.str(e.sourceGameIdentity) ||
        !r.str(e.destinationGameIdentity) ||
        !r.u16(sourceFormat) || !r.u16(destinationFormat) ||
        !r.u8(e.historicalOriginVersion) || !r.u8(conversionResult) ||
        !r.u16(reserved) || reserved != 0 ||
        !readDescriptor(r, e.sourceStore) || !readDescriptor(r, e.destinationStore) ||
        !r.digest(e.sourcePayload) || !r.digest(e.destinationPayload) ||
        !readEntity(r, e.sourceEntity) || !readEntity(r, e.destinationEntity) ||
        !r.u32(e.fidelity.losses) || !r.u32(e.fidelity.adaptations) ||
        !r.u8(e.fidelity.sourceOriginVersion) ||
        !r.u8(e.fidelity.destinationEntityOriginVersion) ||
        !r.u16(reserved) || reserved != 0 ||
        !r.u64(e.createdAtUnix) || !r.u64(e.acknowledgedAtUnix) ||
        !r.digest(e.acknowledgementBinding)) {
        result.error = "conversion evidence payload is malformed";
        return result;
    }
    if (r.pos != bodySize) {
        result.error = "conversion evidence has trailing or unparsed data";
        return result;
    }

    e.relation = static_cast<ProvenanceRelation>(relation);
    e.sourceFormat = static_cast<Enums::GameVersion>(sourceFormat);
    e.destinationFormat = static_cast<Enums::GameVersion>(destinationFormat);
    e.conversionResult = static_cast<EvidenceConversionResult>(conversionResult);
    e.candidateAvailable = (flags & 0x01u) != 0;
    e.lossesShownToUser = (flags & 0x02u) != 0;
    e.lossesAcknowledged = (flags & 0x04u) != 0;

    std::string validationError;
    if (!validateEvidence(e, validationError)) {
        result.error = validationError;
        return result;
    }

    result.status = EvidenceLoadStatus::Ok;
    result.evidence = std::move(e);
    return result;
}

bool readFile(const std::string& path, std::vector<uint8_t>& out, std::string& error) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        error = "open failed: " + std::string(std::strerror(errno));
        return false;
    }
    if (std::fseek(f, 0, SEEK_END) != 0) {
        error = "seek failed";
        std::fclose(f);
        return false;
    }
    const long size = std::ftell(f);
    if (size < 0 || static_cast<size_t>(size) > kMaxEvidenceBytes ||
        std::fseek(f, 0, SEEK_SET) != 0) {
        error = "conversion evidence file size is invalid";
        std::fclose(f);
        return false;
    }
    out.assign(static_cast<size_t>(size), 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool closeOk = std::fclose(f) == 0;
    if (got != out.size() || !closeOk) {
        error = "conversion evidence read/close failed";
        return false;
    }
    return true;
}

} // namespace

std::span<const PresentationItem> lossPresentationCatalog() noexcept {
    return kLossPresentation;
}

std::span<const PresentationItem> adaptationPresentationCatalog() noexcept {
    return kAdaptationPresentation;
}

uint32_t knownLossMask() noexcept {
    uint32_t mask = 0;
    for (const auto& item : kLossPresentation) mask |= item.bit;
    return mask;
}

uint32_t knownAdaptationMask() noexcept {
    uint32_t mask = 0;
    for (const auto& item : kAdaptationPresentation) mask |= item.bit;
    return mask;
}

FidelitySummary summarizeFidelity(const Report& report) {
    FidelitySummary summary;
    for (const auto& item : kLossPresentation)
        if ((report.losses & item.bit) != 0) summary.losses.emplace_back(item.message);
    for (const auto& item : kAdaptationPresentation)
        if ((report.adaptations & item.bit) != 0) summary.adaptations.emplace_back(item.message);
    summary.unknownLossBits = (report.losses & ~knownLossMask()) != 0;
    summary.unknownAdaptationBits = (report.adaptations & ~knownAdaptationMask()) != 0;
    return summary;
}

Digest computeAcknowledgementBinding(const RouteEvidence& evidence) {
    const auto bytes = bindingBytes(evidence);
    return PokeBank::Storage::MoveTx::sha256(bytes);
}

bool markLossesAcknowledged(RouteEvidence& evidence,
                            uint64_t acknowledgedAtUnix,
                            std::string& error) {
    if (!evidence.candidateAvailable || evidence.conversionResult != EvidenceConversionResult::Ok) {
        error = "cannot acknowledge a failed or unavailable conversion candidate";
        return false;
    }
    if ((evidence.fidelity.losses & ~knownLossMask()) != 0 ||
        (evidence.fidelity.adaptations & ~knownAdaptationMask()) != 0) {
        error = "cannot acknowledge unknown fidelity semantics";
        return false;
    }
    if (evidence.requiresLossAcknowledgement() && !evidence.lossesShownToUser) {
        error = "cannot acknowledge declared losses before the loss summary was shown";
        return false;
    }
    if (acknowledgedAtUnix == 0 ||
        (evidence.createdAtUnix != 0 && acknowledgedAtUnix < evidence.createdAtUnix)) {
        error = "invalid acknowledgement timestamp";
        return false;
    }
    evidence.lossesAcknowledged = true;
    evidence.acknowledgedAtUnix = acknowledgedAtUnix;
    evidence.acknowledgementBinding = computeAcknowledgementBinding(evidence);
    return true;
}

EvidenceStore::EvidenceStore()
    : EvidenceStore(PokeBank::Paths::transactionsRoot()) {}

EvidenceStore::EvidenceStore(std::string transactionRoot)
    : root_(std::move(transactionRoot)), recordsRoot_(root_ + "/records") {}

bool EvidenceStore::ensure(std::string& error) const {
    return PokeBank::Paths::ensureDirectoryTree(recordsRoot_, &error);
}

std::string EvidenceStore::evidencePath(const std::string& transactionId) const {
    return validTransactionId(transactionId)
        ? recordsRoot_ + "/" + transactionId + ".conversion-evidence.pbce"
        : std::string{};
}

bool EvidenceStore::persist(const RouteEvidence& evidence, std::string& error) const {
    if (!ensure(error)) return false;
    const std::string path = evidencePath(evidence.transactionId);
    if (path.empty()) {
        error = "invalid conversion-evidence transaction id";
        return false;
    }

    auto bytes = serializeEvidence(evidence, error);
    if (bytes.empty()) return false;

    const auto validator = [&](std::span<const uint8_t> candidate, std::string& validationError) {
        const auto loaded = parseEvidence(candidate);
        if (loaded.status != EvidenceLoadStatus::Ok ||
            loaded.evidence.transactionId != evidence.transactionId) {
            validationError = loaded.error.empty()
                ? "conversion evidence round-trip mismatch"
                : loaded.error;
            return false;
        }
        return true;
    };

    const auto durable = PokeBank::Storage::DurableFile::replace(path, bytes, validator);
    if (!durable.ok) {
        error = durable.error;
        return false;
    }
    return true;
}

EvidenceLoadResult EvidenceStore::load(const std::string& transactionId) const {
    EvidenceLoadResult result;
    const std::string path = evidencePath(transactionId);
    if (path.empty()) {
        result.status = EvidenceLoadStatus::Corrupt;
        result.error = "invalid conversion-evidence transaction id";
        return result;
    }

    struct stat st{};
    errno = 0;
    if (::stat(path.c_str(), &st) != 0) {
        result.status = errno == ENOENT ? EvidenceLoadStatus::NotFound : EvidenceLoadStatus::Corrupt;
        result.error = errno == ENOENT ? "conversion evidence not found" : "conversion evidence stat failed";
        return result;
    }

    std::vector<uint8_t> bytes;
    if (!readFile(path, bytes, result.error)) {
        result.status = EvidenceLoadStatus::Corrupt;
        return result;
    }
    return parseEvidence(bytes);
}

RetirementAuthorization authorizeSourceRetirement(
    const RouteEvidence& evidence,
    const Transaction& transaction,
    bool routeEnabled) {
    RetirementAuthorization out;

    std::string validationError;
    if (!validateEvidence(evidence, validationError)) {
        out.reason = validationError;
        return out;
    }
    if (!transaction.crossGameConversion) {
        out.reason = "transaction is not marked as a cross-game conversion";
        return out;
    }
    if (transaction.id != evidence.transactionId ||
        transaction.source != evidence.sourceStore ||
        transaction.destination != evidence.destinationStore) {
        out.reason = "conversion evidence does not match transaction identity/stores";
        return out;
    }
    if (!evidence.candidateAvailable || evidence.conversionResult != EvidenceConversionResult::Ok) {
        out.reason = "conversion candidate is unavailable or failed";
        return out;
    }
    if (evidence.relation != ProvenanceRelation::Conversion) {
        out.reason = "conversion evidence relation is not CONVERSION";
        return out;
    }
    if ((evidence.fidelity.losses & ~knownLossMask()) != 0 ||
        (evidence.fidelity.adaptations & ~knownAdaptationMask()) != 0) {
        out.reason = "conversion evidence contains unrecognized fidelity semantics";
        return out;
    }

    size_t matchingMoves = 0;
    for (const auto& move : transaction.moves) {
        if (move.sourcePayload == evidence.sourcePayload &&
            move.destinationPayload == evidence.destinationPayload &&
            move.species == evidence.sourceEntity.species &&
            move.form == evidence.sourceEntity.form) {
            ++matchingMoves;
        }
    }
    if (matchingMoves != 1) {
        out.reason = matchingMoves == 0
            ? "conversion evidence payload hashes do not match the transaction move"
            : "conversion evidence matches multiple transaction moves ambiguously";
        return out;
    }
    if (evidence.destinationEntity.species != evidence.sourceEntity.species) {
        out.reason = "conversion evidence changes Pokemon species identity";
        return out;
    }

    if (evidence.requiresLossAcknowledgement()) {
        if (!evidence.lossesShownToUser || !evidence.lossesAcknowledged) {
            out.reason = "declared losses were not explicitly acknowledged";
            return out;
        }
        if (evidence.acknowledgedAtUnix == 0 ||
            evidence.acknowledgementBinding != computeAcknowledgementBinding(evidence)) {
            out.reason = "loss acknowledgement is stale or does not match the exact candidate";
            return out;
        }
    } else if (evidence.lossesAcknowledged &&
               evidence.acknowledgementBinding != computeAcknowledgementBinding(evidence)) {
        out.reason = "acknowledgement binding does not match the exact candidate";
        return out;
    }

    if (!routeEnabled) {
        out.reason = "cross-game route is disabled by product policy";
        return out;
    }

    out.allowed = true;
    return out;
}

bool routeEnabledForTrueMove(const RouteEvidence&) noexcept {
    return false;
}

PokeBank::Storage::MoveTx::RetirementGate makeEvidenceRetirementGate(
    EvidenceStore store,
    bool routeEnabled) {
    return [store = std::move(store), routeEnabled](
               const Transaction& transaction,
               std::string& error) mutable {
        const auto loaded = store.load(transaction.id);
        if (loaded.status != EvidenceLoadStatus::Ok) {
            switch (loaded.status) {
                case EvidenceLoadStatus::NotFound:
                    error = "required conversion evidence is missing";
                    break;
                case EvidenceLoadStatus::UnsupportedVersion:
                    error = "conversion evidence version is unsupported";
                    break;
                case EvidenceLoadStatus::Corrupt:
                    error = "conversion evidence is corrupt: " + loaded.error;
                    break;
                case EvidenceLoadStatus::Ok:
                    break;
            }
            return false;
        }

        const auto auth = authorizeSourceRetirement(loaded.evidence, transaction, routeEnabled);
        if (!auth.allowed) {
            error = auth.reason;
            return false;
        }
        return true;
    };
}

ProvenanceNode makeProvenanceNode(const RouteEvidence& evidence, uint64_t sequence) {
    ProvenanceNode node;
    node.sequence = sequence;
    node.relation = evidence.relation;
    node.transactionId = evidence.transactionId;
    node.historicalOriginVersion = evidence.historicalOriginVersion;
    node.sourceGameIdentity = evidence.sourceGameIdentity;
    node.destinationGameIdentity = evidence.destinationGameIdentity;
    node.sourceStore = evidence.sourceStore;
    node.destinationStore = evidence.destinationStore;
    node.sourcePayload = evidence.sourcePayload;
    node.destinationPayload = evidence.destinationPayload;
    node.fidelity = evidence.fidelity;
    return node;
}

const char* provenanceRelationName(ProvenanceRelation relation) noexcept {
    switch (relation) {
        case ProvenanceRelation::Conversion: return "CONVERSION";
        case ProvenanceRelation::Move: return "MOVE";
        case ProvenanceRelation::Copy: return "COPY";
        case ProvenanceRelation::ExactCloneOf: return "EXACT_CLONE_OF";
        case ProvenanceRelation::DerivedFrom: return "DERIVED_FROM";
        case ProvenanceRelation::ArchiveRecovery: return "ARCHIVE_RECOVERY";
    }
    return "UNKNOWN";
}

} // namespace Conversion
