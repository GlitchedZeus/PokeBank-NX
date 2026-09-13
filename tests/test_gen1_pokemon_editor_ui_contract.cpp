#include "UI/Gen1PokemonEditorUIContract.h"

#include <array>
#include <cassert>
#include <iostream>

using namespace PokeBank::UIModel::Gen1Editor;

static bool contains(const FooterSet& set, FooterAction action) {
    for (std::size_t i = 0; i < set.count; ++i)
        if (set[i] == action) return true;
    return false;
}

int main() {
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

    std::cout << "Gen I cleanup2 logical-editor + hover + footer + layout contract: PASS\n";
    return 0;
}
