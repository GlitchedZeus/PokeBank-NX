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

    ActionCapabilities classicMenu;
    classicMenu.canClone = true;
    classicMenu.canRemove = false;
    classicMenu.hasLegalityProvenance = true;

    const auto empty = actionsForSlot(false, classicMenu);
    assert(empty.count == 4);
    assert(empty[0] == Action::Add);
    assert(empty[1] == Action::Review);
    assert(empty[2] == Action::LegalityProvenance);
    assert(empty[3] == Action::Close);
    assert(surfaceForAction(empty[0]) == Surface::CreateDraft);
    assert(std::string(actionLabel(Action::Close)) == "Cancel");
    assert(std::string(actionLabel(Action::LegalityProvenance)) == "Legality & Provenance");
    constexpr auto menuGeometry = actionMenuGeometry();
    static_assert(menuGeometry.rowStart == 86 && menuGeometry.rowHeight == 42 && menuGeometry.rowStep == 46);

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
    gen2.canRemove = false;
    gen2.hasLegalityProvenance = true;
    const auto gen2Occupied = actionsForSlot(true, gen2);
    assert(gen2Occupied.count == 6);
    assert(gen2Occupied[0] == Action::View);
    assert(gen2Occupied[1] == Action::Edit);
    assert(gen2Occupied[2] == Action::Clone);
    assert(gen2Occupied[3] == Action::LegalityProvenance);
    assert(gen2Occupied[4] == Action::Review);
    assert(gen2Occupied[5] == Action::Close);

    assert(boxActivation(false, true, false) == BoxActivation::Actions);
    assert(boxActivation(true, true, false) == BoxActivation::Actions);
    assert(boxActivation(false, false, true) == BoxActivation::Add);
    assert(boxActivation(true, false, true) == BoxActivation::None);
    const auto afterAdd = postAddSelection(6, 3);
    assert(afterAdd.box == 6 && afterAdd.slot == 3 && !afterAdd.openActions);

    // Gen II extends the same shell: descriptive capabilities move to DETAILS while VALUES remains
    // numeric/stat material. MOVES stays identical.
    constexpr auto gen1Layout = layoutFor(Generation::Gen1);
    constexpr auto gen2Layout = layoutFor(Generation::Gen2);
    static_assert(gen1Layout.detailsRows == 5);
    static_assert(gen2Layout.detailsRows == 8);
    static_assert(gen1Layout.movesRows == gen2Layout.movesRows);
    static_assert(gen1Layout.valuesRows == 7);
    static_assert(gen2Layout.valuesRows == 7);
    static_assert(gen2Layout.valueStatRows == 5);

    Focus focus{};
    assert((focus == Focus{Panel::Details, 0, 0}));
    focus = moveVertical(Generation::Gen2, focus, -1);
    assert((focus == Focus{Panel::Details, 7, 0}));
    focus = switchPanel(Generation::Gen2, focus, 1);
    assert(focus.panel == Panel::Values);
    focus = Focus{Panel::Values, 4, 0};
    focus = moveColumn(Generation::Gen2, focus, 1);
    assert(focus.column == 1);
    focus = moveColumn(Generation::Gen2, focus, 1);
    assert(focus.column == 2);
    focus = moveVertical(Generation::Gen2, Focus{Panel::Values, 6, 2}, 0);
    assert((focus == Focus{Panel::Values, 6, 0}));
    focus = switchPanel(Generation::Gen2, Focus{Panel::Moves, 3, 2}, 1);
    assert(focus.panel == Panel::Details);
    assert(focus.row == 3);
    assert(focus.column == 0);

    assert(fieldAccessForGeneration(Generation::Gen1, FieldIdentity::DV) == FieldAccess::Editable);
    assert(fieldAccessForGeneration(Generation::Gen1, FieldIdentity::StatExperience) == FieldAccess::Editable);
    assert(fieldAccessForGeneration(Generation::Gen1, FieldIdentity::Shiny) == FieldAccess::Derived);
    for (auto field : {FieldIdentity::Ability, FieldIdentity::Friendship, FieldIdentity::Nature,
                       FieldIdentity::Ball, FieldIdentity::IV, FieldIdentity::EV,
                       FieldIdentity::MetLocation, FieldIdentity::MetDate, FieldIdentity::Egg})
        assert(fieldAccessForGeneration(Generation::Gen1, field) == FieldAccess::Hidden);

    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::HeldItem) == FieldAccess::Editable);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::Friendship) == FieldAccess::Editable);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::Pokerus) == FieldAccess::Editable);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::Gender) == FieldAccess::Derived);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::Shiny) == FieldAccess::Derived);
    for (auto field : {FieldIdentity::Ability, FieldIdentity::PersonalityId, FieldIdentity::Nature,
                       FieldIdentity::Ball, FieldIdentity::IV, FieldIdentity::EV,
                       FieldIdentity::MetDate, FieldIdentity::EggDate})
        assert(fieldAccessForGeneration(Generation::Gen2, field) == FieldAccess::Hidden);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::MetLocation, false) == FieldAccess::Hidden);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::MetLocation, true) == FieldAccess::ReadOnly);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::MetLevel, true) == FieldAccess::ReadOnly);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::OriginalTrainerGender, true) == FieldAccess::ReadOnly);

    auto scroll = scrollWindow(5, 5, 4);
    assert(!scroll.scrolls && scroll.first == 0 && scroll.count == 5);
    scroll = scrollWindow(gen2Layout.detailsRows, 5, 0);
    assert(scroll.scrolls && scroll.first == 0 && scroll.count == 5);
    scroll = scrollWindow(gen2Layout.detailsRows, 5, 5);
    assert(scroll.scrolls && scroll.first == 1 && 5 >= scroll.first && 5 < scroll.first + scroll.count);
    scroll = scrollWindow(gen2Layout.detailsRows, 5, 7);
    assert(scroll.scrolls && scroll.first == 3 && 7 < scroll.first + scroll.count);

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

    std::cout << "Shared Pokemon editor architecture + classic parity/field/scroll contracts: PASS\n";
    return 0;
}