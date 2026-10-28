#include "Integration/Gen3/Gen3ExportTransaction.h"

namespace PokeVault::Integration::Gen3 {

Common::VerifiedExportResult publishVerifiedGen3Export(
    const StagedPokemonEditor& editor, const Gen3ExportRequest& request,
    const StagedInventoryEditor* inventory, const Gen3StagedTrainerEditor* trainer) {
    Common::VerifiedExportResult result;
    const bool bagPending = inventory && inventory->hasPendingChanges();
    const bool trainerPending = trainer && trainer->hasPendingChanges();
    if (!editor.hasPendingChanges() && !bagPending && !trainerPending) {
        result.error = "No pending changes to export";
        return result;
    }
    std::string finalizeError;
    auto edited = editor.finalizedBytes(finalizeError);
    if (edited.empty()) {
        result.error = finalizeError.empty() ? "Staged Gen III finalization failed" : finalizeError;
        return result;
    }
    if (bagPending) {
        if (inventory->sourceGame() != editor.sourceGame() ||
            inventory->originalBytes() != editor.originalBytes()) {
            result.error = "Bag and Pokemon workspaces were opened from different saves";
            return result;
        }
        const auto bag = inventory->finalizedBytes(finalizeError);
        if (bag.empty()) {
            result.error = finalizeError.empty() ? "Staged Gen III Bag finalization failed" : finalizeError;
            return result;
        }
        edited = Common::mergeDisjointBlocks(editor.originalBytes(), edited, bag, 0x1000, finalizeError);
        if (edited.empty()) {
            result.error = finalizeError;
            return result;
        }
    }
    if (trainerPending) {
        if (trainer->sourceGame() != editor.sourceGame() ||
            trainer->originalBytes() != editor.originalBytes()) {
            result.error = "Trainer and Pokemon workspaces were opened from different saves";
            return result;
        }
        if (!trainer->applyTo(edited, finalizeError)) {
            result.error = finalizeError;
            return result;
        }
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
            (change.party?"Party "+std::to_string(change.slot+1):
                "Box " + std::to_string(change.box + 1) + " slot " + std::to_string(change.slot + 1)) +
            " " + change.label + ": " +
            (change.createdDetail ? change.afterValue   // CG74: a created Pokemon's detail
                                        : change.beforeValue + " -> " + change.afterValue));
    if (bagPending) transaction.changeLines.push_back("Bag: staged item changes included");
    if (trainerPending)
        for (const auto& change : trainer->pendingChanges())
            transaction.changeLines.push_back(change.label + ": " + change.before + " -> " + change.after);
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
