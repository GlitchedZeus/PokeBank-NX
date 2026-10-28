#include "Integration/Gen1/Gen1ExportTransaction.h"

#include "Integration/Common/ClassicStrictValidators.h"
#include "Names/ItemNames.h"

#include <algorithm>

namespace PokeVault::Integration::Gen1 {

std::vector<std::string> gen1ExportChangeLines(
    const StagedPokemonEditor& editor, const StagedInventoryEditor* inventory,
    const StagedTrainerEditor* trainer) {
    std::vector<std::string> out;
    // An empty "before" is a new value (a created Pokemon's details): "<label>: <value>".
    for (const auto& change : editor.pendingChanges())
        out.push_back(change.label + ": " + (change.beforeValue.empty() ? change.afterValue
                                                                      : change.beforeValue + " -> " + change.afterValue));
    if (inventory) {
        for (const auto& change : inventory->pendingChanges())
            out.push_back(std::string(Names::getItemNameG1(change.itemId)) + ": " +
                          std::to_string(change.before) + " -> " + std::to_string(change.after));
    }
    if (trainer) {
        for (const auto& change : trainer->pendingChanges())
            out.push_back(change.label + ": " + change.beforeValue + " -> " + change.afterValue);
    }
    return out;
}

Common::VerifiedExportResult publishVerifiedGen1Export(
    const StagedPokemonEditor& editor, const Gen1ExportRequest& request,
    const StagedInventoryEditor* inventory, const StagedTrainerEditor* trainer) {
    Common::VerifiedExportResult result;
    const bool inventoryPending = inventory && inventory->hasPendingChanges();
    const bool trainerPending = trainer && trainer->hasPendingChanges();
    if (!editor.hasPendingChanges() && !inventoryPending && !trainerPending) {
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
    std::vector<uint8_t> trainerCandidate;
    if (trainerPending) {
        // Same rule as the Bag/PC workspace: a Trainer workspace from another copy is refused.
        const auto pokemonOriginal = editor.originalBytes();
        const auto trainerOriginal = trainer->originalBytes();
        if (trainer->sourceGame() != game ||
            !std::equal(pokemonOriginal.begin(), pokemonOriginal.end(),
                        trainerOriginal.begin(), trainerOriginal.end())) {
            result.error = "Trainer and Pokemon workspaces were opened from different saves";
            return result;
        }
        trainerCandidate = trainer->finalize(error);
        if (trainerCandidate.empty()) {
            result.error = error.empty() ? "Staged Generation I Trainer finalization failed" : error;
            return result;
        }
    }
    const auto edited = editor.finalizedBytes(error, inventoryCandidate, trainerCandidate);
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
    transaction.changeLines = gen1ExportChangeLines(editor, inventory, trainer);
    transaction.validator = Common::gen1StrictValidator(game);
    transaction.failurePoint = request.failurePoint;
    return Common::publishVerifiedStagedExport(transaction);
}

} // namespace PokeVault::Integration::Gen1
