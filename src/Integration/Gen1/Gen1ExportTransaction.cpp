#include "Integration/Gen1/Gen1ExportTransaction.h"

#include "Integration/Common/ClassicStrictValidators.h"

#include <algorithm>

namespace PokeVault::Integration::Gen1 {

std::vector<std::string> gen1ExportChangeLines(
    const StagedPokemonEditor& editor, const StagedInventoryEditor* inventory) {
    std::vector<std::string> out;
    for (const auto& change : editor.pendingChanges())
        out.push_back(change.label + ": " + change.beforeValue + " -> " + change.afterValue);
    if (inventory) {
        for (const auto& change : inventory->pendingChanges())
            out.push_back("Inventory item " + std::to_string(change.itemId) + ": " +
                          std::to_string(change.before) + " -> " + std::to_string(change.after));
    }
    return out;
}

Common::VerifiedExportResult publishVerifiedGen1Export(
    const StagedPokemonEditor& editor, const Gen1ExportRequest& request,
    const StagedInventoryEditor* inventory) {
    Common::VerifiedExportResult result;
    const bool inventoryPending = inventory && inventory->hasPendingChanges();
    if (!editor.hasPendingChanges() && !inventoryPending) {
        result.error = "No pending Generation I changes to export";
        return result;
    }
    const SourceGame game = editor.metadata().sourceGame;
    std::string error;
    std::vector<uint8_t> inventoryCandidate;
    if (inventoryPending) {
        // A Bag/PC workspace opened from another (or an older) copy of the save must never be
        // spliced into this export, even inside the inventory byte ranges.
        const auto pokemonOriginal = editor.originalBytes();
        const auto inventoryOriginal = inventory->originalBytes();
        if (inventory->sourceGame() != game ||
            !std::equal(pokemonOriginal.begin(), pokemonOriginal.end(),
                        inventoryOriginal.begin(), inventoryOriginal.end())) {
            result.error = "Bag and Pokemon workspaces were opened from different saves";
            return result;
        }
        inventoryCandidate = inventory->finalize(error);
        if (inventoryCandidate.empty()) {
            result.error = error.empty() ? "Staged Generation I Bag finalization failed" : error;
            return result;
        }
    }
    const auto edited = editor.finalizedBytes(error, inventoryCandidate);
    if (edited.empty()) {
        result.error = error.empty() ? "Generation I staged export finalization failed" : error;
        return result;
    }

    Common::VerifiedExportRequest transaction;
    transaction.rootDirectory = request.rootDirectory;
    transaction.directoryStem = request.directoryStem;
    transaction.generationLabel = "Generation I";
    transaction.fileExtension = ".srm";
    transaction.sourcePath = request.sourcePath;
    transaction.gameId = sourceGameId(game);
    transaction.saveFormat = "International R/B/Y SRAM (Party edit deferred)";
    transaction.timestamp = request.timestamp;
    transaction.applicationSha = request.applicationSha;
    transaction.originalBytes = editor.originalBytes();
    transaction.editedBytes = edited;
    transaction.changeLines = gen1ExportChangeLines(editor, inventory);
    transaction.validator = Common::gen1StrictValidator(game);
    transaction.failurePoint = request.failurePoint;
    return Common::publishVerifiedStagedExport(transaction);
}

} // namespace PokeVault::Integration::Gen1
