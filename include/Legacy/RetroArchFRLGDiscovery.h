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

    // Historical type/API name retained for compatibility. The UI-facing legacy catalog represents
    // validated read-only RetroArch Gen I R/B/Y, Gen II G/S/C, and Gen III R/S/E/FR/LG battery
    // saves. Parsing remains generation-specific: exactly one typed save pointer is populated for a
    // ready source. That prevents structural assumptions from leaking between generations while the
    // accepted Game Sources presentation/profile-binding flow remains shared.
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
            const unsigned typed = (save ? 1u : 0u) + (gen1Save ? 1u : 0u) +
                                   (gen2Save ? 1u : 0u);
            return status == LegacySourceStatus::Ready && typed == 1u;
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

    // Bounded read-only Gen III traversal. Only .sav/.srm raw battery saves are considered.
    // Arbitrary RetroArch savestates are never parsed. Filename release hints are accepted only
    // after strict family validation; Ruby/Sapphire exact identity necessarily comes from source
    // evidence. Gen I and Gen II discovery remain separate strict implementations and are merged
    // only by the configured runtime catalog function below.
    [[nodiscard]] FRLGDiscoveryResult discoverFRLGSaves(
        std::span<const std::string> approvedRoots, ScanLimits limits = {});

    // Runtime legacy catalog for the normal Game Sources UI. It preserves the accepted Gen III
    // discovery behavior and appends separately validated R/B/Y and G/S/C results from the same
    // approved configured/fallback RetroArch battery-save root. All scanners remain independently
    // bounded and no generation is allowed to reinterpret another generation's payload.
    [[nodiscard]] FRLGDiscoveryResult discoverConfiguredRetroArchFRLGSaves(
        ScanLimits limits = {},
        const std::string& configPath = "sdmc:/retroarch/retroarch.cfg",
        const std::string& conventionalRoot = "sdmc:/retroarch/cores/savefiles");
}

#endif
