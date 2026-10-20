// NEW-19 / R2-TRAINER / R2-BUG-002 (round 3 hardware): leaving a Gen I-IV session with staged
// changes offered only Discard / Cancel, also for a Trainer-only edit. The classic exit menu is
// Gen V's ConfirmExit plus Export Copy: Y Discard & Exit, X Review, A Export Copy, B Continue.
//
// Exercises the production model (UI/ClassicStagedExitMenu.h) with the shell's own flags, and a
// real Gold / Silver / Crystal GSCReadOnlyTrainer whose ONLY staged edit is the Trainer name: it
// reaches the menu, Export Copy writes a verified copy carrying the edit, Discard & Exit leaves
// the source byte-identical. The routing that the native-only composite TU wires is pinned at
// the end. Synthetic fixture only.
#include "Integration/Gen2/Gen2ExportTransaction.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "UI/ClassicStagedExitMenu.h"
#include "tests/fixtures/gsc_pokemon_fixture.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace Exit = PokeBank::UIModel::ClassicStagedExit;
namespace G2 = PokeVault::Integration::Gen2;

namespace {

// The shell's dirty-exit flags, exactly the members TrainerViewScreen hands to adopt().
struct Shell {
    bool saveConfirmActive = false;
    bool exitingWithUnsavedChanges = false;
    bool exitingViaPlus = false;
    bool itemsShortcutActive = false;
    // What the shell's B / + handlers do when trainer.hasStagedChanges() is true.
    void askToLeave(bool viaPlus) {
        saveConfirmActive = exitingWithUnsavedChanges = true;
        exitingViaPlus = viaPlus;
    }
};

bool adopt(Exit::Session& session, Shell& shell, bool staged) {
    return Exit::adopt(session, shell.saveConfirmActive, shell.exitingWithUnsavedChanges,
                       shell.exitingViaPlus, shell.itemsShortcutActive, staged);
}

std::string slurp(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

void testButtonsAndChords() {
    assert(Exit::kButtons.size() == 4);
    const std::pair<const char*, Exit::Choice> expected[] = {
        {"Y", Exit::Choice::DiscardAndExit}, {"X", Exit::Choice::Review},
        {"A", Exit::Choice::ExportCopy}, {"B", Exit::Choice::Continue}};
    for (std::size_t i = 0; i < 4; ++i) {
        assert(std::string(Exit::kButtons[i].glyph) == expected[i].first);
        assert(Exit::kButtons[i].choice == expected[i].second);
        // Each visible glyph maps to exactly its own choice.
        const std::string g = Exit::kButtons[i].glyph;
        assert(Exit::choiceFor(g == "A", g == "B", g == "X", g == "Y") == Exit::kButtons[i].choice);
    }
    assert(std::string(Exit::kButtons[0].label) == "Discard & Exit");
    assert(std::string(Exit::kButtons[2].label) == "Export Copy");
    assert(Exit::choiceFor(false, false, false, false) == Exit::Choice::None);
    // A chord never discards: B wins over everything, Review / Export over Y.
    assert(Exit::choiceFor(true, true, true, true) == Exit::Choice::Continue);
    assert(Exit::choiceFor(false, false, true, true) == Exit::Choice::Review);
    assert(Exit::choiceFor(true, false, false, true) == Exit::Choice::ExportCopy);
}

void testSessionFlow() {
    int owner = 0;
    auto& session = Exit::sessionFor(&owner, "gold_gbc");
    Shell shell;

    // Nothing staged: the shell's flags are left exactly as they are (it does not ask then).
    shell.askToLeave(false);
    assert(!adopt(session, shell, false));
    assert(shell.saveConfirmActive && shell.exitingWithUnsavedChanges);
    shell = {};
    assert(!adopt(session, shell, true)); // staged, but nobody asked to leave

    // B on the hub with staged changes: the menu takes the question, the old dialog never shows.
    shell.askToLeave(false);
    assert(adopt(session, shell, true) && session.open && !session.viaPlus);
    assert(!shell.saveConfirmActive && !shell.exitingWithUnsavedChanges && !shell.exitingViaPlus);
    assert(adopt(session, shell, true)); // stays open on later frames
    assert(Exit::choose(session, Exit::Choice::None) == Exit::Effect::None && session.open);

    // B Continue closes it; nothing else changes.
    assert(Exit::choose(session, Exit::Choice::Continue) == Exit::Effect::Close && !session.open);
    assert(!adopt(session, shell, true));
    assert(Exit::choose(session, Exit::Choice::DiscardAndExit) == Exit::Effect::None); // closed: inert

    // Export Copy keeps the menu open.
    shell.askToLeave(false);
    assert(adopt(session, shell, true));
    assert(Exit::choose(session, Exit::Choice::ExportCopy) == Exit::Effect::Export && session.open);

    // X Review opens the generation's Review; its B returns here while changes remain.
    assert(Exit::choose(session, Exit::Choice::Review) == Exit::Effect::OpenReview);
    assert(!session.open && session.reviewFromExit);
    assert(Exit::returnFromReview(session, true) && session.open && !session.reviewFromExit);
    // A Review opened from Box Actions is not affected.
    assert(!Exit::returnFromReview(session, true));
    // Discard All inside that Review: nothing staged, so the hub, not the menu.
    assert(Exit::choose(session, Exit::Choice::Review) == Exit::Effect::OpenReview);
    assert(Exit::returnFromReview(session, false) && !session.open);
    // Review unavailable: the menu stays.
    shell.askToLeave(false);
    assert(adopt(session, shell, true));
    assert(Exit::choose(session, Exit::Choice::Review) == Exit::Effect::OpenReview);
    Exit::reviewUnavailable(session);
    assert(session.open && !session.reviewFromExit);
    assert(Exit::choose(session, Exit::Choice::Continue) == Exit::Effect::Close);

    // + (leave the app) and the Items shortcut are remembered for Discard & Exit.
    shell.itemsShortcutActive = true;
    shell.askToLeave(true);
    assert(adopt(session, shell, true) && session.viaPlus && session.itemsShortcut);
    assert(Exit::choose(session, Exit::Choice::DiscardAndExit) == Exit::Effect::Exit && !session.open);

    // A new screen or game starts clean.
    int other = 0;
    assert(!Exit::sessionFor(&other, "gold_gbc").open);
    assert(&Exit::sessionFor(&other, "crystal_gbc") == &Exit::sessionFor(&other, "crystal_gbc"));
}

// A Trainer-only staged session on the real Gen II adapter reaches the menu, and Export Copy's
// verified export carries the edit; Discard & Exit leaves the source unchanged.
void testTrainerOnlySession(const L& layout, G2::SourceGame game, const char* gameId) {
    const auto raw = fixture(layout);
    const auto parsed = G2::parse(raw, game);
    assert(parsed);
    std::string error;
    auto trainer = PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error);
    assert(trainer && error.empty());
    assert(!trainer->hasStagedChanges());
    auto* editor = trainer->stagedEditor();
    assert(editor && editor->stageTrainerName("BLUE", error));
    assert(trainer->hasStagedChanges()); // Trainer-only edits count

    auto& session = Exit::sessionFor(trainer.get(), gameId);
    Shell shell;
    shell.askToLeave(false);
    assert(adopt(session, shell, trainer->hasStagedChanges()));

    // Export Copy: the same verified export the Gen II Review's A runs.
    assert(Exit::choose(session, Exit::Choice::ExportCopy) == Exit::Effect::Export);
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / ("pokebank-classic-exit-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    G2::StagedEditorExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = std::string(gameId) + "_exit";
    request.sourcePath = "/retroarch/saves/fixture.srm";
    request.gameId = gameId;
    request.timestamp = "20261020-120000";
    const auto result = G2::publishVerifiedStagedEditorExport(*editor, request);
    assert(result.success);
    const auto edited = slurp(fs::path(result.directory) / "edited.srm");
    const auto reparsed = G2::parse(std::vector<uint8_t>(edited.begin(), edited.end()), game);
    assert(reparsed && reparsed.save->trainer().name == "BLUE");
    assert(slurp(fs::path(result.directory) / "original_backup.srm") ==
           std::string(raw.begin(), raw.end()));
    fs::remove_all(root);
    assert(session.open); // still asking: the user can now leave safely

    // Discard & Exit: nothing written, the source is byte-identical.
    assert(Exit::choose(session, Exit::Choice::DiscardAndExit) == Exit::Effect::Exit);
    assert(std::equal(editor->originalBytes().begin(), editor->originalBytes().end(), raw.begin()));
}

std::string read(const char* path) {
    std::ifstream in(path);
    std::stringstream out;
    out << in.rdbuf();
    return out.str();
}

// The native-only composite TU: the menu runs before every Gen I-IV layer and over the hub, and
// each generation's Review returns to it.
void testRouting() {
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto gen5Input = composite.find("Gen5SharedReadOnlySurface::handleInput(*this");
    const auto exitInput = composite.find("ClassicStagedExitOverlay::handleInput(*this, down)");
    const auto packed = composite.find("ClassicPackedMove::handleInput(*this");
    assert(gen5Input != std::string::npos && exitInput > gen5Input && exitInput < packed);
    const auto gen5Draw = composite.find("Gen5SharedReadOnlySurface::draw(*this, fb)");
    const auto exitDraw = composite.find("ClassicStagedExitOverlay::draw(*this, fb)");
    assert(exitDraw > gen5Draw && exitDraw < composite.find("Gen4SharedEditorSurface::draw(*this, fb)"));

    const auto overlay = read("src/UI/ClassicStagedExitOverlay.inc");
    for (const char* hook : {"Gen1PokemonEditor::openReviewFromExit", "Gen2PokemonEditor::openReviewFromExit",
                             "Gen3SharedEditorSurface::openReviewFromExit", "Gen4SharedEditorSurface::openReviewFromExit",
                             "Gen1PokemonEditor::exportFromExit", "Gen2PokemonEditor::exportFromExit",
                             "Gen3SharedEditorSurface::exportFromExit", "Gen4SharedEditorSurface::exportFromExit"})
        assert(overlay.find(hook) != std::string::npos);
    for (const char* file : {"src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", "src/UI/Gen2SharedPokemonSurface.inc",
                             "src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc"}) {
        const auto text = read(file);
        assert(text.find("bool openReviewFromExit(TrainerViewScreen& screen)") != std::string::npos);
        assert(text.find("bool exportFromExit(TrainerViewScreen& screen)") != std::string::npos);
        // Both the Review's B and its Discard All consult the exit session.
        std::size_t uses = 0;
        for (auto at = text.find("returnFromReview("); at != std::string::npos; at = text.find("returnFromReview(", at + 1))
            ++uses;
        assert(uses == 2);
    }
}

} // namespace

int main() {
    testButtonsAndChords();
    testSessionFlow();
    testTrainerOnlySession(GS, G2::SourceGame::Gold, "gold_gbc");
    testTrainerOnlySession(GS, G2::SourceGame::Silver, "silver_gbc");
    testTrainerOnlySession(C, G2::SourceGame::Crystal, "crystal_gbc");
    testRouting();
    std::cout << "Classic Gen I-IV staged exit menu: Review / Continue / Discard & Exit / Export Copy, "
                 "Trainer-only sessions (NEW-19) PASS\n";
    return 0;
}
