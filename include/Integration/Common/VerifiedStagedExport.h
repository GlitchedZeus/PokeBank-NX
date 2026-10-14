#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

// Generation-neutral verified export of an app-owned staged save copy.
//
// The transaction writes ONLY inside an app-owned export root: a temporary directory receives an
// exact original backup, the edited save and a provenance manifest; every file is fsync'ed, read
// back, hash-checked and the edited bytes are strictly reparsed by the caller's generation parser
// before the directory is atomically renamed into place. Any failure removes the temporary
// directory. No source/emulator save path is ever opened for writing.
namespace PokeVault::Integration::Common {

enum class ExportFailurePoint : uint8_t {
    None,
    AfterBackupWrite,
    AfterEditedWrite,
    AfterReadbackVerify,
    AfterManifestWrite,
};

// Returns true when bytes parse strictly as the expected exact game; fills error otherwise.
using StrictSaveValidator = std::function<bool(std::span<const uint8_t>, std::string&)>;

struct VerifiedExportRequest {
    std::string rootDirectory;
    std::string directoryStem;
    std::string generationLabel;      // e.g. "Generation IV"
    std::string fileExtension = ".sav";
    std::string sourcePath;
    std::string gameId;
    std::string saveFormat;
    std::string timestamp;
    std::string applicationSha = "unknown";
    std::span<const uint8_t> originalBytes;
    std::span<const uint8_t> editedBytes;
    std::vector<std::string> changeLines;
    StrictSaveValidator validator;

    // Host-only fault injection for cleanup tests. Production callers leave this at None.
    ExportFailurePoint failurePoint = ExportFailurePoint::None;
};

struct VerifiedExportResult {
    bool success = false;
    std::string directory;
    std::string originalSha256;
    std::string editedSha256;
    size_t changedBytes = 0;
    std::string error;
};

[[nodiscard]] std::string sha256Hex(std::span<const uint8_t> bytes);

[[nodiscard]] VerifiedExportResult publishVerifiedStagedExport(const VerifiedExportRequest& request);

} // namespace PokeVault::Integration::Common
