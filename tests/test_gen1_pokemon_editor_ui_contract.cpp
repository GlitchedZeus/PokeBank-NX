#include "UI/Gen1PokemonEditorUIContract.h"

#include <array>
#include <cassert>
#include <iostream>

using namespace PokeBank::UIModel::Gen1Editor;

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

    const auto navigate = addDraftDecision(AddDraftEvent::Navigate);
    assert(!navigate.mutateStagedSave && !navigate.leaveDraft);
    const auto edit = addDraftDecision(AddDraftEvent::EditField);
    assert(!edit.mutateStagedSave && !edit.leaveDraft);
    const auto cancel = addDraftDecision(AddDraftEvent::Cancel);
    assert(!cancel.mutateStagedSave && cancel.leaveDraft);
    const auto stage = addDraftDecision(AddDraftEvent::StageAdd);
    assert(stage.mutateStagedSave && stage.leaveDraft);

    constexpr auto sections = editorSections();
    static_assert(sections.size() == 5);
    static_assert(sections[0] == EditorSection::Summary);
    static_assert(sections[1] == EditorSection::Moves);
    static_assert(sections[2] == EditorSection::DVs);
    static_assert(sections[3] == EditorSection::StatExp);
    static_assert(sections[4] == EditorSection::Trainer);

    static_assert(!hpDVIsStoredEditableField());
    static_assert(!hasSIDField());
    static_assert(!hasNatureField());
    static_assert(!hasAbilityField());
    static_assert(!hasHeldItemField());
    static_assert(!partyEditingEnabled());
    static_assert(!liveRetroArchWritingEnabled());
    static_assert(!liveInstalledGameWritingEnabled());

    std::cout << "Gen I Pokemon editor UI contract: PASS\n";
    return 0;
}
