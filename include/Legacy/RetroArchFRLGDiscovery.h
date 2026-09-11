#ifndef POKEBANK_LEGACY_RETROARCH_FRLG_DISCOVERY_H
#define POKEBANK_LEGACY_RETROARCH_FRLG_DISCOVERY_H

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
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

    // Historical type/API name retained for compatibility. The UI-facing legacy catalog now
    // represents validated read-only RetroArch Gen I R/B/Y, Gen II G/S/C, and Gen III R/S/E/FR/LG
    // battery saves. Parsing remains generation-specific: exactly one strongly typed save pointer
    // is populated for a ready source, preventing one generation's layout assumptions from leaking
    // into another while the accepted Game Sources presentation flow stays shared.
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
        Integration::Gen1::SaveError gen1ParseError = Integration::Gen1::SaveError::None;
        Integration::Gen2::SaveError gen2ParseError = Integration::Gen2::SaveError::None;
        std::string detail;
        std::unique_ptr<Integration::Gen3::ReadOnlySave> save;
        std::shared_ptr<const Integration::Gen1::ReadOnlySave> gen1Save;
        std::shared_ptr<const Integration::Gen2::ReadOnlySave> gen2Save;

        [[nodiscard]] bool ready() const noexcept {
            const int count = static_cast<int>(save != nullptr) +
                              static_cast<int>(gen1Save != nullptr) +
                              static_cast<int>(gen2Save != nullptr);
            return status == LegacySourceStatus::Ready && count == 1;
        }
        [[nodiscard]] bool isGen1() const noexcept { return gen1Save != nullptr; }
        [[nodiscard]] bool isGen2() const noexcept { return gen2Save != nullptr; }
        [[nodiscard]] bool isGen3() const noexcept { return save != nullptr; }
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

    [[nodiscard]] FRLGDiscoveryResult discoverFRLGSaves(
        std::span<const std::string> approvedRoots, ScanLimits limits = {});

    // Runtime legacy catalog for the normal Game Sources UI. The already accepted Gen I and Gen III
    // scanners remain generation-specific and unchanged; strict Gen II discovery is appended from
    // the same approved configured/fallback RetroArch battery-save root.
    [[nodiscard]] FRLGDiscoveryResult discoverConfiguredRetroArchFRLGSaves(
        ScanLimits limits = {},
        const std::string& configPath = "sdmc:/retroarch/retroarch.cfg",
        const std::string& conventionalRoot = "sdmc:/retroarch/cores/savefiles");
}

#endif
