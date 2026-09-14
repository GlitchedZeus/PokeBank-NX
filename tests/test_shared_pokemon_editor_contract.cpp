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
    static_assert(createBrowsingMutatesStagedSave() == false);
    static_assert(editBrowsingMutatesSource() == false);
    static_assert(stageAddRequiresExplicitAction());
    static_assert(gen2ExternalPassiveViewUsesSharedSurface());
    static_assert(passiveViewSurfaceFor(PassiveViewOrigin::Box) == Surface::View);
    static_assert(passiveViewSurfaceFor(PassiveViewOrigin::Party) == Surface::View);
    static_assert(passiveViewSurfaceFor(PassiveViewOrigin::Storage) == Surface::View);
    static_assert(passiveViewSurfaceFor(PassiveViewOrigin::ActionSheet) == Surface::View);
    static_assert(!gen2LegacyPathProductionReachable(Gen2LegacyPath::StageAddPromptWizard));
    static_assert(!gen2LegacyPathProductionReachable(Gen2LegacyPath::LegacyTwentyEightRowEditor));
    static_assert(!gen2LegacyPathProductionReachable(Gen2LegacyPath::LegacyPokemonActionsModal));
    static_assert(!gen2LegacyPathProductionReachable(Gen2LegacyPath::RawSpeciesIdCreatePrompt));
    static_assert(!gen2LegacyPathProductionReachable(Gen2LegacyPath::RawMoveIdCreatePrompt));
    static_assert(!gen2LegacyPathProductionReachable(Gen2LegacyPath::RawHeldItemIdCreatePrompt));
    static_assert(!gen2LegacyPathProductionReachable(Gen2LegacyPath::LegacyHandleInput));

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

    // Gen I and Gen II share the same DETAILS and MOVES shell. Gen II extends VALUES
    // with capability rows for shiny/gender/held item/friendship/Pokerus.
    constexpr auto gen1Layout = layoutFor(Generation::Gen1);
    constexpr auto gen2Layout = layoutFor(Generation::Gen2);
    static_assert(gen1Layout.detailsRows == gen2Layout.detailsRows);
    static_assert(gen1Layout.movesRows == gen2Layout.movesRows);
    static_assert(gen1Layout.valuesRows == 7);
    static_assert(gen2Layout.valuesRows == 10);

    Focus focus{};
    assert((focus == Focus{Panel::Details, 0, 0}));
    focus = moveVertical(Generation::Gen2, focus, -1);
    assert((focus == Focus{Panel::Details, 4, 0}));
    focus = switchPanel(Generation::Gen2, focus, 1);
    assert(focus.panel == Panel::Values);
    focus = Focus{Panel::Values, 4, 0};
    focus = moveColumn(Generation::Gen2, focus, 1);
    assert(focus.column == 1);
    focus = moveColumn(Generation::Gen2, focus, 1);
    assert(focus.column == 2);
    focus = moveVertical(Generation::Gen2, Focus{Panel::Values, 9, 2}, 0);
    assert((focus == Focus{Panel::Values, 9, 0})); // capability rows have one focus target.
    focus = switchPanel(Generation::Gen2, Focus{Panel::Moves, 3, 2}, 1);
    assert(focus.panel == Panel::Details);
    assert(focus.row == 3);
    assert(focus.column == 0);

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
