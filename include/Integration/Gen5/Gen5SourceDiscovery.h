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
    // Also bound directory enumeration (including unrelated files). A large
    // RetroArch save directory must never allocate/sort millions of names
    // merely because only the first 256 valid save files are parsed.
    size_t maxEntries = 4096;
    size_t maxDirectories = 128;
};
struct DiscoveryResult {
    std::vector<Source::SaveInstance> instances;
    size_t filesExamined = 0;
    size_t entriesExamined = 0;
    size_t directoriesExamined = 0;
    // "No saves" is not proof of a scanned emulator folder. Report only
    // attempted/actually opened bounded roots, never full SD traversal.
    size_t rootsConsidered = 0;
    size_t rootsOpened = 0;
    // Distinct safe roots admitted to the fair-share scan budget.
    size_t rootsExamined = 0;
    bool limitReached = false;
    size_t unreadableDirectories = 0;
};

// Read a single confirmed raw 0x80000 save or a DeSmuME .dsv with a
// 0x80000-byte prefix and validated final 40-byte footer, regardless of
// trailing emulator metadata size (Gen IV adapter parity). Only the fixed
// prefix/footer are buffered. Malformed wrappers and savestates fail closed.
// The original is never opened in write mode.
[[nodiscard]] bool readNormalizedSourceReadOnly(
    const std::string& path, std::vector<uint8_t>& bytes,
    std::string& diagnostic, std::string* containerType = nullptr);

// Stable metadata key for a source owned by one Switch profile. On FAT/exFAT
// (st_ino == 0), saved content naturally changes whenever the emulator saves:
// the persisted profile assignment must therefore key the NORMALIZED PATH,
// NOT the changing byte fingerprint. On filesystems with usable inode identity
// a different inode at the same path still requires explicit reassignment.
// This is not edit authorization: every A/Open revalidates exact game, copy and
// selected 0x80000 SHA-256 snapshot separately.
[[nodiscard]] std::string persistentGen5SourceIdentity(
    std::string_view path, uint64_t device, uint64_t inode);

// A file is always validated by its internal exact game ID, not its filename.
[[nodiscard]] Source::SaveInstance inspectSourceFile(
    const std::string& path, std::string_view providerLabel,
    std::string_view assignedExactGame = {},
    SaveCopySelection copy = SaveCopySelection::Automatic);

// One physical read and fingerprint for every choice from the same file.
// Different valid copies yield two labeled rows; single/identical copies
// yield the pinned validated row. Rejections remain visible as one row.
[[nodiscard]] std::vector<Source::SaveInstance> inspectSourceChoices(
    const std::string& path, std::string_view providerLabel,
    std::string_view assignedExactGame = {});

// Reopen a catalog entry only after fresh strict validation AND snapshot
// equivalence. Never trust cached trainer data or filesystem timestamp alone.
[[nodiscard]] ReadOnlyProbe reopenValidatedSource(
    const Source::SaveInstance& selected,
    SaveCopySelection copy = SaveCopySelection::Automatic);

// Owner-requested evidence: read the source read-only, inspect BOTH copies
// and write a plain-text report (no names, IDs or raw bytes) as a NEW file in
// the app-owned diagnostics directory. Never opens the source for writing.
[[nodiscard]] bool writeGen5DiagnosticsReport(
    const std::string& sourcePath, std::string_view exactGame,
    std::string_view timestamp, const std::string& diagnosticsRoot,
    std::string& writtenPath, std::string& error);

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
