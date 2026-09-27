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

[[nodiscard]] SourceCandidate inspectSourceFile(
    const std::string& path,
    std::string_view sourceType,
    std::string_view assignedGameId = {});

[[nodiscard]] DiscoveryResult discoverSources(
    std::span<const DiscoveryRoot> roots,
    DiscoveryLimits limits = {});

// Explicitly bounded known-location discovery. This never scans sdmc:/ itself.
// RetroArch roots come from savefile_directory (or its conventional savefiles folder);
// DraStic and melonDS use only their known app roots. Manual selection is separate.
[[nodiscard]] DiscoveryResult discoverKnownSources(
    DiscoveryLimits limits = {},
    const std::string& retroArchConfig = "sdmc:/retroarch/retroarch.cfg",
    const std::string& retroArchFallback = "sdmc:/retroarch/cores/savefiles");

[[nodiscard]] bool candidateMatchesGame(
    const SourceCandidate& candidate, std::string_view gameId) noexcept;

[[nodiscard]] const char* candidateStatusName(CandidateStatus status) noexcept;

}
#endif
