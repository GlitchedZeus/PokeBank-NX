#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "Gen II editor foundation source must exist");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    const auto foundation = readFile("src/UI/Gen2PokemonEditorFoundation.inc");
    const auto unified = readFile("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    const auto parity = readFile("src/UI/Gen2SharedSurfaceParity.inc");
    const auto shared = readFile("src/UI/Gen2SharedPokemonSurface.inc");
    const auto composite = readFile("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto trainerPage = readFile("src/UI/Gen2TrainerSessionOverlay.inc");
    const auto pickerFix = readFile("src/UI/Gen2HardwarePickerFix.inc");
    const auto workspaceFix = readFile("src/UI/Gen2HardwareWorkspaceFix.inc");
    const auto passiveReleaseTouch = readFile("src/UI/ClassicReleaseActionFix.inc");
    const auto passiveBegin = passiveReleaseTouch.find("bool handlePassiveViewTouch(");
    const auto passiveEnd = passiveReleaseTouch.find("} // namespace", passiveBegin);
    assert(passiveBegin != std::string::npos && passiveEnd > passiveBegin);
    const auto passiveTouch = passiveReleaseTouch.substr(passiveBegin, passiveEnd - passiveBegin);
    assert(passiveTouch.find("Unified::scrollWindow") == std::string::npos);
    assert(passiveTouch.find("unified.detailsTouchScroll.updateVertical(") != std::string::npos);
    assert(passiveTouch.find("tapped >= 1700 && tapped < 1700 + detailCount") != std::string::npos);
    assert(passiveTouch.find("touch.justTouchedDown()") == std::string::npos);

    const auto finalFix = readFile("src/UI/Gen2HardwareFinalFix.inc");
    const auto legacyOverlay = readFile("src/UI/TrainerViewScreenGSCOverlay.inc");
    assert(legacyOverlay.find("const u64 heldButtons = padGetButtons(&pad)") != std::string::npos);
    assert(legacyOverlay.find("const HidAnalogStickState stick = padGetStickPos(&pad, 0)") != std::string::npos);
    assert(legacyOverlay.find("const u64 navigated = controllerNavigation.apply(") != std::string::npos);
    assert(legacyOverlay.find("handleStagedEditorInput(*this, navigated, touch)") != std::string::npos);
    const auto picker = readFile("src/UI/Gen2PokemonPickerOverlay.inc");
    // Both paging directions still update selection, but now stop active
    // touch inertia first so physical controls cannot fight the viewport.
    // Paging follows the shared list scheme (Gen13PickerPaging.h) and stops touch inertia first.
    assert(picker.find("if (keys.any() && listCount > 0) {\n            picker.scroll.stop();\n            picker.model.index = PokeBank::UIModel::SharedPickerPaging::pagedRow(") != std::string::npos);
    const auto pickerModel = readFile("include/UI/Gen2PokemonPickerModel.h");
    const auto nativePresentation = readFile("include/UI/Gen2NativePresentation.h");
    const auto sharedShell = readFile("include/UI/SharedPokemonShell.h");
    const auto heldItemModel = readFile("include/UI/Gen2HeldItemPicker.h");

    // Active Create/Edit routes consume Species A and picker cancel/accept before box actions.
    const auto editorGuard = parity.find("if (stateFor(screen).mode != Mode::None)", parity.find("bool handleFinalGen2SurfaceInput"));
    const auto boxRoute = parity.find("namespace Access", editorGuard);
    assert(editorGuard < boxRoute);
    const auto dispatch = parity.substr(editorGuard, boxRoute - editorGuard);
    assert(dispatch.find("handlePickerInput(screen, down, held, stickX, stickY, touch)") != std::string::npos);
    assert(dispatch.find("return true; // Editor Species") != std::string::npos);
    const auto release = readFile("src/UI/ClassicReleaseActionFix.inc");
    const auto gen2Release = release.substr(release.find("namespace UI::Gen2PokemonEditor"));
    assert(gen2Release.find("stateFor(screen).mode != Mode::None") != std::string::npos);
    const auto drawing = gen2Release.substr(gen2Release.find("bool drawReleaseActionSurface"));
    assert(drawing.find("screen.drawLegacyBase(fb)") < drawing.find("fb.drawFilledRect"));
    // Hardware regression: neither Pokemon Actions / Empty Slot nor Release confirmation may
    // focus-outline the entire modal. Only the selected row/action owns the theme focus color.
    assert(drawing.find("fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2)") == std::string::npos);
    assert(drawing.find("drawNarrowPokemonActions(fb, screen.touchButtons, model, legacy.pokemonRow") != std::string::npos);
    assert(drawing.find("ClassicPokemonActionMenu::fromShared(actions)") != std::string::npos);
    assert(drawing.find("screen.legacyProviderLabel() + \" GBC SOURCE / READ ONLY\"") != std::string::npos);
    assert(drawing.find("Names::getSpeciesName(pokemon->species)") != std::string::npos);
    assert(drawing.find("drawPanelSurface(fb, x, y, w, h, false, 18)") != std::string::npos); // confirmation only
    assert(drawing.find("fb.drawSelectionHighlight(x + 22, rowY, w - 44, geometry.rowHeight)") == std::string::npos);
    const auto input = gen2Release.substr(gen2Release.find("bool handleReleaseActionInput"));
    const auto close = input.substr(input.find("if (down & HidNpadButton_B)", input.find("const uint64_t nav")));
    const auto closeBody = close.substr(0, close.find("return true;"));
    assert(closeBody.find("legacy.active = false") != std::string::npos);
    assert(closeBody.find("selectedBoxIndex") == std::string::npos);
    assert(closeBody.find("selectedItemIndex") == std::string::npos);
    assert(closeBody.find("stage") == std::string::npos);
    const auto boxBase = readFile("src/UI/TrainerViewScreenBase.inc");
    // Source identity is now a human-readable provider / game / immutable
    // status banner; the old debug-style concatenation is intentionally gone.
    assert(boxBase.find("legacyProviderLabel() + \"  •  \" + titleName + \"  •  Read-only source\"") != std::string::npos);
    assert(boxBase.find("if (!legacyReadOnlySource() && !trainer.saveRevisionString.empty()") != std::string::npos);
    assert(foundation.find("Gen2HeldItemPicker::initialIndex(state.itemChoices, state.working.heldItem)") != std::string::npos);
    assert(heldItemModel.find("#include \"UI/SharedHeldItemPicker.h\"") != std::string::npos);
    assert(heldItemModel.find("columns = SharedHeldItemPicker::columns") != std::string::npos);
    assert(heldItemModel.find("rows = SharedHeldItemPicker::rows") != std::string::npos);
    assert(heldItemModel.find("pageSize = SharedHeldItemPicker::pageSize") != std::string::npos);
    const auto cancel = foundation.substr(foundation.find("bool handleItemPicker"));
    assert(cancel.find("if (down & HidNpadButton_B)") < cancel.find("state.working.heldItem ="));

    assert(foundation.find("Gen II Level") != std::string::npos);
    assert(foundation.find("Gen II Experience") != std::string::npos);
    const auto directProgression = unified.substr(unified.find("bool activateUnifiedField"),
        unified.find("bool handleUnifiedWorkspaceInput") - unified.find("bool activateUnifiedField"));
    assert(directProgression.find("beginProgressionEditor(screen)") == std::string::npos);
    assert(directProgression.find("\"Generation II Level\"") != std::string::npos);
    assert(directProgression.find("\"Generation II Experience\"") != std::string::npos);
    assert(directProgression.find("state.setLevel(requested)") != std::string::npos);
    assert(directProgression.find("state.setExperience(experience)") != std::string::npos);

    assert(foundation.find("state.baseline = *pokemon") != std::string::npos);
    assert(foundation.find("state.working = *pokemon") != std::string::npos);
    assert(foundation.find("Dialogs::drawDialogFrame") != std::string::npos);
    assert(foundation.find("\"New Pokémon\"") != std::string::npos);
    assert(foundation.find("\"Unsaved changes\"") != std::string::npos);
    assert(foundation.find("\"B\", \"Back\"") != std::string::npos);
    assert(foundation.find("\"Y\", \"Discard\"") != std::string::npos);
    assert(foundation.find("create ? \"Keep\" : \"Save\"") != std::string::npos);
    assert(foundation.find("create ? stageCreate(screen) : stageEdit(screen)") != std::string::npos);
    assert(foundation.find("Create draft discarded; staged/source bytes unchanged") != std::string::npos);
    assert(foundation.find("This Edit session discarded; earlier staged work preserved") != std::string::npos);
    const auto confirmInputBegin = foundation.find("bool handleConfirm");
    const auto confirmInputEnd = foundation.find("} // namespace", confirmInputBegin);
    assert(confirmInputBegin != std::string::npos && confirmInputEnd != std::string::npos);
    const auto confirmInput = foundation.substr(confirmInputBegin, confirmInputEnd - confirmInputBegin);
    assert(confirmInput.find("HidNpadButton_Y") != std::string::npos);
    assert(confirmInput.find("HidNpadButton_X") == std::string::npos);

    assert(foundation.find("Add blocked: correct red incompatible move rows first") != std::string::npos);
    assert(foundation.find("Create draft cancelled; staged save unchanged") == std::string::npos);
    assert(unified.find("No local draft is lost on a stray Back press.") != std::string::npos);
    // The workspace footer (X Add / X Keep) now comes from the shared layer model.
    assert(unified.find("drawWorkspaceFooter(fb, screen);") != std::string::npos);
    assert(readFile("include/UI/Gen2WorkspaceLayers.h").find("\"X\", create ? \"Add\" : \"Keep\"") !=
           std::string::npos);
    assert(unified.find("Stage Add") == std::string::npos);
    assert(foundation.find("Pokemon added to staged copy only; source .srm unchanged") == std::string::npos);

    assert(foundation.find("refreshStagedBoxPresentation(error)") != std::string::npos);
    assert(foundation.find("closeToBoxAfterAdd(screen, destinationBox, static_cast<int>(slot))") != std::string::npos);
    assert(foundation.find("screen.selectedBoxIndex = target.box") != std::string::npos);
    assert(foundation.find("screen.selectedItemIndex = target.slot") != std::string::npos);
    assert(foundation.find("legacy.pokemonActions = false") != std::string::npos);
    assert(foundation.find("added to Box") != std::string::npos);

    assert(parity.find("SharedEditor::boxActivation") != std::string::npos);
    assert(parity.find("SharedEditor::BoxActivation::Add") != std::string::npos);
    assert(parity.find("SharedEditor::BoxActivation::Actions") != std::string::npos);
    assert(parity.find("if (down & HidNpadButton_X) return true") != std::string::npos);
    assert(composite.find("{\"A\", \"Actions\"}") != std::string::npos);
    assert(composite.find("{\"X\", \"Add\"}") != std::string::npos);
    assert(composite.find("{\"L/R\", \"Box\"}") != std::string::npos);
    assert(composite.find("drawGen2ClassicBoxFooter") != std::string::npos);

    assert(shared.find("caps.hasLegalityProvenance = true") != std::string::npos);
    assert(shared.find("SharedEditor::actionLabel(action)") != std::string::npos);
    assert(parity.find("ClassicPokemonActionMenu::fromShared(actions)") != std::string::npos);
    const auto actionsDraw = parity.substr(parity.find("void drawParityGen2Actions"),
        parity.find("void drawParityGen2Review") - parity.find("void drawParityGen2Actions"));
    assert(actionsDraw.find("screen.drawLegacyBase(fb)") != std::string::npos);
    assert(actionsDraw.find("screen.drawGSCOverlay(fb)") == std::string::npos);
    assert(actionsDraw.find("drawNarrowPokemonActions(fb, screen.touchButtons") != std::string::npos);
    const auto narrow = readFile("include/UI/NarrowPokemonActionMenu.h");
    assert(narrow.find("int width = 440") != std::string::npos);
    assert(narrow.find("onlySharedLabels(classic(") != std::string::npos); // no placeholder rows (Master Vault, Make Shiny)
    assert(actionsDraw.find("fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2)") == std::string::npos);
    // The composite wrapper must not paint the legacy staged-editor Accent shell first.
    const auto ownsFrame = parity.substr(parity.find("bool finalGen2SurfaceOwnsFrame"),
        parity.find("bool handleFinalGen2SurfaceInput") - parity.find("bool finalGen2SurfaceOwnsFrame"));
    assert(ownsFrame.find("legacy.active && legacy.pokemonActions && !legacy.review") != std::string::npos);
    const auto compositeDraw = composite.substr(composite.find("void TrainerViewScreen::draw(PKSEFramebuffer& fb)"));
    assert(compositeDraw.find("if (!Gen2PokemonEditor::finalGen2SurfaceOwnsFrame(*this))") != std::string::npos);
    assert(shared.find("drawSharedProvenanceSurface") != std::string::npos);
    assert(shared.find("UI::SharedEditorChrome::drawOriginLegalityPanel(") != std::string::npos);
    assert(shared.find("rows,P::LegalityState::NotChecked") != std::string::npos);
    assert(shared.find("case SharedEditor::Action::Close") != std::string::npos);

    // The established inline page owns rendering and exactly two cursor rows.
    assert(trainerPage.find("drawTrainerSession") == std::string::npos);
    assert(trainerPage.find("drawFilledRect") == std::string::npos);
    // Gen I and Gen II share one Trainer session flow (R2-TRAINER); its two-row cursor lives there.
    assert(trainerPage.find("UI::ClassicTrainerSession::handle(") != std::string::npos);
    assert(readFile("include/UI/ClassicTrainerSessionInput.h").find("screen.trainerSelectedRow = (screen.trainerSelectedRow + 1) % kRows") != std::string::npos);
    assert(trainerPage.find("editor->stageTrainerEdit(name, money, error)") != std::string::npos);
    assert(parity.find("trainerSessionFor") == std::string::npos);
    const auto base = readFile("src/UI/TrainerViewScreenBase.inc");
    assert(base.find("constexpr int kEditRows = 2") != std::string::npos);
    assert(unified.find("Unified::statsHeading()") != std::string::npos);
    assert(unified.find("Unified::cellFocus(unified.focus)") != std::string::npos);
    assert(unified.find("unified.detailsViewport.ensure(unified.focus.row, count, visible)") != std::string::npos);
    assert(unified.find("unified.detailsTouchScroll.offset()") != std::string::npos);
    assert(unified.find("appendClippedTouchButton(screen.touchButtons, 1700 + row") != std::string::npos);
    assert(unified.find("passive ? 0 : unified.detailsFocus") == std::string::npos);
    const auto summary = readFile("src/UI/Panels/BoxPokemonPanel.cpp");
    assert(summary.find("StatsRadar::drawGen2Labeled") != std::string::npos);
    assert(summary.find("Gen2Workspace::battleStats(record)") != std::string::npos);

    // Exact-format capability hygiene and Gen II Values remain truthful: descriptive native fields
    // are now in DETAILS, while VALUES keeps stats plus derived Shiny/Gender.
    assert(foundation.find("HP DV is derived") != std::string::npos);
    assert(unified.find("\"DV\"") != std::string::npos);
    assert(unified.find("\"Stat Exp\"") != std::string::npos);
    // Round 7 (owner Round 6 A22 / C01): the owner rejected squeezing tagged / slashed Special
    // values into the numeric STATS column. Gen I is the reference: plain numbers in the columns,
    // and SpA / SpD in the lower-middle "CALCULATED SPECIAL STATS" block below the grid. No
    // off-grid SpD line, no tags in the column. Round 11 (owner H22, deliberate change): the
    // Special row's Calculated cell shows the two numbers (Sp. Atk then Sp. Def) instead of nothing.
    assert(unified.find("drawGen2CalculatedColumn(fb, battle, x, y, w);") != std::string::npos);
    assert(unified.find("drawGen2SpecialBlock(fb, battle, x, y);") != std::string::npos);
    assert(unified.find("y + 246") == std::string::npos);
    assert(unified.find("Special DV / Stat Exp shared by SpA and SpD") == std::string::npos);
    const auto gen2Presentation = readFile("include/UI/Gen2WorkspacePresentation.h");
    assert(gen2Presentation.find("if (row >= 0 && row < kSpecialRow) return std::to_string(battle[static_cast<std::size_t>(row)]);") !=
           std::string::npos);
    assert(gen2Presentation.find("if (row == kSpecialRow) return std::to_string(battle[4]) + kSpecialCellGap + std::to_string(battle[5]);") !=
           std::string::npos);
    assert(unified.find("y + B::kCaptionY, \"CALCULATED SPECIAL STATS\"") != std::string::npos);
    // R2-GOLD: calculated stats are ReadOnly gold through the shared field-state contract.
    assert(unified.find("gen2StatCellColor(row, 2), TextStyle::Caption);") != std::string::npos);
    const auto valuesBegin = unified.find("void drawUnifiedValues");
    const auto valuesEnd = unified.find("void drawUnifiedMoveEditor", valuesBegin);
    assert(valuesBegin != std::string::npos && valuesEnd != std::string::npos && valuesEnd > valuesBegin);
    const auto valuesSurface = unified.substr(valuesBegin, valuesEnd - valuesBegin);
    assert(valuesSurface.find("unified.focus.column == 2 ? Colors::Accent") == std::string::npos);
    assert(unified.find("{\"Held Item\", itemText(p.heldItem)}") != std::string::npos);
    assert(unified.find("{\"Friendship\", std::to_string(p.friendship)}") != std::string::npos);
    assert(unified.find("{\"Pokerus\", Gen2Native::pokerusText(p.pokerus)}") != std::string::npos);
    assert(unified.find("std::array<std::pair<std::string, std::string>, 2> capabilityRows") != std::string::npos);
    // Origin & Legality moved from under the move rows into the Gen II Origin & Legality pane.
    assert(unified.find("drawGen2SharedMoveRows(") != std::string::npos);
    assert(unified.find("const int first = unified.detailsViewport.firstRow") != std::string::npos);
    // Touching a visually scrolled Details row must not invoke the old
    // independently recalculated focus-centred hit test. The 1700+ row
    // targets published by the actual clipped render own selection.
    const auto oldHardwareFocus = workspaceFix.substr(
        workspaceFix.find("bool focusUnifiedTouchPoint("),
        workspaceFix.find("void handleUnifiedTouch(") - workspaceFix.find("bool focusUnifiedTouchPoint("));
    assert(oldHardwareFocus.find("Unified::scrollWindow") == std::string::npos);
    assert(unified.find("hit >= 1700 && hit < 1700 + count ? hit - 1700 : -1") != std::string::npos);
    assert(unified.find("fb.setClipRect(x + 8, top, w - 16, visible * rowHeight)") != std::string::npos);
    // The final Gen II repaint now uses a touch-aware pixel-offset viewport but
    // still renders an eight-row clipped window over the semantic Details list.
    assert(finalFix.find("const int detailFirst = std::max(0, unified.detailsViewport.firstRow)") != std::string::npos);
    assert(finalFix.find("static_cast<std::size_t>(detailFirst)") != std::string::npos);
    assert(finalFix.find("unified.detailsTouchScroll.offset()") != std::string::npos);
    assert(finalFix.find("viewportY = y + 216") != std::string::npos);
    assert(finalFix.find("fb.clearClip()") != std::string::npos);
    assert(unified.find("\"OT Name\", p.originalTrainer") != std::string::npos);
    assert(unified.find("Met Level for selected Crystal encounter") != std::string::npos);
    assert(unified.find("Encounter::forGameSpecies(screen.sourceGameId, p.species)") != std::string::npos);

    assert(unified.find("Choose a legitimate Crystal encounter first") != std::string::npos);
    assert(unified.find("focus.row == 10 && !caught.present") != std::string::npos);
    // OT gender / Met Time cycle inline over the shared selector's choices (Round 3 NEW-07);
    // the pending/recorded split and the encounter's allowed-time mask are unchanged.
    assert(unified.find("Selector::nextOtGender(state.crystalOtGender, caught.present)") != std::string::npos);
    assert(foundation.find("uint8_t crystalMetTime = 0") != std::string::npos);
    assert(unified.find("Pending Met Time is") != std::string::npos);
    assert(unified.find("crystalAllowedTimeMask") != std::string::npos);
    // Met Time cycles inline (round 2 phase 11) over the same pending/recorded + allowed-time rules.
    assert(unified.find("Selector::nextMetTime(caught.timeOfDay, true, crystalAllowedTimeMask(screen, p, caught))") != std::string::npos);
    assert(picker.find("resolvedMetTime = state.crystalMetTime") != std::string::npos);
    assert(unified.find("passiveViewMoveColumn") != std::string::npos);
    assert(parity.find("passiveViewMoveColumn") != std::string::npos);
    assert(unified.find("Intentionally do not touch p.caughtData") != std::string::npos);
    assert(unified.find("crystalOtGenderChoiceText(displayedOtGender)") != std::string::npos);
    assert(finalFix.find("crystalOtGenderChoiceText(displayedOtGender)") != std::string::npos);
    assert(finalFix.find("std::max(0, w - 36)") != std::string::npos);
    assert(unified.find("Current level cannot be below this Crystal met level") != std::string::npos);
    assert(nativePresentation.find("Current level below met level") != std::string::npos);
    assert(picker.find("Gen2Picker::locationRowText(") != std::string::npos);
    assert(readFile("include/UI/Gen2PokemonPickerModel.h").find("encounterTimeLabel(encounter.timeMask)") !=
           std::string::npos);
    assert(sharedShell.find("drawVerticalScrollIndicator") != std::string::npos);
    assert(finalFix.find("drawVerticalScrollIndicator") != std::string::npos);
    // Move rows: one row-wide focus, drawn by the shared Moves panel in every layer.
    assert(finalFix.find("drawGen2SharedMoveRows(") != std::string::npos);
    assert(unified.find("Unified::moveRowColumn") != std::string::npos);
    const auto movePanel = unified.substr(unified.find("void drawGen2SharedMoveRows("),
        unified.find("void drawProgressionEditor(") - unified.find("void drawGen2SharedMoveRows("));
    assert(movePanel.find("visibleMoveFocus = Unified::normalizeMoveRowFocus") != std::string::npos);
    assert(movePanel.find("UI::Gen2MovesPanel::draw(") != std::string::npos);
    assert(movePanel.find("unified.focus.column == 1") == std::string::npos);
    assert(movePanel.find("unified.focus.column == 2") == std::string::npos);
    assert(unified.find("A/Y/X never mutate a View session") != std::string::npos);
    assert(finalFix.find("{\"D-pad\", \"Navigate\"}, {\"L/R\", \"Panel\"}") != std::string::npos);

    const auto overlay = readFile("src/UI/TrainerViewScreenGSCOverlay.inc");
    assert(overlay.find("publishVerifiedStagedEditorExport(editor, request)") != std::string::npos);
    assert(overlay.find("Verified staged export: ") != std::string::npos);
    assert(overlay.find("source remains unchanged") != std::string::npos);
    assert(overlay.find("std::fopen") == std::string::npos);
    assert(overlay.find("std::fwrite") == std::string::npos);
    assert(overlay.find("writeBytes(") == std::string::npos);
    assert(overlay.find("writeText(") == std::string::npos);

    // Hardware-retest corrections remain narrow wrappers around the accepted shared editor.
    assert(composite.find("#define handlePickerInput handlePickerInputBase") != std::string::npos);
    assert(composite.find("#define drawPickerOverlay drawPickerOverlayBase") != std::string::npos);
    assert(composite.find("#define handleUnifiedGen2SurfaceInput handleUnifiedGen2SurfaceInputBase") != std::string::npos);
    assert(composite.find("#define drawUnifiedGen2Surface drawUnifiedGen2SurfaceBase") != std::string::npos);
    assert(composite.find("#define drawFinalGen2Surface drawFinalGen2SurfaceBase") != std::string::npos);

    // Crystal Met Location is species/exact-game aware; accepting it writes the native
    // location and encounter-minimum caught level rather than an arbitrary landmark.
    assert(pickerModel.find("Encounter::forGameSpecies(sourceGameId, species)") != std::string::npos);
    // The Crystal location chooser is a proportional "Met Location" window (phase 11). Preserve
    // actual species-scoped encounters and native minimum-level presentation.
    assert(picker.find("\"Met Location\"") != std::string::npos);
    assert(picker.find("model.locationCount()") != std::string::npos);
    assert(picker.find("model.encounterChoices[") != std::string::npos);
    assert(picker.find("encounter->minLevel") != std::string::npos);
    assert(picker.find("Encounter::timeAllowed(*encounter, resolvedMetTime)") != std::string::npos);
    assert(picker.find("drawPokerusPicker") == std::string::npos);
    assert(picker.find("Kind::Pokerus") == std::string::npos);
    assert(picker.find("Pokerus strain") == std::string::npos);
    assert(picker.find("Pokerus days remaining") == std::string::npos);
    assert(picker.find("Stored byte preview") == std::string::npos);
    assert(unified.find("if (focus.row == 8)") != std::string::npos);
    // Owner campaign directive: Pokerus uses the shared selector interaction (same three native
    // states None/Infected/Cured, still no strain/days editor) instead of cycling in place.
    assert(unified.find("SharedEditorPresentation::nextPokerusByte(") != std::string::npos);
    assert(unified.find("p.pokerus = Gen2Native::cyclePokerusState(p.pokerus)") == std::string::npos);
    assert(picker.find("state.working.caughtData = 0") != std::string::npos);

    // Move slots must enter the accepted Gen I-style Move N contextual editor first.
    const auto contextualMoveEditor = unified.substr(unified.find("bool handleUnifiedMoveEditor("),
        unified.find("uint8_t crystalAllowedTimeMask(") - unified.find("bool handleUnifiedMoveEditor("));
    assert(contextualMoveEditor.find("normalizeMoveRowFocus") == std::string::npos);
    assert(contextualMoveEditor.find("unified.moveEditorRow == 0") != std::string::npos);
    assert(contextualMoveEditor.find("unified.moveEditorRow == 1") != std::string::npos);
    assert(contextualMoveEditor.find("unified.moveEditorRow == 2") != std::string::npos);
    assert(contextualMoveEditor.find("state.setPP(") != std::string::npos);
    assert(contextualMoveEditor.find("state.setPPUps(") != std::string::npos);
    assert(unified.find("beginUnifiedMoveEditor") != std::string::npos);
    assert(unified.find("drawUnifiedMoveEditor") != std::string::npos);
    assert(unified.find("Contextual editor — B always cancels this dialog") != std::string::npos);
    assert(unified.find("Keep these move details") != std::string::npos);
    assert(unified.find("Discard these move details") != std::string::npos);
    assert(unified.find("openPickerForFocusedField") != std::string::npos);
    assert(unified.find("beginUnifiedMoveEditor(screen, state, static_cast<int>(focus.row))") != std::string::npos);
    assert(unified.find("constexpr int w = 650, h = 390, navH = 48") != std::string::npos);
    // Nested move picker input must not fall through to the underlying Box A action.
    assert(parity.find("if (pickerStateFor(screen).model.active()) return false;") != std::string::npos);

    // Normal Gen II Move picker: Empty + exact-game-compatible choices only.
    assert(pickerFix.find("hardwareMoveAllowed") != std::string::npos);
    assert(pickerFix.find("MoveCompatibility::canLearnMove") != std::string::npos);
    assert(pickerFix.find("choices.push_back(0)") != std::string::npos);
    assert(pickerFix.find("kHardwareMovePickerLayout =") != std::string::npos);
    assert(pickerFix.find("MovePickerPresentation::compactPickerLayout()") != std::string::npos);
    assert(pickerFix.find("panelW = kHardwareMovePickerLayout.width") != std::string::npos);
    assert(pickerFix.find("panelH = kHardwareMovePickerLayout.height") != std::string::npos);
    assert(pickerFix.find("visible = kHardwareMovePickerLayout.visibleRows") != std::string::npos);
    assert(pickerFix.find("kHardwareMovePickerLayout.rowStep") != std::string::npos);
    assert(pickerFix.find("const int first = overlay.moveViewport.firstRow;") != std::string::npos);
    assert(pickerFix.find("const int liveOffset = overlay.moveScroll.offset();") != std::string::npos);
    // The shared Gen I-V Move chooser rows (UI/SharedMovesPanel.h) own the columns: No. / Move /
    // Type / Pwr / Acc / PP from the exact Gen II facts.
    assert(pickerFix.find("UI::SharedMovesPanel::drawChooserCaptions(fb, columns, y + 58, Enums::GameVersion::GSC)") != std::string::npos);
    assert(pickerFix.find("UI::SharedMovesPanel::drawChooserRow(fb, columns, yy + 6, move, Enums::GameVersion::GSC, selected)") != std::string::npos);
    assert(pickerFix.find("Compatible moves only") != std::string::npos);
    assert(pickerFix.find("MovePickerRowUI") == std::string::npos);
    assert(pickerFix.find("Needs correction") == std::string::npos);
    assert(shared.find("!editor->pendingChanges().empty()") != std::string::npos);
    assert(shared.find("drawNarrowPokemonActions(fb, screen.touchButtons, model") != std::string::npos);
    // Both Gen II rendering routes use the SAME narrow, native-indexed popup.
    assert(parity.find("drawNarrowPokemonActions(fb, screen.touchButtons, model") != std::string::npos);

    // Empty move rows cannot focus PP/Ups and their meaningless numeric cells are hidden.
    assert(workspaceFix.find("normalizeHardwareMoveRowFocus") != std::string::npos);
    assert(workspaceFix.find("Unified::normalizeMoveRowFocus") != std::string::npos);
    assert(workspaceFix.find("state.working.moves[row] == 0") == std::string::npos);
    assert(workspaceFix.find("state.working.moves[static_cast<std::size_t>(slot)] != 0") != std::string::npos);
    assert(workspaceFix.find("rightX + 184") != std::string::npos);

    // Panel titles never carry focus; only the actual field/cell does. Round 7 P2 (owner Oct 26):
    // they are the one shared Gen IV / V heading style. The stray scroll glyph is erased.
    assert(workspaceFix.find("clearHardwarePanelHeading") != std::string::npos);
    assert(workspaceFix.find("PokeBank::UIModel::PanelHeading::draw(fb, x, y, text)") != std::string::npos);
    assert(workspaceFix.find("clearHardwarePanelHeading(fb, leftX, contentY, 120, \"DETAILS\")") != std::string::npos);
    assert(workspaceFix.find("clearHardwarePanelHeading(fb, midX, contentY, 120, \"STATS\")") != std::string::npos);
    assert(workspaceFix.find("clearHardwarePanelHeading(fb, rightX, contentY, 120, \"MOVES\")") != std::string::npos);
    assert(workspaceFix.find("stray bottom glyph") != std::string::npos);
    assert(workspaceFix.find("contentY + contentH - 34") != std::string::npos);

    // Round 6 P2 (owner Oct 25, R2-TRAINER / NEW-18): Gold / Silver store no trainer gender, and the
    // separate Gold / Silver Gender card painted over the Trainer rows ("Male" under the new row).
    // The owner's rule is "no invented Gen I or Gold / Silver gender": the second renderer is gone
    // and the one Trainer model gives Gold / Silver no Gender row; Crystal keeps its stored gender.
    // The established Name/Money two-row cursor contract above is unchanged.
    assert(finalFix.find("drawGoldSilverTrainerGender") == std::string::npos);
    assert(finalFix.find("trainerGender == 0 ? \"Male\"") == std::string::npos);
    // The row model itself is exercised for Gold / Silver / Crystal under every writer set by
    // tests/test_classic_trainer_page.cpp. This contract is built without -Iinclude
    // (touch-input-gating.yml), so it pins the model's native rule as source.
    const auto page = readFile("include/UI/ClassicTrainerPage.h");
    assert(page.find("const bool genderStored = in.generation >= 3 || (in.generation == 2 && in.crystal);") !=
           std::string::npos);
    assert(page.find("if (genderStored && !genderEditable) out.push_back({Field::Gender, false, -1});") !=
           std::string::npos);

    std::cout << "GSC shared editor Trainer/field-layout/session/export + hardware regression contract: PASS\n";
    return 0;
}
