#ifndef POKEBANK_LEGACY_RETROARCH_FRLG_DISCOVERY_H
#define POKEBANK_LEGACY_RETROARCH_FRLG_DISCOVERY_H

#include "Integration/Gen3/PKSMGen3Adapter.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Legacy {
    enum class LegacySourceStatus : unsigned char {
        Ready,
        InvalidSave,
        AmbiguousIdentity,
        ReadError,
        ScanLimitReached,
    };

    struct ScanLimits {
        size_t maxDepth = 2;
        size_t maxFiles = 256;
    };

    struct FRLGSource {
        std::string path;
        std::string normalizedPath;
        // Canonical filesystem identity used only to collapse aliases of this same file. Two
        // separately stored saves remain distinct even when their bytes happen to match.
        std::string canonicalPath;
        uint64_t fileSize = 0;
        int64_t modifiedTime = 0;
        std::string gameId;
        LegacySourceStatus status = LegacySourceStatus::ReadError;
        Integration::Gen3::SaveError parseError = Integration::Gen3::SaveError::None;
        std::string detail;
        std::unique_ptr<Integration::Gen3::ReadOnlySave> save;

        [[nodiscard]] bool ready() const noexcept {
            return status == LegacySourceStatus::Ready && save != nullptr;
        }
    };

    struct FRLGDiscoveryResult {
        enum class RootKind : unsigned char { None, Configured, ConventionalFallback };
        std::vector<FRLGSource> sources;
        size_t filesExamined = 0;
        bool limitReached = false;
        std::string activeRoot;
        RootKind activeRootKind = RootKind::None;
    };

    // Reads only savefile_directory from a RetroArch configuration. Relative values are
    // resolved beside the config file; "default" and missing/unreadable configs yield no root.
    [[nodiscard]] std::vector<std::string> retroArchSaveRootsFromConfig(
        const std::string& configPath);

    // Bounded read-only traversal. Only .sav/.srm files are considered. Unrelated invalid files
    // are suppressed; a filename/path hint is accepted only after strict FRLG-family validation.
    [[nodiscard]] FRLGDiscoveryResult discoverFRLGSaves(
        std::span<const std::string> approvedRoots, ScanLimits limits = {});

    // Switch convenience path: configured RetroArch root, plus the conventional save root only
    // when it actually exists. It never recursively scans sdmc:/ as a whole.
    [[nodiscard]] FRLGDiscoveryResult discoverConfiguredRetroArchFRLGSaves(
        ScanLimits limits = {},
        const std::string& configPath = "sdmc:/retroarch/retroarch.cfg",
        const std::string& conventionalRoot = "sdmc:/retroarch/cores/savefiles");
}

#endif
