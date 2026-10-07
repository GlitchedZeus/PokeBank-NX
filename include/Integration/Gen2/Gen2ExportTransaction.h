#pragma once

#include "Integration/Gen2/Gen2StagedEditor.h"

#include <span>
#include <string>

namespace PokeVault::Integration::Gen2 {

enum class ExportFailurePoint : uint8_t {
    None,
    AfterBackupWrite,
    AfterEditedWrite,
    AfterReadbackVerify,
    AfterManifestWrite,
};

struct ExportTransactionRequest {
    std::string rootDirectory;
    std::string directoryStem;
    std::string sourcePath;
    std::string gameId;
    std::string saveFormat = "PK2 / GSC SRAM";
    std::string timestamp;
    std::string applicationSha = "unknown";
    SourceGame sourceGame = SourceGame::Gold;
    std::span<const uint8_t> originalBytes;
    std::span<const uint8_t> editedBytes;
    std::span<const StagedChange> pendingChanges;

    // Host-only fault injection for cleanup tests. Production callers leave this at None.
    ExportFailurePoint failurePoint = ExportFailurePoint::None;
};

struct StagedEditorExportRequest {
    std::string rootDirectory;
    std::string directoryStem;
    std::string sourcePath;
    std::string gameId;
    std::string saveFormat = "PK2 / GSC SRAM";
    std::string timestamp;
    std::string applicationSha = "unknown";
};

struct ExportTransactionResult {
    bool success = false;
    std::string directory;
    std::string originalSha256;
    std::string editedSha256;
    std::string error;
};

// Low-level verified publisher for already-finalized bytes. Kept public for focused corruption/fault tests.
ExportTransactionResult publishVerifiedExport(const ExportTransactionRequest& request);

// Authoritative UI-facing Generation II export workflow. It owns staged finalization and pending-change
// capture, then delegates the actual filesystem transaction to publishVerifiedExport(). Renderers should
// call this rather than writing .srm/manifest files themselves.
ExportTransactionResult publishVerifiedStagedEditorExport(
    const StagedEditor& editor, const StagedEditorExportRequest& request);

} // namespace PokeVault::Integration::Gen2
