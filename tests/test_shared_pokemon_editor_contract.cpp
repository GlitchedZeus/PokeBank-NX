#include "UI/SharedPokemonEditorContract.h"
#include "UI/PokemonEditorExitGuard.h"
#include "UI/SharedHeldItemPicker.h"

#include <cassert>
#include <iostream>

using namespace PokeBank::UIModel::SharedPokemonEditor;

int main() {
    namespace ExitGuard = PokeBank::UIModel::PokemonEditorExitGuard;
    static_assert(!ExitGuard::requiresConfirmation(ExitGuard::SessionKind::View));
    static_assert(ExitGuard::requiresConfirmation(ExitGuard::SessionKind::Create));
    static_assert(ExitGuard::requiresConfirmation(ExitGuard::SessionKind::Edit, false));
    static_assert(ExitGuard::requiresConfirmation(ExitGuard::SessionKind::Edit, true));
    static_assert(ExitGuard::futureGenerationsUseSharedExitGuard());
    namespace HeldItems = PokeBank::UIModel::SharedHeldItemPicker;
    static_assert(HeldItems::columns == 4);
    static_assert(HeldItems::rows == 10);
    static_assert(HeldItems::pageSize == 40);
    static_assert(HeldItems::futureGenerationsUseSharedGrid());
    static_assert(HeldItems::move(0, 240, 1, 0) == 1);
    static_assert(HeldItems::move(0, 240, 0, 1) == 4);
    static_assert(HeldItems::move(39, 240, 0, 0, 1) == 79);
    static_assert(HeldItems::move(200, 240, 0, 0, -1) == 160);

    static_assert(!generationOwnsSeparateEditorUI());
    static_assert(futureGenerationsExtendSameFoundation());
    static_assert(oneTopLevelPokemonSurfaceOwnsFrame());
    static_assert(passiveViewHasFieldCursor());
    static_assert(!passiveViewAllowsEditing());
    static_assert(passiveViewAllowsPanelSwitching());
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
    static_assert(gen1Layout.detailsRows == 6);
    static_assert(gen2Layout.detailsRows == 9);
    static_assert(gen1Layout.movesRows == gen2Layout.movesRows);
    static_assert(gen1Layout.valuesRows == 6);
    static_assert(gen2Layout.valuesRows == 7);
    static_assert(gen2Layout.valueStatRows == 5);

    Focus focus{};
    assert((focus == Focus{Panel::Details, 0, 0}));
    focus = moveVertical(Generation::Gen2, focus, -1);
    assert((focus == Focus{Panel::Details, 8, 0}));
    focus = switchPanel(Generation::Gen2, focus, 1);
    assert(focus.panel == Panel::Values);
    focus = Focus{Panel::Values, 4, 0};
    focus = moveColumn(Generation::Gen2, focus, 1);
    assert(focus.column == 1);
    focus = moveColumn(Generation::Gen2, focus, 1);
    assert(focus.panel == Panel::Moves && focus.column == 0);
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
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::MetLocation, true) == FieldAccess::Editable);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::MetLevel, true) == FieldAccess::Editable);
    assert(fieldAccessForGeneration(Generation::Gen2, FieldIdentity::OriginalTrainerGender, true) == FieldAccess::Editable);

    assert(std::string(statsHeading()) == "STATS");
    assert((moveColumn(Generation::Gen2, {Panel::Details, 4, 0}, 1) == Focus{Panel::Values, 4, 0}));
    assert((moveColumn(Generation::Gen2, {Panel::Values, 4, 1}, 1) == Focus{Panel::Moves, 0, 0}));
    assert((moveColumn(Generation::Gen2, {Panel::Moves, 3, 0}, -1) == Focus{Panel::Values, 3, 1}));
    static_assert(valueCellFocusable(Generation::Gen1, 4, 0));
    static_assert(valueCellFocusable(Generation::Gen1, 4, 1));
    static_assert(!valueCellFocusable(Generation::Gen1, 4, 2));
    static_assert(!valueCellFocusable(Generation::Gen2, 4, 2));
    static_assert(!valueCellFocusable(Generation::Gen3, 5, 2));
    assert(moveColumn(Generation::Gen2, {Panel::Values, 6, 0}, 1).panel == Panel::Moves);
    assert(moveColumn(Generation::Gen2, {Panel::Values, 6, 0}, -1).panel == Panel::Details);
    assert(moveVertical(Generation::Gen2, {}, -1, true).row == 12);
    std::size_t detailsFocus = detailsScrollFocus({Panel::Details, 8, 0}, 0);
    for (int row = 0; row < 7; ++row)
        assert(detailsScrollFocus({Panel::Values, static_cast<uint8_t>(row), 0}, detailsFocus) == 8);
    for (int row = 0; row < 4; ++row)
        assert(detailsScrollFocus({Panel::Moves, static_cast<uint8_t>(row), 0}, detailsFocus) == 8);
    for (auto panel : {Panel::Values, Panel::Moves}) {
        int end = 0;
        const uint8_t columns = panel == Panel::Values ? 2 : 3;
        for (uint8_t column = 0; column < columns; ++column) {
            const auto cell = cellFocus({panel, 1, column});
            assert(cell.x >= end && cell.width > 0 && cell.width < 200);
            end = cell.x + cell.width;
        }
    }

    auto scroll = scrollWindow(5, 5, 4);
    assert(!scroll.scrolls && scroll.first == 0 && scroll.count == 5);
    // Splitting Level/Experience makes Gold/Silver nine Details rows; the shared
    // eight-row viewport scrolls by one without changing panel geometry.
    scroll = scrollWindow(gen2Layout.detailsRows, 8, 8);
    assert(scroll.scrolls && scroll.first == 1 && scroll.count == 8);
    // Crystal has thirteen Details rows and keeps exactly eight visible at once.
    constexpr auto crystalLayout = layoutFor(Generation::Gen2, true);
    static_assert(crystalLayout.detailsRows == 13);
    scroll = scrollWindow(crystalLayout.detailsRows, 8, 0);
    assert(scroll.scrolls && scroll.first == 0 && scroll.count == 8);
    scroll = scrollWindow(crystalLayout.detailsRows, 8, 9);
    assert(scroll.scrolls && scroll.first == 2 && 9 < scroll.first + scroll.count);
    scroll = scrollWindow(crystalLayout.detailsRows, 8, 12);
    assert(scroll.scrolls && scroll.first == 5 && 12 < scroll.first + scroll.count);

    // A real scrollbar is visible only when the Details window overflows.
    auto thumb = scrollThumb(gen2Layout.detailsRows, 8, 0, 288);
    assert(thumb.visible);
    const auto crystalTopThumb = scrollThumb(crystalLayout.detailsRows, 8, 0, 288);
    const auto crystalMidThumb = scrollThumb(crystalLayout.detailsRows, 8, 1, 288);
    const auto crystalBottomThumb = scrollThumb(crystalLayout.detailsRows, 8, 5, 288);
    assert(crystalTopThumb.visible && crystalTopThumb.offset == 0);
    assert(crystalTopThumb.length > 0 && crystalTopThumb.length < 288);
    assert(crystalMidThumb.offset > crystalTopThumb.offset);
    assert(crystalBottomThumb.offset == 288 - crystalBottomThumb.length);
    assert(crystalBottomThumb.offset >= 0 &&
           crystalBottomThumb.offset + crystalBottomThumb.length <= 288);

    // Derived HP DV is visible but skipped: LEFT from HP Stat Exp stays in STATS.
    static_assert(moveRowFocus(506).x == 8 && moveRowFocus(506).width == 490);
    assert((moveColumn(Generation::Gen1, {Panel::Values, 0, 1}, -1) == Focus{Panel::Values, 1, 0}));
    assert((moveColumn(Generation::Gen2, {Panel::Values, 0, 1}, -1) == Focus{Panel::Values, 1, 0}));
    assert((moveColumn(Generation::Gen3, {Panel::Values, 0, 1}, -1) == Focus{Panel::Values, 0, 0}));
    assert((moveColumn(Generation::Gen3, {Panel::Details, 0, 0}, 1) == Focus{Panel::Values, 0, 0}));
    assert((moveColumn(Generation::Gen3, {Panel::Values, 0, 1}, 1) == Focus{Panel::Moves, 0, 0}));
    assert((moveColumn(Generation::Gen3, {Panel::Moves, 0, 0}, -1) == Focus{Panel::Values, 0, 1}));
    assert((moveColumn(Generation::Gen3, {Panel::Values, 0, 0}, -1) == Focus{Panel::Details, 0, 0}));

    // All main workspace modes share row-only moves, without changing the lower-level
    // cell model used by accepted Gen I and independent contextual editors.
    for (const auto generation : {Generation::Gen2, Generation::Gen3}) {
        for (uint8_t column = 0; column < 3; ++column) {
            const Focus stale{Panel::Moves, 2, column};
            assert((normalizeMoveRowFocus(generation, stale) == Focus{Panel::Moves, 2, 0}));
            assert((moveRowColumn(generation, stale, 1) == Focus{Panel::Moves, 2, 0}));
            assert((moveRowColumn(generation, stale, -1) == Focus{Panel::Values, 2, 1}));
        }
    }
    assert((moveColumn(Generation::Gen1, {Panel::Moves, 2, 0}, 1) == Focus{Panel::Moves, 2, 1}));

    // View selects the move row itself only. PP / PP Ups remain display metadata.
    assert((normalizePassiveViewFocus(Generation::Gen2, {Panel::Moves, 2, 2}, true) ==
            Focus{Panel::Moves, 2, 0}));
    assert((passiveViewMoveColumn(Generation::Gen2, {Panel::Moves, 2, 0}, 1, true) ==
            Focus{Panel::Moves, 2, 0}));
    assert((passiveViewMoveColumn(Generation::Gen2, {Panel::Moves, 2, 0}, -1, true) ==
            Focus{Panel::Values, 2, 1}));
    assert((passiveViewMoveColumn(Generation::Gen2, {Panel::Moves, 2, 2}, -1, true) ==
            Focus{Panel::Values, 2, 1}));
    assert((passiveViewMoveColumn(Generation::Gen3, {Panel::Moves, 1, 2}, -1) ==
            Focus{Panel::Values, 1, 1}));
    assert((moveColumn(Generation::Gen2, {Panel::Moves, 2, 0}, 1, true) ==
            Focus{Panel::Moves, 2, 1}));
    assert((passiveViewMoveColumn(Generation::Gen3, {Panel::Values, 1, 1}, 1) ==
            Focus{Panel::Moves, 1, 0}));
    assert((passiveViewMoveColumn(Generation::Gen3, {Panel::Moves, 1, 0}, 1) ==
            Focus{Panel::Moves, 1, 0}));

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