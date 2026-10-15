#ifndef POKEBANK_GEN5_WORKING_COPY_EXPORT_H
#define POKEBANK_GEN5_WORKING_COPY_EXPORT_H

#include "Integration/Gen5/Gen5StagedSaveCandidate.h"

#include <cstdint>
#include <span>
#include <string>

namespace PokeVault::Integration::Gen5 {

// Writes ONLY a new, app-owned export directory. No API argument grants a
// source-path write, emulator injection, or installed-game save permission.
// On Switch the destination is fixed to PokeBank NX's exports/gen5 tree.
enum class WorkingCopyFailurePoint : uint8_t {
    None, AfterBackup, AfterEdited, AfterReadback, AfterManifest,
    // Host-only race simulation: the destination appears (as an empty
    // directory) just before publication, or publication fails midway.
    DestinationAppears, AfterFirstPublishedFile
};
struct WorkingCopyExportRequest {
    std::string profileIdentity; // exact 32-character, nonzero AccountUid hex
    std::string timestamp;       // YYYYMMDD_HHMMSS
    std::string sourcePath;      // evidence only; never opened or written here
    std::string applicationSha = "unknown";
    std::span<const uint8_t> observedSourceBytes;

    // Host fixture root. On Switch, nonempty overrides MUST equal the
    // fixed PokeBank-owned exports/gen5 path or the operation fails closed.
    std::string rootDirectory;
    WorkingCopyFailurePoint failurePoint = WorkingCopyFailurePoint::None;
};
struct WorkingCopyExportResult {
    bool success = false;
    std::string directory;
    std::string sourceSha256;
    std::string editedSha256;
    std::string error;
    size_t changedRecords = 0;
};

// Source bytes must already have been freshly read/physically revalidated by
// the calling source provider. Rechecks their complete byte equivalence and
// exact game/copy against the app-owned staged workspace BEFORE file I/O.
// Creates original_backup.sav + edited.sav + EXPORT_MANIFEST.txt and checks
// both on-disk SAV5 copies and SHA-256 before publishing the directory.
[[nodiscard]] WorkingCopyExportResult publishGen5WorkingCopy(
    const Gen5StagedPokemonWorkspace& workspace,
    const WorkingCopyExportRequest& request);

} // namespace PokeVault::Integration::Gen5
#endif
