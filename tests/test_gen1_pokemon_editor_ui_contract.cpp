#include "UI/Gen1PokemonEditorUIContract.h"
#include "UI/PokemonEditorExitGuard.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

using namespace PokeBank::UIModel::Gen1Editor;

static bool contains(const FooterSet& set, FooterAction action) {
    for (std::size_t i = 0; i < set.count; ++i)
        if (set[i] == action) return true;
    return false;
}

static std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

static std::size_t countOccurrences(const std::string& text, const std::string& needle) {
    std::size_t count = 0;
    for (std::size_t pos = 0; (pos = text.find(needle, pos)) != std::string::npos; pos += needle.size())
        ++count;
    return count;
}

int main() {
    namespace ExitGuard = PokeBank::UIModel::PokemonEditorExitGuard;
    static_assert(ExitGuard::requiresConfirmation(ExitGuard::SessionKind::Create));
    static_assert(ExitGuard::requiresConfirmation(ExitGuard::SessionKind::Edit, false));

    const auto occupied = actionsForSlot(true);
    assert(occupied.count == 7);
    assert(occupied[0] == Action::View);
    assert(occupied[1] == Action::Edit);
    assert(occupied[2] == Action::Clone);
    assert(occupied[3] == Action::Remove);
    assert(occupied[4] == Action::LegalityProvenance);
    assert(occupied[5] == Action::ReviewPendingChanges);
    assert(occupied[6] == Action::Cancel);

    const auto empty = actionsForSlot(false);
    assert(empty.count == 4);
    assert(empty[0] == Action::AddPokemon);
    assert(empty[1] == Action::ReviewPendingChanges);
    assert(empty[2] == Action::LegalityProvenance);
    assert(empty[3] == Action::Cancel);
    static_assert(emptySlotUsesCompactDialog());
    static_assert(smallConfirmationUsesCompactDialog());

    for (const auto event : {AddDraftEvent::Navigate, AddDraftEvent::EditField,
                             AddDraftEvent::PreviewSpecies, AddDraftEvent::JumpSection,
                             AddDraftEvent::OpenLogicalSubEditor, AddDraftEvent::CancelSubEditor}) {
        const auto decision = addDraftDecision(event);
        assert(!decision.mutateStagedSave && !decision.leaveDraft);
    }
    const auto cancel = addDraftDecision(AddDraftEvent::Cancel);
    assert(!cancel.mutateStagedSave && cancel.leaveDraft);
    const auto stage = addDraftDecision(AddDraftEvent::StageAdd);
    assert(stage.mutateStagedSave && stage.leaveDraft);

    static_assert(addUsesSingleScrollableWorkspace());
    static_assert(!addRequiresWizardPageNavigation());
    static_assert(legacyRawFieldCount() == 30);
    static_assert(logicalWorkspaceRowCount() == 13);
    static_assert(logicalMoveRowCount() == 4);
    static_assert(!ppIsTopLevelEditorRow());
    static_assert(!ppUpsIsTopLevelEditorRow());
    static_assert(dvsAreGrouped());
    static_assert(statExperienceIsGrouped());
    constexpr auto starts = logicalSectionStarts();
    static_assert(starts[0] == 0 && starts[1] == 3 && starts[2] == 7);
    static_assert(starts[3] == 9 && starts[4] == 11);

    assert(speciesPickerRow(1, "Bulbasaur") == "001 - Bulbasaur");
    assert(speciesPickerRow(4, "Charmander") == "004 - Charmander");
    assert(speciesPickerRow(151, "Mew") == "151 - Mew");

    static_assert(speciesPreviewOnHighlight(1, 6) == 6);
    static_assert(speciesAfterPickerClose(1, 6, false) == 1);
    static_assert(speciesAfterPickerClose(1, 6, true) == 6);
    static_assert(!speciesHoverMutatesDraft());
    static_assert(!speciesHoverMutatesStagedSave());

    constexpr auto sections = editorSections();
    static_assert(sections.size() == 5);
    static_assert(sections[0] == EditorSection::Summary);
    static_assert(sections[1] == EditorSection::Moves);
    static_assert(sections[2] == EditorSection::Stats);
    static_assert(sections[3] == EditorSection::Trainer);
    static_assert(sections[4] == EditorSection::Actions);

    static_assert(gen1RadarAxisCount() == 5);
    static_assert(!gen1HasSplitSpecial());
    constexpr auto labels = gen1StatLabels();
    static_assert(labels[0][0] == 'H' && labels[1][0] == 'A');
    static_assert(labels[4][0] == 'S');

    static_assert(editorGeometryHasNoKnownCollision());
    constexpr auto geometry = editorGeometry720p();
    static_assert(geometry.scrollBottom < geometry.statusTop);
    static_assert(geometry.panelBottom < geometry.footerTop);

    static_assert(cloneUsesVisualDestinationGrid());
    static_assert(!cloneBrowseMutatesStagedSave());
    static_assert(cloneRequiresExplicitConfirm());
    static_assert(!cloneSourceMutatedByClone());

    const auto editableBoxes = footerForSurface(FooterSurface::Boxes, true);
    assert(contains(editableBoxes, FooterAction::Add));
    assert(contains(editableBoxes, FooterAction::Remove));
    const auto readOnlyBoxes = footerForSurface(FooterSurface::Boxes, false);
    assert(!contains(readOnlyBoxes, FooterAction::Add));
    assert(!contains(readOnlyBoxes, FooterAction::Remove));

    const auto draftFooter = footerForSurface(FooterSurface::AddDraft);
    assert(contains(draftFooter, FooterAction::StageAdd));
    assert(contains(draftFooter, FooterAction::CancelDraft));
    const auto pickerFooter = footerForSurface(FooterSurface::SpeciesPicker);
    assert(!contains(pickerFooter, FooterAction::StageAdd));
    assert(contains(pickerFooter, FooterAction::Select));
    assert(contains(pickerFooter, FooterAction::Page));
    const auto moveEditorFooter = footerForSurface(FooterSurface::MoveEditor);
    assert(contains(moveEditorFooter, FooterAction::Apply));
    const auto dvEditorFooter = footerForSurface(FooterSurface::DVEditor);
    assert(contains(dvEditorFooter, FooterAction::Apply));
    const auto statExpFooter = footerForSurface(FooterSurface::StatExpEditor);
    assert(contains(statExpFooter, FooterAction::Apply));
    const auto cloneFooter = footerForSurface(FooterSurface::CloneDestination);
    assert(contains(cloneFooter, FooterAction::ConfirmClone));
    assert(contains(cloneFooter, FooterAction::PreviousNextBox));

    static_assert(footerTopmostSurfaceOverridesParent());
    static_assert(footerRestoresParentOnClose());
    static_assert(!footerMayAdvertiseUnavailableAction());

    static_assert(!hpDVIsStoredEditableField());
    static_assert(!hasSIDField());
    static_assert(!hasNatureField());
    static_assert(!hasAbilityField());
    static_assert(!hasHeldItemField());
    static_assert(!partyEditingEnabled());
    static_assert(!liveRetroArchWritingEnabled());
    static_assert(!liveInstalledGameWritingEnabled());
    static_assert(!fullEncounterLegalityEngineEnabled());

    const auto composite = readFile("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    assert(composite.find("ux2StageAddWithClassicSelection") != std::string::npos);
    assert(composite.find("const int destinationBox = state.box") != std::string::npos);
    assert(composite.find("const int destinationSlot = state.slot") != std::string::npos);
    assert(composite.find("screen.selectedBoxIndex = destinationBox") != std::string::npos);
    assert(composite.find("screen.selectedItemIndex = destinationSlot") != std::string::npos);
    assert(composite.find("drawFooterWithClassicAddLabel") != std::string::npos);
    assert(composite.find("text.replace(pos, std::char_traits<char>::length(oldLabel), \"Add\")") != std::string::npos);
    assert(composite.find("added to Box") != std::string::npos);
    assert(composite.find("Gen1MoveStatusParity") == std::string::npos);

    const auto workspace = readFile("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    assert(workspace.find("ExitGuard::requiresConfirmation") != std::string::npos);
    assert(workspace.find("Dialogs::drawDialogFrame") != std::string::npos);
    assert(workspace.find("\"New Pokémon\"") != std::string::npos);
    assert(workspace.find("\"Unsaved changes\"") != std::string::npos);
    assert(workspace.find("\"B\", \"Back\"") != std::string::npos);
    assert(workspace.find("\"Y\", \"Discard\"") != std::string::npos);
    assert(workspace.find("create ? \"Keep\" : \"Save\"") != std::string::npos);
    assert(workspace.find("A Add Staged     X Discard Draft     B Continue Editing") == std::string::npos);
    const auto exitInputBegin = workspace.find("if (foundation.editExitConfirm)");
    const auto exitInputEnd = workspace.find("if (down & HidNpadButton_Up)", exitInputBegin);
    assert(exitInputBegin != std::string::npos && exitInputEnd != std::string::npos && exitInputEnd > exitInputBegin);
    const auto exitInput = workspace.substr(exitInputBegin, exitInputEnd - exitInputBegin);
    assert(exitInput.find("HidNpadButton_Y") != std::string::npos);
    assert(exitInput.find("HidNpadButton_X") == std::string::npos);
    assert(workspace.find("MoveCompatibility::canLearnMove") != std::string::npos);
    assert(workspace.find("fb.drawText(statusX, rowY + 9, status") != std::string::npos);
    assert(workspace.find("centerX + 142") == std::string::npos);

    // DETAILS owns progression. Level and EXP each use one direct Nintendo numeric prompt;
    // the legacy combined Level/Experience contextual editor is not reachable here.
    const auto identityEditBegin = workspace.find("void foundationEditIdentity");
    const auto identityEditEnd = workspace.find("void foundationEditValue", identityEditBegin);
    assert(identityEditBegin != std::string::npos && identityEditEnd > identityEditBegin);
    const auto identityEdit = workspace.substr(identityEditBegin, identityEditEnd - identityEditBegin);
    assert(identityEdit.find("ux2OpenLevelExpEditor(") == std::string::npos);
    assert(countOccurrences(identityEdit, "Utils::promptNumber(") == 3); // Level, EXP, Trainer ID
    assert(identityEdit.find("\"Generation I Level\", current, 1, 100") != std::string::npos);
    assert(identityEdit.find("\"Generation I Experience\"") != std::string::npos);
    assert(identityEdit.find("Pokemon::getLevelFromExp") != std::string::npos);

    // The active three-panel Values surface edits one exact stat cell per A press.
    const auto valueEditBegin = workspace.find("void foundationEditValue");
    const auto valueEditEnd = workspace.find("void foundationHandleMainInput", valueEditBegin);
    assert(valueEditBegin != std::string::npos && valueEditEnd != std::string::npos &&
           valueEditEnd > valueEditBegin);
    const auto valueEdit = workspace.substr(valueEditBegin, valueEditEnd - valueEditBegin);
    assert(valueEdit.find("ux2OpenDVEditor(") == std::string::npos);
    assert(valueEdit.find("ux2OpenStatExpEditor(") == std::string::npos);
    assert(valueEdit.find("ux2OpenLevelExpEditor(") == std::string::npos);
    assert(countOccurrences(valueEdit, "Utils::promptNumber(") == 2);
    assert(valueEdit.find(" DV (0-15)") != std::string::npos);
    assert(valueEdit.find("dvs[index], 0, 15") != std::string::npos);
    assert(valueEdit.find(" Stat Exp (0-65535)") != std::string::npos);
    assert(valueEdit.find("statExperience[index], 0, 65535") != std::string::npos);
    assert(valueEdit.find("Generation I Level") == std::string::npos);
    assert(valueEdit.find("state.draft.dvs = dvs") != std::string::npos);
    assert(valueEdit.find("edit.dvs = dvs") != std::string::npos);
    assert(valueEdit.find("state.draft.statExperience = statExperience") != std::string::npos);
    assert(valueEdit.find("edit.statExperience = statExperience") != std::string::npos);
    assert(valueEdit.find("preShinyDraftDVs.reset()") != std::string::npos);
    assert(valueEdit.find("preShinyEditDVs.reset()") != std::string::npos);

    // The fullscreen hardware layer now owns passive View presentation as well as
    // Edit/Create. Compatibility must therefore be proven at the renderer it
    // delegates to, rather than requiring the old passive modal object to call
    // view.setMoveCompatibility() itself.
    const auto fullscreen = readFile("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    const auto passive = readFile("src/UI/Gen1PokemonEditorPassiveView.inc");
    assert(passive.find("drawFullscreenGen1Workspace(screen, fb)") != std::string::npos);
    assert(fullscreen.find("MoveCompatibility::firstIncompatible") != std::string::npos);
    assert(fullscreen.find("MoveCompatibility::canLearnMove") != std::string::npos);
    assert(fullscreen.find("Move compatibility") != std::string::npos);
    assert(fullscreen.find("Encounter legality") != std::string::npos);

    // The independent legacy details routes still use the presentation object and
    // retain their explicit compatibility initialization.
    for (const auto* path : {"src/UI/Modals/Gen1PokemonDetailsModal.cpp",
                             "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"})
        assert(readFile(path).find("view.setMoveCompatibility(") != std::string::npos);
    const auto passivePresentation = readFile("src/UI/Gen1PokemonDetailsPresentation.cpp");
    assert(passivePresentation.find("p.moveCompatible[i] ? \"OK\" : \"Unusual\"") != std::string::npos);
    assert(passivePresentation.find("Encounter legality\", \"Not checked") != std::string::npos);

    const auto hardware = readFile("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    assert(hardware.find("Foundation::valueCellEditable(static_cast<Foundation::ValueRow>(r)") != std::string::npos);
    assert(hardware.find("(r == 0 && c == 0) ? Colors::TextDim") != std::string::npos);
    assert(hardware.find("std::to_string(Editor::derivedHPDV(dvs)) + \" *\"") != std::string::npos);
    assert(hardware.find("leftLabels{\"Species\", \"Nickname\", \"Level\", \"EXP\", \"OT\", \"Trainer ID\"}") != std::string::npos);
    assert(hardware.find("levelSelected") == std::string::npos);
    const auto specialBegin = hardware.find("\"CALCULATED SPECIAL STATS\"");
    const auto specialEnd = hardware.find("// MOVES", specialBegin);
    assert(specialBegin != std::string::npos && specialEnd != std::string::npos && specialEnd > specialBegin);
    const auto specialBlock = hardware.substr(specialBegin, specialEnd - specialBegin);
    assert(specialBlock.find("\"SpA\"") != std::string::npos);
    assert(specialBlock.find("\"SpD\"") != std::string::npos);
    assert(specialBlock.find("one stored Gen I Special stat; split display only") != std::string::npos);
    assert(specialBlock.find("drawClassicFocus") == std::string::npos);
    assert(specialBlock.find("drawSelectionHighlight") == std::string::npos);
    assert(composite.find("Gen1PokemonEditor::drawFoundationPicker(*this, fb)") != std::string::npos);
    assert(hardware.find("state.mode != UX2Mode::MoveEditor") != std::string::npos);
    assert(hardware.find("state.subForAdd ? UX2Mode::AddDraft : UX2Mode::Edit") != std::string::npos);
    assert(hardware.find("const bool add = workspaceMode == UX2Mode::AddDraft") != std::string::npos);
    assert(composite.find("Gen1PokemonEditor::foundationMoveEditorActive(*this)") != std::string::npos);
    const auto speciesBackdrop = hardware.substr(hardware.find("void drawFoundationPicker"));
    const auto moveDialog = speciesBackdrop.substr(speciesBackdrop.find("if (!foundationPickerActive(screen))"));
    assert(moveDialog.find("drawOverlayUXCleanup3(screen, fb)") < moveDialog.find("ux3DrawPicker"));
    assert(speciesBackdrop.find("drawFullscreenGen1Workspace(screen, fb)") < speciesBackdrop.find("ux3DrawPicker"));
    const auto pickerSource = readFile("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc");
    assert(pickerSource.find("down & (HidNpadButton_L | HidNpadButton_Left)") != std::string::npos);
    assert(pickerSource.find("down & (HidNpadButton_R | HidNpadButton_Right)") != std::string::npos);
    const auto pickerDraw = pickerSource.substr(pickerSource.find("void ux3DrawPicker"));
    assert(pickerDraw.find("fb.drawFilledRect") < pickerDraw.find("ux2DrawPanel"));
    assert(speciesBackdrop.substr(0, speciesBackdrop.find("void drawFoundationBottomSplit")).find("fb.drawFilledRect") == std::string::npos);
    std::cout << "Gen I cleanup2 logical-editor + packed-Add + move-status parity contract: PASS\n";
    return 0;
}
