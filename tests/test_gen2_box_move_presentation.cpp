// NEW-04 (round 3 hardware): Gold / Silver / Crystal PC box Move picked up but never completed.
// Pickup runs StagedEditor::beginPackedMove and then refreshes the box grid through
// GSCReadOnlyTrainer::refreshStagedBoxPresentation, which used finalizedBytes(). finalizedBytes()
// refuses while a carry is active, so every pickup failed its first refresh with "Place or cancel
// carried Pokemon before finalizing the Gen II staged save" and the carry was cancelled. Gen I
// presents its grid without finalizing and was unaffected.
//
// This drives the real trainer bridge and staged editor through the overlay's sequence: pickup ->
// refresh, drop into the same box -> refresh, pickup -> drop into another box -> refresh, then
// finalize and the verified export. Synthetic fixture only; the source buffer never changes.
#include "Integration/Gen2/Gen2ExportTransaction.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "tests/fixtures/gsc_pokemon_fixture.h"
#include "Utils/StringHelpers.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace G2 = PokeVault::Integration::Gen2;

namespace {

std::vector<std::string> shownOrder(const PokeVault::Legacy::GSCReadOnlyTrainer& trainer, std::size_t box) {
    std::vector<std::string> out;
    for (std::size_t slot = 0; slot < trainer.getSlotsPerBox(); ++slot) {
        const auto& pokemon = trainer.boxes[box][slot];
        if (!pokemon) break;
        out.push_back(Utils::utf16ToUtf8(pokemon->nickname()));
    }
    return out;
}

std::vector<uint8_t> slurp(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

void run(const L& layout, G2::SourceGame game, const char* gameId) {
    const auto raw = fixture(layout);
    const auto parsed = G2::parse(raw, game);
    assert(parsed);
    std::string error;
    auto trainer = PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error);
    assert(trainer && error.empty());
    auto* editor = trainer->stagedEditor();
    assert(editor);
    for (const char* nick : {"B", "C"}) {
        G2::BoxPokemonCreate create;
        create.species = 25;
        create.level = 15;
        create.nickname = nick;
        create.otName = "ASH";
        std::size_t slot = 99;
        assert(editor->stageAddBoxPokemon(0, create, slot, error));
    }
    assert(trainer->refreshStagedBoxPresentation(error));
    assert((shownOrder(*trainer, 0) == std::vector<std::string>{"PIKA", "B", "C"}));

    // 1. Pickup (Box 1 slot 2): the grid refresh must succeed mid-carry and show the gap closed.
    assert(editor->beginPackedMove(0, 1, error));
    assert(editor->packedMoveActive());
    assert(trainer->refreshStagedBoxPresentation(error) && error.empty());
    assert((shownOrder(*trainer, 0) == std::vector<std::string>{"PIKA", "C"}));
    std::string finalizeError;
    assert(editor->finalizedBytes(finalizeError).empty() && !finalizeError.empty()); // still not exportable

    // 2. Drop into the same box after "C".
    std::size_t placed = 99;
    assert(editor->placePackedMove(0, 2, placed, error) && placed == 2);
    assert(trainer->refreshStagedBoxPresentation(error));
    assert((shownOrder(*trainer, 0) == std::vector<std::string>{"PIKA", "C", "B"}));

    // 3. Pickup again and drop into another box (Box 4, empty).
    assert(editor->beginPackedMove(0, 2, error));
    assert(trainer->refreshStagedBoxPresentation(error));
    assert(editor->placePackedMove(3, 0, placed, error) && placed == 0);
    assert(trainer->refreshStagedBoxPresentation(error));
    assert((shownOrder(*trainer, 0) == std::vector<std::string>{"PIKA", "C"}));
    assert((shownOrder(*trainer, 3) == std::vector<std::string>{"B"}));

    // 4. A cancelled pickup also refreshes and restores the grid.
    assert(editor->beginPackedMove(0, 0, error) && trainer->refreshStagedBoxPresentation(error));
    assert(editor->cancelPackedMove(error) && trainer->refreshStagedBoxPresentation(error));
    assert((shownOrder(*trainer, 0) == std::vector<std::string>{"PIKA", "C"}));

    // 5. Finalize and the verified export: the moved Pokemon is in Box 4 of the exported copy.
    const auto finalized = editor->finalizedBytes(error);
    assert(!finalized.empty() && error.empty());
    const auto reparsed = G2::parse(finalized, game);
    assert(reparsed && reparsed.save->boxes()[3].slots[0] && reparsed.save->boxes()[3].slots[0]->nickname == "B");

    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / ("pokebank-gen2-move-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    G2::StagedEditorExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = std::string(gameId) + "_move";
    request.sourcePath = "/retroarch/saves/fixture.srm";
    request.gameId = gameId;
    request.timestamp = "20261020-120000";
    const auto result = G2::publishVerifiedStagedEditorExport(*editor, request);
    assert(result.success);
    assert(slurp(fs::path(result.directory) / "edited.srm") == finalized);
    assert(slurp(fs::path(result.directory) / "original_backup.srm") == raw);
    fs::remove_all(root);
    assert(std::equal(editor->originalBytes().begin(), editor->originalBytes().end(), raw.begin()));
}

} // namespace

int main() {
    run(GS, G2::SourceGame::Gold, "gold_gbc");
    run(GS, G2::SourceGame::Silver, "silver_gbc");
    run(C, G2::SourceGame::Crystal, "crystal_gbc");
    std::cout << "Gen II PC box Move: pickup / same-box / other-box / cancel refresh, finalize, export (NEW-04) PASS\n";
    return 0;
}
