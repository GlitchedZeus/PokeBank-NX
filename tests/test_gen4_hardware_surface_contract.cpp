#include "UI/SharedPokemonEditorContract.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void contains(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos)
        std::cerr << "Missing Gen IV surface contract: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
}

int main() {
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    static_assert(Shared::genderUsesInlineToggle(Shared::Generation::Gen4));
    static_assert(!Shared::genderOpensDedicatedPicker(Shared::Generation::Gen4));

    // Immutable external sources still need the staged dirty-session exit confirmation.
    static_assert(Shared::immutableSourceBlocksSaveDialog(true, false));
    static_assert(!Shared::immutableSourceBlocksSaveDialog(true, true));
    static_assert(!Shared::immutableSourceBlocksSaveDialog(false, false));

    // Gen IV has six native IV/EV rows. The sixth row must stay a column-selectable
    // IV/EV cell, not collapse into the older-generation full-width fallback row.
    constexpr auto gen4SixthIv = Shared::cellFocusFor(
        Shared::Generation::Gen4, {Shared::Panel::Values, 5, 1});
    static_assert(gen4SixthIv.x == 180 && gen4SixthIv.width == 90);
    constexpr auto gen2SixthFallback = Shared::cellFocusFor(
        Shared::Generation::Gen2, {Shared::Panel::Values, 5, 1});
    static_assert(gen2SixthFallback.x == 130 && gen2SixthFallback.width == 244);

    // Gen IV has six editable IV/EV rows; Sp. Def must retain distinct IV vs EV focus rectangles.
    constexpr Shared::Focus spDefIv{Shared::Panel::Values, 5, 0};
    constexpr Shared::Focus spDefEv{Shared::Panel::Values, 5, 1};
    constexpr auto spDefIvFocus = Shared::cellFocusFor(Shared::Generation::Gen4, spDefIv);
    constexpr auto spDefEvFocus = Shared::cellFocusFor(Shared::Generation::Gen4, spDefEv);
    static_assert(spDefIvFocus.x == 110 && spDefIvFocus.width == 64);
    static_assert(spDefEvFocus.x == 180 && spDefEvFocus.width == 90);
    static_assert(spDefIvFocus.x != spDefEvFocus.x || spDefIvFocus.width != spDefEvFocus.width);

    // Simulate the two-frame immutable-source dirty-exit path that the first source-only contract
    // missed: B opens the exit-only prompt, a neutral frame must not clear it, then A may discard.
    bool saveConfirmActive = true;
    bool exitingWithUnsavedChanges = true;
    if (Shared::immutableSourceBlocksSaveDialog(saveConfirmActive, exitingWithUnsavedChanges))
        saveConfirmActive = false;
    assert(saveConfirmActive); // neutral frame preserved the exit-only confirmation
    bool goBack = false;
    if (saveConfirmActive && exitingWithUnsavedChanges) {
        saveConfirmActive = false;
        exitingWithUnsavedChanges = false;
        goBack = true;
    }
    assert(goBack && !saveConfirmActive && !exitingWithUnsavedChanges);

    const auto surface = read("src/UI/Gen4SharedPokemonSurface.inc");
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto bridge = read("src/Legacy/Gen4ReadOnlyTrainer.cpp");
    const auto staged = read("src/Integration/Gen4/Gen4StagedPokemonEditor.cpp");
    const auto session = read("include/UI/Gen4SharedPokemonSession.h");
    const auto baseUi = read("src/UI/TrainerViewScreenBase.inc");
    const auto trainerBase = read("include/Trainer/Trainer.h");
    const auto rbyBridge = read("include/Legacy/RBYReadOnlyTrainer.h");
    const auto gscBridge = read("include/Legacy/GSCReadOnlyTrainer.h");
    const auto gen3Bridge = read("include/Legacy/FRLGReadOnlyTrainer.h");
    const auto gen4Bridge = read("include/Legacy/Gen4ReadOnlyTrainer.h");

    contains(surface, "SharedPokemonShell::drawChrome");
    contains(surface, "SharedPokemonShell::Geometry");
    contains(surface, "SharedPokemonShell::drawScrollableDetails");
    contains(surface, "SharedPokemonShell::drawDataAndGraph");
    contains(surface, "Shared::normalizeMoveRowFocus(Shared::Generation::Gen4");
    contains(surface, "Shared::moveRowColumn(Shared::Generation::Gen4");
    contains(surface, "Shared::passiveViewMoveColumn(Shared::Generation::Gen4");
    contains(surface, "Shared::moveRowFocus(g.rightW)");

    // Gen IV inherits the accepted D-pad / left-stick repeat navigation path.
    contains(surface, "screen.controllerNavigation.apply(");
    contains(surface, "down, held, stickX, stickY");

    // Gender is a one-press inline toggle, never a nested Male/Female picker.
    contains(surface, "state.session.cycleGender()");
    assert(surface.find("PickerTarget::Gender") == std::string::npos);
    assert(surface.find("Choose Gender") == std::string::npos);

    // Nature and Ability use bounded pickers; Shiny is another direct field action.
    contains(surface, "PickerTarget::Nature");
    contains(surface, "PickerTarget::Ability");
    contains(surface, "state.session.cycleShiny()");
    contains(surface, "Could not preserve the other PID-linked Generation IV traits");

    // Outer move focus is exactly one row. PP / PP Ups live in the contextual dialog.
    contains(surface, "openMoveEditor(screen, state, state.focus.row)");
    contains(surface, "Same shared contextual editor");
    contains(surface, "PP Ups");
    contains(surface, "state.moveEditorRow = 0; // Shared move editor opens on the Move row");
    contains(surface, "state.moveEditorRow == 1");
    contains(surface, "state.moveEditorRow == 2");
    contains(surface, "state.session.working->setMove(slot, value)");
    contains(surface, "PickerTarget::Move");
    contains(surface, "Names::isMovePresent");
    contains(surface, "move <= 467");
    contains(surface, "Native Gen IV move catalog");
    assert(surface.find("move choice read-only in G4-03") == std::string::npos);

    // G4-04 keeps accepted View/Edit and adds native Create for empty PC slots.
    contains(surface, "result.values[result.count++] = Shared::Action::View");
    contains(surface, "result.values[result.count++] = Shared::Action::Edit");
    contains(surface, "result.values[result.count++] = Shared::Action::Add");
    contains(surface, "case Shared::Action::Add");
    contains(surface, "beginCreate(screen)");
    contains(surface, "PickerTarget::Species");
    contains(surface, "PickerTarget::HeldItem");
    contains(surface, "PickerTarget::Language");
    contains(surface, "PickerTarget::Ball");
    contains(surface, "PickerTarget::MetLocation");
    contains(surface, "PickerTarget::Pokerus");
    contains(surface, "PickerTarget::Form");
    contains(surface, "openFormPicker");
    contains(surface, "auto probe = *state.session.working");
    contains(surface, "probe.setForm");
    contains(surface, "gen4FormLabel");
    contains(surface, "state.session.working->setForm");
    contains(surface, "Names::isGen4HeldItemPresent");
    contains(surface, "Names::getLocationTable");
    contains(surface, "Enums::getBallList");
    contains(surface, "createBoxDraft(");
    contains(surface, "keepCreate(");
    contains(session, "Mode::Create");
    contains(session, "Guard::Create");
    contains(staged, "stageCreateBoxPokemon");
    assert(surface.find("Generation IV Create follows after Edit hardware acceptance") == std::string::npos);
    assert(surface.find("Create is not enabled in the Edit milestone") == std::string::npos);
    assert(surface.find("result.values[result.count++] = Shared::Action::Clone") == std::string::npos);
    assert(surface.find("result.values[result.count++] = Shared::Action::Remove") == std::string::npos);
    contains(surface, "partyEntrySurface");
    contains(surface, "TargetKind::Party");
    contains(surface, "keepParty");
    contains(surface, "beginPassiveView");
    contains(surface, "screen.closeDetailsModal()");

    // Visible read-only rows remain navigable in Edit/Create rather than being silently skipped.
    assert(surface.find("if (detailEditable(state.focus.row)) return;") == std::string::npos);
    contains(surface, "This Generation IV field is visible but read-only in the current G4-04 slice");
    contains(surface, "state.focus.panel == Shared::Panel::Values &&");
    contains(surface, "state.focus.row == row;");

    // A committed staged edit must not claim UI success when the refreshed presentation failed.
    contains(surface, "if (!refreshPresentation(screen))");
    contains(surface, "Generation IV change is staged, but presentation refresh failed");

    // Dirty Back uses the same shared exit guard and explicit A/Y/B confirmation.
    contains(session, "PokemonEditorExitGuard::requiresConfirmation");
    contains(surface, "if (state.session.confirmExit)");
    contains(surface, "HidNpadButton_Y");
    contains(surface, "Dialogs::drawDialogFrame");
    contains(surface, "\"Unsaved changes\"");
    contains(surface, "\"Y\", \"Discard\"");

    // Staged trainer bridge exists, but the immutable source stays owned separately.
    contains(bridge, "Gen4StagedPokemonEditor::create(");
    contains(bridge, "save.sourceBytes()");
    contains(bridge, "refreshStagedPokemonPresentation");

    // Invalid unrelated Gen IV records remain quarantined instead of making a valid
    // staged edit to another Pokémon fail presentation refresh.
    contains(bridge, "displayParty.push_back(nullptr);");
    contains(bridge, "if (!pokemon.valid() || pokemon.empty()) continue;");
    contains(bridge, "displayParty.push_back(nullptr)");
    contains(bridge, "if (!pokemon.valid() || pokemon.empty()) continue;");
    contains(surface, "if (!refreshPresentation(screen))");
    contains(staged, "original_");
    contains(staged, "staged_");
    contains(staged, "refreshStorageCrc");
    contains(staged, "refreshGeneralCrc");
    contains(staged, "commitPartyPokemon");
    contains(staged, "Gen4ReadOnlySave::parse(");
    assert(staged.find("std::fopen") == std::string::npos);
    assert(staged.find("std::fwrite") == std::string::npos);
    assert(staged.find("rename(") == std::string::npos);

    // Save-level B/+ exit guards protect staged work even when the external source is immutable.
    contains(trainerBase, "virtual bool hasStagedChanges() const noexcept { return false; }");
    contains(rbyBridge, "hasStagedChanges() const noexcept override");
    contains(gscBridge, "hasStagedChanges() const noexcept override");
    contains(gen3Bridge, "hasStagedChanges() const noexcept override");
    contains(gen4Bridge, "hasStagedChanges() const noexcept override");
    contains(baseUi, "hasUnsavedChanges || trainer.hasStagedChanges()");
    contains(baseUi, "immutableSourceBlocksSaveDialog(");
    contains(baseUi, "exitOnlySaveConfirm");
    assert(baseUi.find("!sourceReadOnly() && hasUnsavedChanges") == std::string::npos);
    assert(baseUi.find("!sourceReadOnly() && bank && bank->hasChanged()") == std::string::npos);

    contains(baseUi, "immutableSourceBlocksSaveDialog(");
    contains(baseUi, "const bool exitOnlySaveConfirm = saveConfirmActive && exitingWithUnsavedChanges;");
    assert(baseUi.find("statEdit.dialogActive = saveConfirmActive") == std::string::npos);

    // Final composite routes Gen IV before generic/Gen III fallback.
    contains(composite, "#include \"Gen4SharedPokemonSurface.inc\"");
    const auto update = composite.find("void TrainerViewScreen::update");
    const auto g4input = composite.find("Gen4SharedEditorSurface::handleInput", update);
    const auto g3input = composite.find("Gen3SharedEditorSurface::handleInput", update);
    assert(update != std::string::npos && g4input > update && g3input > g4input);
    const auto draw = composite.find("void TrainerViewScreen::draw(PKSEFramebuffer& fb)");
    const auto g4draw = composite.find("Gen4SharedEditorSurface::draw", draw);
    const auto g3draw = composite.find("Gen3SharedEditorSurface::draw", draw);
    assert(draw != std::string::npos && g4draw > draw && g3draw > g4draw);

    std::cout << "Gen IV shared editor hardware surface contract PASS\n";
    return 0;
}
