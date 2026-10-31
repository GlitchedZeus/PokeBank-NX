#ifndef POKEBANK_MODERN_SOURCES_H
#define POKEBANK_MODERN_SOURCES_H

// Save Instances for every title served by a Modern::NativeSession adapter (Gen VI X/Y/ORAS now;
// SM/USUM and later families register here as their adapters land). The flow mirrors the accepted
// Gen IV/V model exactly: discovery over verified provider layouts (Dekopon, Checkpoint-layout
// exports, manual link), strict validation of the bytes, explicit per-profile assignment in
// LegacySourceBindings, content-fingerprint revalidation on open, and a read-only Home card
// snapshot. No function here can write a source save.

#include "Enums/GameVersion.h"
#include "Integration/Modern/NativeSession.h"
#include "Legacy/LegacySourceBindings.h"
#include "Source/HomeCardSnapshot.h"
#include "Source/SaveInstance.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Modern {

// ---- family dispatch (the capability registry says which ids are served) ----
[[nodiscard]] bool isModernAssignedId(std::string_view gameId) noexcept;
[[nodiscard]] Enums::GameVersion gameGroupFor(std::string_view gameId) noexcept;
[[nodiscard]] std::string_view expectedFamily(std::string_view gameId) noexcept;   // "XY", "ORAS", "SM", "USUM"
[[nodiscard]] std::string_view platformLabelFor(std::string_view gameId) noexcept; // "Nintendo 3DS"
[[nodiscard]] std::string_view generationLabelFor(std::string_view gameId) noexcept; // "Generation VI"
[[nodiscard]] std::string exportsRootFor(std::string_view gameId);                 // app-owned exports/<gen>
[[nodiscard]] OpenedSession openSession(std::string_view gameId, std::span<const uint8_t> bytes);

// ---- provider labels the Home card / binding records store (Source::identifyProvider knows them) ----
inline constexpr std::string_view kProviderDekopon = "Dekopon";
inline constexpr std::string_view kProviderCheckpoint = "Checkpoint";
inline constexpr std::string_view kProviderManual = "Manual";
// SAV6/SAV7 archives hold one native copy; the binding store records this fixed marker.
inline constexpr std::string_view kSourceVariant3ds = "3ds-single-copy";
// A Sword/Shield `main` is one SwishCrypto file (no Primary/Backup pair); manual link only until a
// Switch save-manager folder layout is verified from its source.
inline constexpr std::string_view kSourceVariantSwitchMain = "switch-main-single";
[[nodiscard]] std::string_view sourceVariantFor(std::string_view gameId) noexcept;

// ---- strict file inspection ----
// Reads the regular file at `path` (never a directory, never followed through dot segments),
// validates it as the assigned exact game and returns one Save Instance row. A failed validation
// is still a row, with the reason in `diagnostic`.
[[nodiscard]] Source::SaveInstance inspectSourceFile(const std::string& path, std::string_view providerLabel,
                                                     std::string_view assignedExactGame,
                                                     std::string_view evidenceGameId = {});
[[nodiscard]] std::vector<Source::SaveInstance> inspectSourceChoices(const std::string& path,
                                                                     std::string_view providerLabel,
                                                                     std::string_view assignedExactGame);
[[nodiscard]] bool readSourceFileReadOnly(const std::string& path, std::vector<uint8_t>& bytes, std::string& diagnostic);

struct DiscoveryRoots {
    std::string checkpointRoot = "sdmc:/3ds/Checkpoint/saves";
    std::string dekoponPointerFile = "sdmc:/switch/dekopon/user_dir.txt";
    std::string dekoponDefaultUserDir = "sdmc:/switch/dekopon/";
};
struct DiscoveryResult {
    std::vector<Source::SaveInstance> instances;
    size_t rootsConsidered = 0;
    size_t rootsOpened = 0;
    size_t filesExamined = 0;
};
[[nodiscard]] DiscoveryResult discoverKnownSources(std::string_view exactGame, const DiscoveryRoots& roots = {});

// ---- catalog / assignment / open (same contract as Gen5GameSourceCatalog / Gen5AssignedSource) ----
struct GameSourceCatalog {
    std::vector<Source::SaveInstance> rows;
    size_t wrongGame = 0;
    size_t invalidOrAmbiguous = 0;
    size_t ownedByOtherProfile = 0;
    size_t ambiguousOwnership = 0;
    bool validQuery = false;
};
[[nodiscard]] GameSourceCatalog forGameAndProfile(std::span<const Source::SaveInstance> discovered,
                                                  const Legacy::LegacySourceBindings& bindings,
                                                  std::string_view profile, std::string_view exactGame);

struct SourceAssignmentResult {
    bool success = false;
    Source::SaveInstance instance;
    std::string diagnostic;
};
[[nodiscard]] SourceAssignmentResult assignValidatedSource(Legacy::LegacySourceBindings& bindings,
                                                           const Source::SaveInstance& selected,
                                                           std::string_view profile, std::string_view exactGame);

enum class AssignedOpenStatus : uint8_t {
    Unassigned, Missing, Ambiguous, Unreadable, InvalidSave, AssignmentMismatch, Ready
};
struct AssignedSourceReadOnly {
    AssignedOpenStatus status = AssignedOpenStatus::Unassigned;
    Legacy::AssignedFile source;
    Source::SaveInstance instance;
    std::unique_ptr<NativeSession> session;
    std::string diagnostic;
    [[nodiscard]] bool ready() const noexcept {
        return status == AssignedOpenStatus::Ready && session != nullptr && instance.ready() && instance.readOnly();
    }
};
[[nodiscard]] AssignedSourceReadOnly openAssignedSource(const Legacy::LegacySourceBindings& bindings,
                                                        std::string_view profile, std::string_view exactGame,
                                                        std::string_view expectedContentFingerprint = {});

// Read-only Home card summary built from the same strict session (no partial party).
[[nodiscard]] Source::HomeCardSnapshot homeCardSnapshot(const AssignedSourceReadOnly& assigned);

} // namespace PokeVault::Integration::Modern

#endif
