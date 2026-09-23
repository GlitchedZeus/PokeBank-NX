#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedPokemonShell.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/Gen3EncounterPickerModel.h"
#include "UI/ClassicPackedMultiSelect.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>

std::string read(const char* path) {
    std::ifstream in(path); assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
void contains(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing surface contract: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
int main() {
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    namespace Multi = PokeBank::UIModel::ClassicPackedMove;
    constexpr UI::SharedPokemonShell::Geometry geometry(1280, 720, 46);
    namespace HeldItems = PokeBank::UIModel::SharedHeldItemPicker;
    static_assert(HeldItems::columns == 4 && HeldItems::rows == 10 && HeldItems::pageSize == 40);
    static_assert(HeldItems::futureGenerationsUseSharedGrid());
    namespace EncounterPicker = PokeBank::UIModel::Gen3EncounterPicker;
    namespace Encounter = PokeVault::Integration::EncounterGuardrails;
    const std::vector<Encounter::EncounterTemplate> pickerChoices{
        {"sapphire_gba", 116, 16, 5, 10, Encounter::Method::Surf, 0},
        {"sapphire_gba", 116, 99, 25, 30, Encounter::Method::SuperRod, 0},
    };
    const auto pickerRows = EncounterPicker::rowValues(pickerChoices.size());
    assert((pickerRows == std::vector<uint16_t>{0, 1}));
    assert(EncounterPicker::selectedRow(pickerChoices, 99, 27) == 1);
    assert(EncounterPicker::choiceAt(pickerChoices, pickerRows[1])->location == 99);
    assert(EncounterPicker::choiceAt(pickerChoices, 99) == nullptr);
    static_assert(geometry.leftX == 24 && geometry.leftW == 300 && geometry.midX == 338);
    static_assert(geometry.midW == 398 && geometry.rightX == 750 && geometry.rightW == 506);
    static_assert(geometry.y + geometry.h == 666);
    // Execute the actual shared navigation/window model through every Gen III Details row.
    Shared::Focus focus{Shared::Panel::Details, 0, 0};
    for (int row = 0; row < 15; ++row) {
        assert(focus.row == row);
        const auto window = Shared::scrollWindow(15, 8, focus.row);
        assert(window.scrolls && window.count == 8);
        assert(window.first <= focus.row && focus.row < window.first + window.count);
        assert(window.first + window.count <= 15);
        // Last value baseline fits the Details panel; sprite/type header remains above it.
        assert(224 + int(window.count - 1) * 38 + 18 < geometry.h - 30);
        if (row < 14) focus = Shared::moveVertical(Shared::Generation::Gen3, focus, 1);
    }
    const auto lastDetails = Shared::detailsScrollFocus(focus, 0);
    focus = Shared::switchPanel(Shared::Generation::Gen3, focus, 1);
    assert(Shared::detailsScrollFocus(focus, lastDetails) == 14);
    const auto rectangle = Multi::rectangleSlots(14, 0, 30);
    const int expected[] = {0,1,2,6,7,8,12,13,14};
    assert(rectangle.count == 9);
    for (unsigned i = 0; i < rectangle.count; ++i) assert(rectangle[i] == expected[i]);
    assert(Multi::moveCursor(29, Multi::Direction::Right, 30) == 29);
    assert(Multi::moveCursor(29, Multi::Direction::Down, 30) == 29);
    // D-pad horizontal navigation crosses shared panels without requiring L/R.
    assert(Shared::moveColumn(Shared::Generation::Gen3, {Shared::Panel::Details, 0, 0}, 1).panel ==
           Shared::Panel::Values);
    assert((Shared::moveColumn(Shared::Generation::Gen3, {Shared::Panel::Values, 0, 1}, 1) ==
            Shared::Focus{Shared::Panel::Moves, 0, 0}));
    assert((Shared::moveColumn(Shared::Generation::Gen3, {Shared::Panel::Moves, 0, 0}, -1) ==
            Shared::Focus{Shared::Panel::Values, 0, 1}));
    assert((Shared::moveColumn(Shared::Generation::Gen3, {Shared::Panel::Values, 0, 0}, -1) ==
            Shared::Focus{Shared::Panel::Details, 0, 0}));

    // Passive View selects only the move row. PP and PP Ups remain display-only.
    assert((Shared::passiveViewMoveColumn(
                Shared::Generation::Gen3, {Shared::Panel::Moves, 2, 0}, 1) ==
            Shared::Focus{Shared::Panel::Moves, 2, 0}));
    assert((Shared::normalizePassiveViewFocus(
                Shared::Generation::Gen3, {Shared::Panel::Moves, 2, 2}) ==
            Shared::Focus{Shared::Panel::Moves, 2, 0}));
    // Edit/Create outer workspaces also keep the cursor on the move name.
    assert((Shared::moveRowColumn(
                Shared::Generation::Gen3, {Shared::Panel::Moves, 2, 0}, 1) ==
            Shared::Focus{Shared::Panel::Moves, 2, 0}));

    // Production routing and renderer wiring (Switch compilation is a separate native gate).
    const auto surface = read("src/UI/Gen3SharedPokemonSurface.inc");
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto move = read("src/UI/ClassicPackedMoveOverlay.inc");
    const auto bridge = read("src/Legacy/FRLGReadOnlyTrainer.cpp");
    const auto shell = read("include/UI/SharedPokemonShell.h");
    const auto gen2 = read("src/UI/Gen2HardwareFinalFix.inc");
    contains(surface, "beginPassiveView(screen)");
    contains(surface, "if (workspaceActive(state)) return state.passive");
    contains(surface, "screen.detailsTargetPokemon()"); // Box, Party and Bank exact target resolver
    contains(surface, "target->getGameGroup() != Enums::GameVersion::FRLG");
    contains(surface, "state.session.begin(*record, SessionModel::Mode::View)");
    contains(surface, "screen.closeDetailsModal()");
    contains(surface, "if (!(down & HidNpadButton_A) || !state.session.editable()) return true");
    contains(surface, "{\"D-pad\", \"Navigate\"}, {\"L/R\", \"Panel\"}, {\"B\", \"Back\"}");
    contains(surface, "PID-LINKED / SAFE EDIT");
    contains(surface, "PickerTarget::Nature");
    contains(surface, "state.session.cycleGender()");
    assert(surface.find("openValuePicker(screen, state, PickerTarget::Gender)") == std::string::npos);
    contains(surface, "PickerTarget::Ability");
    contains(surface, "SessionModel::nativeAbilitySlots(state.session.working.species)");
    contains(surface, "target != PickerTarget::Ability &&");
    contains(surface, "state.session.setShiny");
    // Gen III must use the same shared D-pad/left-stick navigation path as Gen I/II.
    contains(surface, "screen.controllerNavigation.apply(");
    contains(surface, "down, held, stickX, stickY");
    contains(surface, "handleWorkspace(screen, state, navigated)");
    contains(surface, "handleActions(screen, state, navigated)");
    assert(surface.find("(void)stickX") == std::string::npos);
    assert(surface.find("(void)stickY") == std::string::npos);
    // Gen III species picker matches the accepted Gen I shiny-preview interaction:
    // Create and Edit both preview on Y, A commits Species + Shiny, B cancels.
    contains(surface, "state.speciesPreviewShiny = p.shiny");
    contains(surface, "if (down & HidNpadButton_Y)");
    contains(surface, "state.speciesPreviewShiny = !state.speciesPreviewShiny");
    contains(surface, "state.session.setShiny(state.speciesPreviewShiny)");
    assert(surface.find("state.session.mode == SessionModel::Mode::Create && (down & HidNpadButton_Y)") == std::string::npos);
    contains(surface, "state.speciesPreview, 386, state.speciesPreviewShiny");
    contains(surface, "{{\"D-pad\", \"Browse\"}, {\"Y\", \"Shiny\"}, {\"A\", \"Choose\"}, {\"B\", \"Cancel\"}}");
    contains(surface, "PID (read-only)");
    contains(surface, "Trainer ID");
    contains(surface, "down & (HidNpadButton_Left | HidNpadButton_L)");
    contains(surface, "down & (HidNpadButton_Right | HidNpadButton_R)");
    contains(surface, "if (row == 1 || row == 2 || row == 4 || row == 5) return true;");
    contains(surface, "p.tid = static_cast<uint16_t>(r.value)"); contains(surface, "SID (read-only)");
    contains(surface, "SharedPokemonShell::drawChrome");
    contains(gen2, "SharedPokemonShell::drawChrome");

    // Create/Edit Back uses PKSE's established New Pokemon / Unsaved changes dialog chrome.
    contains(surface, "Dialogs::drawDialogFrame");
    contains(surface, "\"New Pokémon\"");
    contains(surface, "\"Unsaved changes\"");
    contains(surface, "\"B\", \"Back\"");
    contains(surface, "\"Y\", \"Discard\"");
    contains(surface, "create ? \"Keep\" : \"Save\"");
    const auto exitInputBegin = surface.find("if (state.session.confirmExit)");
    const auto exitInputEnd = surface.find("const auto panelBeforeNavigation", exitInputBegin);
    assert(exitInputBegin != std::string::npos && exitInputEnd != std::string::npos &&
           exitInputEnd > exitInputBegin);
    const auto exitInput = surface.substr(exitInputBegin, exitInputEnd - exitInputBegin);
    assert(exitInput.find("HidNpadButton_Y") != std::string::npos);
    assert(exitInput.find("HidNpadButton_X") == std::string::npos);
    contains(surface, "SharedPokemonShell::Geometry"); contains(gen2, "SharedPokemonShell::Geometry");
    contains(surface, "SharedPokemonShell::drawPortrait"); contains(surface, "getTypeSprite");
    contains(surface, "SharedPokemonShell::drawDataAndGraph");
    contains(shell, "BATTLE STATS"); contains(shell, "StatsRadar::drawGen3Labeled");
    contains(surface, "GEN III DATA"); contains(surface, "Colors::Panel");
    contains(surface, "SharedPokemonShell::drawScrollableDetails");
    contains(shell, "scrollWindow(total, visibleRows, focus)");
    contains(shell, "visibleRows = 8");
    contains(shell, "labelWidth + 28");
    contains(shell, "fitDetailsText");
    contains(shell, "drawVerticalScrollIndicator");
    contains(shell, "viewportY = y + 216");
    contains(shell, "fb.setClipRect");
    contains(shell, "fb.clearClip()");
    contains(surface, "previewCreate"); contains(surface, "previewEdit");
    contains(surface, "SessionModel::sameEditableRecord");
    contains(surface, "Encounter::forGameSpeciesWithGen3Provenance");
    contains(surface, "EncounterPicker::rowValues");
    contains(surface, "EncounterPicker::selectedRow");
    contains(surface, "EncounterPicker::choiceAt");
    contains(surface, "forGameSpeciesWithGen3Provenance");
    contains(surface, "Encounter origin");
    contains(surface, "origin →");
    assert(surface.find("state.pickerValues.push_back(current)") != std::string::npos);
    assert(surface.find("No supported exact-game encounter templates for this Pokemon") != std::string::npos);
    contains(surface, "Met Level for selected encounter");
    contains(surface, "state.session.setEncounterMetLevel(choice->encounter.minLevel)");
    contains(surface, "state.session.setEncounterMetLevel(static_cast<uint8_t>(r.value))");
    contains(surface, "only native Generation III ability");
    contains(surface, "normalizeEditableFocus(state");
    contains(surface, "state.focus = Shared::normalizeMoveRowFocus");
    contains(surface, "Shared::moveRowColumn");
    contains(surface, "Shared::passiveViewMoveColumn");
    contains(surface, "Shared::normalizePassiveViewFocus");
    contains(surface, "visibleMoveFocus = Shared::normalizeMoveRowFocus");
    contains(surface, "Shared::moveRowFocus(rightW)");
    contains(surface, "beginMoveEditor(screen, state, static_cast<int>(state.focus.row))");
    const auto contextStart = surface.find("bool handleMoveEditor(");
    const auto contextEnd = surface.find("bool handleWorkspace(", contextStart);
    assert(contextStart != std::string::npos && contextEnd > contextStart);
    const auto contextualMoveEditor = surface.substr(contextStart, contextEnd - contextStart);
    assert(contextualMoveEditor.find("normalizeMoveRowFocus") == std::string::npos);
    contains(contextualMoveEditor, "state.moveEditorRow == 0");
    contains(contextualMoveEditor, "state.moveEditorRow == 1");
    contains(contextualMoveEditor, "state.moveEditorRow == 2");
    contains(surface, "beginMoveEditor");
    contains(surface, "drawMoveEditor");
    contains(surface, "Contextual editor — B always cancels this dialog");
    contains(surface, "Keep these move details");
    contains(surface, "Discard these move details");
    contains(surface, "000 - Empty move slot");
    contains(surface, "openValuePicker(screen, state, PickerTarget::Move, state.moveEditorSlot)");
    contains(surface, "state.pickerTarget == PickerTarget::HeldItem");
    contains(surface, "HeldItemGrid::move");
    contains(surface, "HeldItemGrid::columns");
    contains(surface, "HeldItemGrid::pageSize");
    contains(surface, "{\"D-pad/Stick\", \"Navigate\"}, {\"L/R\", \"Page\"}");
    assert(surface.find("Add Pokemon") != std::string::npos); // Box action still exists.
    const auto moveEditorBegin = surface.find("void drawMoveEditor(");
    const auto workspaceBegin = surface.find("void drawWorkspace(", moveEditorBegin);
    assert(moveEditorBegin != std::string::npos && workspaceBegin > moveEditorBegin);
    contains(surface, "state.session.editable() &&");
    contains(surface, "screen.drawGSCOverlay(fb)");
    contains(surface, "Game/species/location/level constrained");
    const auto actionsBegin = surface.find("void drawActions(");
    const auto actionsEnd = surface.find("void drawReview(", actionsBegin);
    assert(actionsBegin != std::string::npos && actionsEnd != std::string::npos && actionsEnd > actionsBegin);
    const auto actionsBody = surface.substr(actionsBegin, actionsEnd - actionsBegin);
    assert(actionsBody.find("screen.drawGSCOverlay(fb)") != std::string::npos);
    assert(actionsBody.find("Colors::Background") == std::string::npos);
    assert(actionsBody.find("const int w = occupied ? 650 : 560") != std::string::npos);
    assert(actionsBody.find("const int h = occupied ? 500 : 350") != std::string::npos);
    assert(actionsBody.find("Shared::actionMenuGeometry()") != std::string::npos);
    assert(actionsBody.find("Colors::Divider, 1") != std::string::npos);
    assert(actionsBody.find("Colors::FocusBorder, 2") == std::string::npos);
    assert(actionsBody.find("constexpr int x = 250, y = 72, w = 780, h = 560") == std::string::npos);
    contains(composite, "return !Gen3SharedEditorSurface::ownsFrame(screen)");
    assert(composite.find("ClassicPackedMove::handleInput") < composite.find("Gen3SharedEditorSurface::handleInput"));
    contains(move, "Move / Hold Multi"); contains(move, "Hold Multi");
    contains(move, "kMultiHoldFrames"); contains(move, "rectangleSlots");
    contains(move, "beginSparseMove"); contains(move, "placeSparseMove"); contains(move, "cancelSparseMove");
    // Sparse carry presentation reads validated staged bytes so source holes can be shown,
    // while finalizedBytes remains blocked by the backend until place/cancel.
    contains(bridge, "const auto& bytes = stagedPokemon_->stagedBytes()");
    const auto refreshStart = bridge.find("bool FRLGReadOnlyTrainer::refreshStagedPokemonPresentation");
    const auto refreshEnd = bridge.find("bool FRLGReadOnlyTrainer::populate", refreshStart);
    assert(refreshStart != std::string::npos && refreshEnd > refreshStart);
    assert(bridge.substr(refreshStart, refreshEnd - refreshStart).find("finalizedBytes") == std::string::npos);
    contains(move, "beginPackedMove"); contains(move, "placePackedGroupMove"); // accepted packed backend remains
    contains(move, "if (isGen3(screen)) return beginGen3");
    contains(move, "The selected sparse rectangle contains no Pokemon");
    contains(move, "if (state.active)");
    contains(move, "if (down & HidNpadButton_B) return cancel(screen, state)");
    contains(move, "if (down & HidNpadButton_Y) return place(screen, state)");
    contains(move, "down & (HidNpadButton_A | HidNpadButton_X)");
    contains(move, "D-pad and L/R keep using the accepted box navigation path");
    contains(move, "screen.currentlySelecting = true");
    contains(move, "setLegacyHoldingPresentation(screen, true)");
    contains(move, "drawVisual(fb");
    contains(surface, "state.previewRecord");
    contains(surface, "p.calculatedStats");
    std::cout << "Gen III hardware routing, shared shell, Details scroll and Y contract: PASS\n";
}
