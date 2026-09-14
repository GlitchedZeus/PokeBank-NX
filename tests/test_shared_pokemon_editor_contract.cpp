#include "UI/SharedPokemonEditorContract.h"

#include <cassert>
#include <iostream>

using namespace PokeBank::UIModel::SharedPokemonEditor;

int main() {
    static_assert(!generationOwnsSeparateEditorUI());
    static_assert(futureGenerationsExtendSameFoundation());
    static_assert(oneTopLevelPokemonSurfaceOwnsFrame());
    static_assert(!passiveViewHasFieldCursor());
    static_assert(!passiveViewAllowsEditing());
    static_assert(!passiveViewAllowsPanelSwitching());
    static_assert(!createBrowsingMutatesStagedSave());
    static_assert(!editBrowsingMutatesSource());
    static_assert(stageAddRequiresExplicitAction());

    const auto empty = actionsForSlot(false);
    assert(empty.count == 3);
    assert(empty[0] == Action::Add);
    assert(empty[1] == Action::Review);
    assert(empty[2] == Action::Close);
    assert(surfaceForAction(empty[0]) == Surface::CreateDraft);

    ActionCapabilities gen1;
    gen1.canClone = true;
    gen1.canRemove = true;
    gen1.hasLegalityProvenance = true;
    const auto gen1Occupied = actionsForSlot(true, gen1);
    assert(gen1Occupied.count == 7);
    assert(gen1Occupied[0] == Action::View);
    assert(gen1Occupied[1] == Action::Edit);
    assert(gen1Occupied[2] == Action::Clone);
    assert(gen1Occupied[3] == Action::Remove);
    assert(gen1Occupied[4] == Action::LegalityProvenance);
    assert(gen1Occupied[5] == Action::Review);
    assert(gen1Occupied[6] == Action::Close);

    ActionCapabilities gen2;
    gen2.canClone = true;
    gen2.canRemove = false; // backend capability is not implemented yet; shell remains shared.
    gen2.hasLegalityProvenance = false;
    const auto gen2Occupied = actionsForSlot(true, gen2);
    assert(gen2Occupied.count == 5);
    assert(gen2Occupied[0] == Action::View);
    assert(gen2Occupied[1] == Action::Edit);
    assert(gen2Occupied[2] == Action::Clone);
    assert(gen2Occupied[3] == Action::Review);
    assert(gen2Occupied[4] == Action::Close);

    assert(draftDecision(DraftEvent::Navigate).mutateStagedSave == false);
    assert(draftDecision(DraftEvent::BrowsePicker).mutateStagedSave == false);
    assert(draftDecision(DraftEvent::AcceptPicker).mutateStagedSave == false);
    assert(draftDecision(DraftEvent::Cancel).leaveDraft == true);
    assert(draftDecision(DraftEvent::Cancel).mutateStagedSave == false);
    assert(draftDecision(DraftEvent::StageAdd).mutateStagedSave == true);
    assert(draftDecision(DraftEvent::StageAdd).leaveDraft == true);

    constexpr auto g = geometry720p();
    static_assert(g.x == 40 && g.y == 31 && g.width == 1200 && g.height == 610);
    static_assert(g.detailsX < g.valuesX && g.valuesX < g.movesX);
    static_assert(g.footerTop > g.y + g.height);

    std::cout << "Shared Pokemon editor architecture contract: PASS\n";
    return 0;
}
