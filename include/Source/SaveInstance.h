#ifndef POKEBANK_SOURCE_SAVE_INSTANCE_H
#define POKEBANK_SOURCE_SAVE_INSTANCE_H

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Source {

// Provider-neutral presentation metadata for one validated physical save. Generation-specific
// parsers keep their own native result types; this model is the boundary handed to Save Instances,
// profile visibility, source details, deduplication and ordering.
enum class SaveInstanceKind : uint8_t {
    BatterySave,
    SaveState,
    Backup,
    ManualImport,
};

enum class ValidationStatus : uint8_t {
    Unknown,
    Ready,
    Invalid,
    Unsupported,
    Missing,
    ReadError,
    AssignmentMismatch,
};

enum class AccessMode : uint8_t {
    ReadOnly,
    StagedWorkspace,
};

enum class DiagnosticState : uint8_t {
    None,
    RecoveredOlderCopy,
};

struct SaveInstance {
    // The first fields intentionally preserve the historical classic aggregate layout. They are
    // also useful provider-neutral UI fields, so older strict parser adapters can migrate without
    // changing their parsing semantics.
    size_t sourceIndex = 0; // Opaque generation-specific validation/catalog handle.
    SaveInstanceKind kind = SaveInstanceKind::BatterySave;
    std::string label;      // Friendly filename/source name.
    std::string providerLabel;
    std::string sourceLabel;
    std::string location;   // Historical alias for sourcePath.
    std::string normalizedPath;
    std::string sourceIdentity;
    std::string contentFingerprint;
    std::string trainerName;
    uint64_t fileSize = 0;
    int64_t modifiedTime = 0;
    size_t partyCount = 0;
    bool mostRecentlyModified = false;

    // Common source-instance contract.
    std::string gameId;
    uint8_t generation = 0;
    std::string platformLabel;
    std::string providerId;
    std::string sourcePath;
    std::string physicalIdentity;
    std::string containerType;
    ValidationStatus validation = ValidationStatus::Unknown;
    AccessMode access = AccessMode::ReadOnly;
    DiagnosticState diagnosticState = DiagnosticState::None;
    bool rememberedSource = false;
    std::string claimedProfile;
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept {
        return validation == ValidationStatus::Ready;
    }
    [[nodiscard]] bool readOnly() const noexcept {
        return access == AccessMode::ReadOnly;
    }
    [[nodiscard]] const std::string& path() const noexcept {
        return sourcePath.empty() ? location : sourcePath;
    }
};

[[nodiscard]] inline std::string providerIdFor(std::string_view label) {
    std::string out;
    out.reserve(label.size());
    bool dash = false;
    for (unsigned char ch : label) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
            dash = false;
        } else if (!out.empty() && !dash) {
            out.push_back('-');
            dash = true;
        }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out.empty() ? std::string("source") : out;
}

[[nodiscard]] inline std::string_view physicalKey(const SaveInstance& instance) noexcept {
    if (!instance.physicalIdentity.empty()) return instance.physicalIdentity;
    if (!instance.normalizedPath.empty()) return instance.normalizedPath;
    if (!instance.sourcePath.empty()) return instance.sourcePath;
    return instance.location;
}

[[nodiscard]] inline bool samePhysicalSource(
    const SaveInstance& left, const SaveInstance& right) noexcept {
    const auto a = physicalKey(left);
    const auto b = physicalKey(right);
    return !a.empty() && !b.empty() && a == b;
}

// Returns false when the same physical file is already represented. Provider labels never create a
// second row for one physical file discovered through overlapping roots.
inline bool appendDeduplicated(std::vector<SaveInstance>& instances, SaveInstance instance) {
    if (std::any_of(instances.begin(), instances.end(),
            [&](const auto& existing) { return samePhysicalSource(existing, instance); }))
        return false;
    instances.push_back(std::move(instance));
    return true;
}

// One shared ordering rule for every generation: trustworthy physical mtime first, then stable
// provider/path/identity tie-breaks. The first row alone receives the recency badge.
inline void sortNewestFirst(std::vector<SaveInstance>& instances) {
    std::sort(instances.begin(), instances.end(), [](const auto& left, const auto& right) {
        const bool leftHasTime = left.modifiedTime > 0;
        const bool rightHasTime = right.modifiedTime > 0;
        if (leftHasTime != rightHasTime) return leftHasTime;
        if (left.modifiedTime != right.modifiedTime) return left.modifiedTime > right.modifiedTime;
        if (left.providerId != right.providerId) return left.providerId < right.providerId;
        if (left.normalizedPath != right.normalizedPath)
            return left.normalizedPath < right.normalizedPath;
        return left.sourceIdentity < right.sourceIdentity;
    });
    for (auto& instance : instances) instance.mostRecentlyModified = false;
    if (!instances.empty() && instances.front().modifiedTime > 0)
        instances.front().mostRecentlyModified = true;
}

[[nodiscard]] inline bool visibleToProfile(
    const SaveInstance& instance, std::string_view profileIdentity) noexcept {
    return instance.claimedProfile.empty() || instance.claimedProfile == profileIdentity;
}

} // namespace PokeVault::Source

#endif
