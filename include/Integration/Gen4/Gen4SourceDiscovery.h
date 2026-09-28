#ifndef POKEBANK_INTEGRATION_GEN4_SOURCE_DISCOVERY_H
#define POKEBANK_INTEGRATION_GEN4_SOURCE_DISCOVERY_H

#include "Enums/GameVersion.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen4 {

enum class CandidateStatus : uint8_t {
    Ready,
    InvalidSave,
    UnsupportedWrapper,
    UnsupportedSavestate,
    ReadError,
    AssignmentMismatch,
};

struct DiscoveryRoot {
    std::string path;
    std::string sourceType;
    size_t maxDepth = 2;
};

struct DiscoveryLimits {
    size_t maxFiles = 256;
};

struct SourceCandidate {
    CandidateStatus status = CandidateStatus::ReadError;
    std::string path;
    std::string normalizedPath;
    std::string sourceIdentity;
    std::string sourceType;
    std::string expectedRawFamily;
    uint64_t fileSize = 0;
    int64_t modifiedTime = 0;
    Layout layout = Layout::DiamondPearl;
    Enums::GameVersion exactGameFromSave = Enums::GameVersion::Invalid;
    bool recoveredOlderCopy = false;
    std::string trainerName;
    uint8_t partyCount = 0;
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept { return status == CandidateStatus::Ready; }
};

struct DiscoveryResult {
    std::vector<SourceCandidate> candidates;
    size_t filesExamined = 0;
    bool limitReached = false;
};

enum class SourceContainerKind : uint8_t {
    Raw,
    DsvFooter,
};

enum class SourcePayloadStatus : uint8_t {
    Ready,
    InvalidSize,
    UnsupportedWrapper,
    ReadError,
};

struct SourcePayloadRead {
    SourcePayloadStatus status = SourcePayloadStatus::ReadError;
    SourceContainerKind kind = SourceContainerKind::Raw;
    std::vector<uint8_t> bytes;
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept { return status == SourcePayloadStatus::Ready; }
};

// Read-only container adapter shared by discovery and assigned-source open.
// Supported:
// - exact 0x80000 raw payloads
// - documented DeSmuME-compatible .dsv footer containers where padded_size == 0x80000
// The source file is never modified, truncated, normalized or rewritten.
[[nodiscard]] SourcePayloadRead readSourcePayloadReadOnly(const std::string& path);

[[nodiscard]] SourceCandidate inspectSourceFile(
    const std::string& path,
    std::string_view sourceType,
    std::string_view assignedGameId = {});

[[nodiscard]] DiscoveryResult discoverSources(
    std::span<const DiscoveryRoot> roots,
    DiscoveryLimits limits = {});

// Centralized provider roots. Cartridge backups are candidates; savestate roots are scanned only
// so .dss can be diagnosed explicitly and are never eligible for assignment.
[[nodiscard]] std::vector<DiscoveryRoot> defaultDraSticRoots();

// Explicitly bounded known-location discovery. This never scans sdmc:/ itself.
// RetroArch roots come from savefile_directory (or its conventional savefiles folder);
// DraStic and melonDS use only their known app roots. DraStic savestate roots are diagnostics-only.
 // Manual selection is separate.
[[nodiscard]] DiscoveryResult discoverKnownSources(
    DiscoveryLimits limits = {},
    const std::string& retroArchConfig = "sdmc:/retroarch/retroarch.cfg",
    const std::string& retroArchFallback = "sdmc:/retroarch/cores/savefiles");

[[nodiscard]] bool candidateMatchesGame(
    const SourceCandidate& candidate, std::string_view gameId) noexcept;

[[nodiscard]] const char* candidateStatusName(CandidateStatus status) noexcept;

}
#endif
