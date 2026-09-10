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
        Ready, InvalidSave, AmbiguousIdentity, ReadError, ScanLimitReached,
    };

    struct ScanLimits { size_t maxDepth = 2; size_t maxFiles = 256; };

    // Historical type/API name retained for compatibility; this now represents all read-only
    // RetroArch Gen III GBA sources (R/S/E/FR/LG), not only FRLG.
    struct FRLGSource {
        std::string path;
        std::string normalizedPath;
        std::string sourceIdentity;
        std::string canonicalPath;
        uint64_t fileSize = 0;
        int64_t modifiedTime = 0;
        std::string contentFingerprint;
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

    [[nodiscard]] std::vector<std::string> retroArchSaveRootsFromConfig(
        const std::string& configPath);

    // Bounded read-only traversal. Only .sav/.srm raw battery saves are considered. Arbitrary
    // RetroArch savestates are never parsed. Filename release hints are accepted only after strict
    // Gen III family validation; Ruby/Sapphire exact identity necessarily comes from that source.
    [[nodiscard]] FRLGDiscoveryResult discoverFRLGSaves(
        std::span<const std::string> approvedRoots, ScanLimits limits = {});

    [[nodiscard]] FRLGDiscoveryResult discoverConfiguredRetroArchFRLGSaves(
        ScanLimits limits = {},
        const std::string& configPath = "sdmc:/retroarch/retroarch.cfg",
        const std::string& conventionalRoot = "sdmc:/retroarch/cores/savefiles");
}

#endif
