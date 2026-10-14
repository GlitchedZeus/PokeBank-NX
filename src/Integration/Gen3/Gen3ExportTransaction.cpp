#include "Integration/Gen3/Gen3ExportTransaction.h"

namespace PokeVault::Integration::Gen3 {

Common::VerifiedExportResult publishVerifiedGen3Export(
    const StagedPokemonEditor& editor, const Gen3ExportRequest& request) {
    Common::VerifiedExportResult result;
    if (!editor.hasPendingChanges()) {
        result.error = "No pending changes to export";
        return result;
    }
    std::string finalizeError;
    const auto edited = editor.finalizedBytes(finalizeError);
    if (edited.empty()) {
        result.error = finalizeError.empty() ? "Staged Gen III finalization failed" : finalizeError;
        return result;
    }

    const SourceGame game = editor.sourceGame();
    Common::VerifiedExportRequest transaction;
    transaction.rootDirectory = request.rootDirectory;
    transaction.directoryStem = request.directoryStem;
    transaction.generationLabel = "Generation III";
    transaction.fileExtension = ".sav";
    transaction.sourcePath = request.sourcePath;
    transaction.gameId = std::string(sourceGameId(game));
    transaction.saveFormat = "PK3 / rotating-sector Flash (0x20000)";
    transaction.timestamp = request.timestamp;
    transaction.applicationSha = request.applicationSha;
    transaction.originalBytes = editor.originalBytes();
    transaction.editedBytes = edited;
    for (const auto& change : editor.pendingChanges())
        transaction.changeLines.push_back(
            "Box " + std::to_string(change.box + 1) + " slot " + std::to_string(change.slot + 1) +
            " " + change.label + ": " + change.beforeValue + " -> " + change.afterValue);
    transaction.validator = [game](std::span<const uint8_t> bytes, std::string& error) {
        auto parsed = parse(bytes, game);
        if (!parsed) {
            error = parsed.detail.empty() ? std::string(errorMessage(parsed.error)) : parsed.detail;
            return false;
        }
        return true;
    };
    transaction.failurePoint = request.failurePoint;
    return Common::publishVerifiedStagedExport(transaction);
}

} // namespace PokeVault::Integration::Gen3
