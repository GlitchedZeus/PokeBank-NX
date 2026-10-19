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
    // Host-only race simulation: another writer creates the final directory
    // (empty) or a plain file at that path just before publication.
    DestinationAppearsAsEmptyDirectory,
    DestinationAppearsAsFile,
    AfterFirstPublishedFile,
    // Host-only: flip one byte of the written original backup before read-back.
    CorruptBackupAfterWrite,
};

// Returns true when bytes parse strictly as the expected exact game; fills error otherwise.
using StrictSaveValidator = std::function<bool(std::span<const uint8_t>, std::string&)>;

struct VerifiedExportRequest {
    std::string rootDirectory;
    std::string directoryStem;
    std::string generationLabel;      // e.g. "Generation IV"
    std::string fileExtension = ".sav";
    // Optional extension of edited<ext> only. Empty: fileExtension, as before. A verbatim .dsv
    // backup keeps fileExtension ".dsv" while the edited raw payload is named edited.sav, so it
    // never looks like a footer container to an emulator or to discovery.
    std::string editedFileExtension;
    std::string sourcePath;
    std::string gameId;
    std::string saveFormat;
    std::string timestamp;
    std::string applicationSha = "unknown";
    std::span<const uint8_t> originalBytes;
    std::span<const uint8_t> editedBytes;

    // Optional verbatim source FILE (for example a DeSmuME .dsv: payload, emulator
    // metadata and footer). Empty: the backup is originalBytes, exactly as before. Non-empty:
    // it must start with originalBytes; original_backup<ext> is written from it and read back
    // against it, and the manifest adds SOURCE_FILE_SIZE, SOURCE_FILE_SHA256,
    // ORIGINAL_BACKUP_SHA256 and ORIGINAL_BACKUP_IS=VERBATIM_SOURCE_FILE. SOURCE_SIZE,
    // SOURCE_SHA256 and CHANGED_BYTES keep describing the payload.
    std::span<const uint8_t> backupBytes;
    // Optional container facts (the Gen V working-copy manifest keys). Each is written only
    // when set and must be a token of [A-Za-z0-9_.-]: SOURCE_CONTAINER (e.g. "dsv-footer"),
    // EDITED_CONTAINER (e.g. "raw-nds-battery"), SOURCE_SHA256_SCOPE (e.g.
    // "0x80000_SAV4_PAYLOAD"). No other manifest text can be supplied.
    std::string sourceContainer;
    std::string editedContainer;
    std::string sourceSha256Scope;
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
    std::string backupSha256; // SHA-256 of original_backup<ext> (== originalSha256 unless verbatim)
    size_t changedBytes = 0;
    std::string error;
};

[[nodiscard]] std::string sha256Hex(std::span<const uint8_t> bytes);

// Combine two staged copies of the SAME original whose edits live in different fixed-size blocks
// (for example Gen III 4 KiB sectors, each with its own checksum). Every block is taken from the
// copy that changed it; a block changed by both copies is refused rather than guessed. Returns an
// empty vector and fills error on mismatch/overlap.
[[nodiscard]] std::vector<uint8_t> mergeDisjointBlocks(std::span<const uint8_t> original,
                                                       std::span<const uint8_t> first,
                                                       std::span<const uint8_t> second,
                                                       size_t blockSize, std::string& error);

[[nodiscard]] VerifiedExportResult publishVerifiedStagedExport(const VerifiedExportRequest& request);

} // namespace PokeVault::Integration::Common
