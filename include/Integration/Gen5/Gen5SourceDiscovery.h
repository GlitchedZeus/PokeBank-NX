#ifndef POKEBANK_GEN5_SOURCE_DISCOVERY_H
#define POKEBANK_GEN5_SOURCE_DISCOVERY_H

#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// A bounded, opt-in read-only source catalog. This is not wired to Product Home,
// profile auto-assignment, or the shared editor. Never scan an SD-card root.
struct DiscoveryRoot {
    std::string path;
    std::string providerLabel;
    size_t maxDepth = 2;
};
struct DiscoveryLimits {
    size_t maxFiles = 256;
};
struct DiscoveryResult {
    std::vector<Source::SaveInstance> instances;
    size_t filesExamined = 0;
    bool limitReached = false;
};

// Read a single confirmed raw 0x80000 save or exact documented DeSmuME
// 0x80000+40 DSV footer container. Unsupported wrappers and savestates fail
// closed. The original is never opened in write mode.
[[nodiscard]] bool readNormalizedSourceReadOnly(
    const std::string& path, std::vector<uint8_t>& bytes,
    std::string& diagnostic, std::string* containerType = nullptr);

// A file is always validated by its internal exact game ID, not its filename.
[[nodiscard]] Source::SaveInstance inspectSourceFile(
    const std::string& path, std::string_view providerLabel,
    std::string_view assignedExactGame = {});

// Reopen a catalog entry only after fresh strict validation AND snapshot
// equivalence. Never trust cached trainer data or filesystem timestamp alone.
[[nodiscard]] ReadOnlyProbe reopenValidatedSource(
    const Source::SaveInstance& selected,
    SaveCopySelection copy = SaveCopySelection::Automatic);

[[nodiscard]] DiscoveryResult discoverSources(
    std::span<const DiscoveryRoot> roots, DiscoveryLimits limits = {});

// Known nonrecursive top-level directories only; fixed bounded descent.
// Custom paths are passed to discoverSources explicitly by a caller.
[[nodiscard]] std::vector<DiscoveryRoot> defaultDraSticRoots();
[[nodiscard]] DiscoveryResult discoverKnownSources(
    DiscoveryLimits limits = {},
    const std::string& retroArchConfig = "sdmc:/retroarch/retroarch.cfg",
    const std::string& retroArchFallback = "sdmc:/retroarch/cores/savefiles");

} // namespace PokeVault::Integration::Gen5
#endif
