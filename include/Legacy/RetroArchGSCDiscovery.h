#ifndef POKEBANK_LEGACY_RETROARCH_GSC_DISCOVERY_H
#define POKEBANK_LEGACY_RETROARCH_GSC_DISCOVERY_H

#include "Integration/Gen2/Gen2ReadOnlySave.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Legacy {

enum class GSCSourceStatus : unsigned char {
    Ready,
    InvalidSave,
    AmbiguousIdentity,
    ReadError,
    ScanLimitReached,
};

struct GSCScanLimits {
    std::size_t maxDepth = 2;
    std::size_t maxFiles = 256;
};

struct GSCSource {
    std::string path;
    std::string normalizedPath;
    std::string sourceIdentity;
    std::string canonicalPath;
    uint64_t fileSize = 0;
    int64_t modifiedTime = 0;
    std::string contentFingerprint;
    std::string gameId;
    GSCSourceStatus status = GSCSourceStatus::ReadError;
    Integration::Gen2::SaveError parseError = Integration::Gen2::SaveError::None;
    std::string detail;
    std::shared_ptr<const Integration::Gen2::ReadOnlySave> save;

    [[nodiscard]] bool ready() const noexcept {
        return status == GSCSourceStatus::Ready && save != nullptr;
    }
};

struct GSCDiscoveryResult {
    enum class RootKind : unsigned char { None, Configured, ConventionalFallback };
    std::vector<GSCSource> sources;
    std::size_t filesExamined = 0;
    bool limitReached = false;
    std::string activeRoot;
    RootKind activeRootKind = RootKind::None;
};

[[nodiscard]] std::vector<std::string> retroArchGSCSaveRootsFromConfig(
    const std::string& configPath);

[[nodiscard]] GSCDiscoveryResult discoverGSCSaves(
    std::span<const std::string> approvedRoots, GSCScanLimits limits = {});

[[nodiscard]] GSCDiscoveryResult discoverConfiguredRetroArchGSCSaves(
    GSCScanLimits limits = {},
    const std::string& configPath = "sdmc:/retroarch/retroarch.cfg",
    const std::string& conventionalRoot = "sdmc:/retroarch/cores/savefiles");

} // namespace PokeVault::Legacy

#endif
