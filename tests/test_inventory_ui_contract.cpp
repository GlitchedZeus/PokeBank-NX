#include "UI/InventoryUIContract.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace PokeBank::UIModel;

int main() {
    static_assert(inventoryAction(InventoryInput::A) == InventoryAction::EditAmount);
    static_assert(inventoryAction(InventoryInput::X) == InventoryAction::AddItem);
    static_assert(inventoryAction(InventoryInput::Y) == InventoryAction::RemoveItem);
    static_assert(inventoryAction(InventoryInput::L) == InventoryAction::PreviousCategory);
    static_assert(inventoryAction(InventoryInput::R) == InventoryAction::NextCategory);
    static_assert(inventoryAction(InventoryInput::Plus) == InventoryAction::Options);
    static_assert(inventoryAction(InventoryInput::Minus) == InventoryAction::Help);
    static_assert(inventoryAction(InventoryInput::B) == InventoryAction::Back);

    constexpr auto occupied = inventoryActionAvailability(true, true, true, true);
    static_assert(occupied.editAmount && occupied.addItem && occupied.removeItem);
    constexpr auto empty = inventoryActionAvailability(true, true, false, false);
    static_assert(!empty.editAmount && empty.addItem && !empty.removeItem);
    constexpr auto readOnly = inventoryActionAvailability(false, true, true, true);
    static_assert(!readOnly.editAmount && !readOnly.addItem && !readOnly.removeItem);
    constexpr auto fixedQuantity = inventoryActionAvailability(true, true, true, false);
    static_assert(!fixedQuantity.editAmount && fixedQuantity.addItem && fixedQuantity.removeItem);

    // A read-only source with a real PokeBank staged editor must bypass generic read-only footer.
    static_assert(useReadOnlyInventoryFooterOverride(true, false, false, false));
    static_assert(!useReadOnlyInventoryFooterOverride(true, false, false, true));
    static_assert(!useReadOnlyInventoryFooterOverride(false, false, false, false));
    static_assert(!useReadOnlyInventoryFooterOverride(true, true, false, false));
    static_assert(!useReadOnlyInventoryFooterOverride(true, false, true, false));

    const std::string fullFooter = inventoryFooter(true, true, true, true);
    assert(fullFooter.find("A: Edit Amount") != std::string::npos);
    assert(fullFooter.find("X: Add Item") != std::string::npos);
    assert(fullFooter.find("Y: Remove Item") != std::string::npos);
    assert(fullFooter.find("L/R: Category") != std::string::npos);
    assert(fullFooter.find("+: Options") != std::string::npos);
    assert(fullFooter.find("-: Help") != std::string::npos);
    assert(fullFooter.find("B: Back") != std::string::npos);

    const std::string emptyFooter = inventoryFooter(true, true, false, false);
    assert(emptyFooter.find("X: Add Item") != std::string::npos);
    assert(emptyFooter.find("A: Edit Amount") == std::string::npos);
    assert(emptyFooter.find("Y: Remove Item") == std::string::npos);
    assert(emptyFooter.find("L/R: Category") != std::string::npos);
    assert(emptyFooter.find("+: Options") != std::string::npos);
    assert(emptyFooter.find("-: Help") != std::string::npos);
    assert(emptyFooter.find("B: Back") != std::string::npos);

    const std::string fixedFooter = inventoryFooter(true, true, true, false);
    assert(fixedFooter.find("A: Edit Amount") == std::string::npos);
    assert(fixedFooter.find("X: Add Item") != std::string::npos);
    assert(fixedFooter.find("Y: Remove Item") != std::string::npos);

    const std::string readOnlyFooter = inventoryFooter(false, true, true, true);
    assert(readOnlyFooter.find("A: Edit Amount") == std::string::npos);
    assert(readOnlyFooter.find("X: Add Item") == std::string::npos);
    assert(readOnlyFooter.find("Y: Remove Item") == std::string::npos);
    assert(readOnlyFooter.find("B: Back") != std::string::npos);

    static_assert(InventoryPickerLayout::Width == 560);
    static_assert(InventoryPickerLayout::VerticalMargin == 60);
    static_assert(InventoryPickerLayout::RowHeight == 40);
    static_assert(InventoryPickerLayout::FooterHeight == 64);
    static_assert(InventoryPickerLayout::Width < 900);
    static_assert(InventoryPickerLayout::ClassicRowsPerPage == 11);

    InventoryBaseline baseline{
        {0, 17, 53, true},   // source-owned isNew item: Potion-like representative
        {0, 18, 5, false},
        {1, 17, 8, false},   // same item id in a different category is a distinct semantic identity
    };

    const auto untouchedNew = inventoryItemPresentationState(baseline, 0, 17, 53, true, false);
    assert(untouchedNew.sourceIsNew);
    assert(!untouchedNew.stagedModified);
    assert(itemNameColorRole(untouchedNew) == ItemNameColorRole::NormalText);

    const auto changed = inventoryItemPresentationState(baseline, 0, 17, 99, true, false);
    assert(changed.sourceIsNew);
    assert(changed.stagedModified);
    assert(itemNameColorRole(changed) == ItemNameColorRole::ModifiedText);

    const auto reverted = inventoryItemPresentationState(baseline, 0, 17, 53, true, false);
    assert(reverted.sourceIsNew);
    assert(!reverted.stagedModified);
    assert(itemNameColorRole(reverted) == ItemNameColorRole::NormalText);

    // Raw isNew=true is not dirty, but changing that source-owned metadata bit is a real semantic edit.
    const auto currentFlagCleared = inventoryItemPresentationState(baseline, 0, 17, 53, false, false);
    assert(currentFlagCleared.sourceIsNew);
    assert(currentFlagCleared.stagedModified);
    const auto sourceFalseUntouched = inventoryItemPresentationState(baseline, 0, 18, 5, false, false);
    assert(!sourceFalseUntouched.sourceIsNew);
    assert(!sourceFalseUntouched.stagedModified);
    const auto sourceFalseFlagChanged = inventoryItemPresentationState(baseline, 0, 18, 5, true, false);
    assert(!sourceFalseFlagChanged.sourceIsNew);
    assert(sourceFalseFlagChanged.stagedModified);

    const auto added = inventoryItemPresentationState(baseline, 0, 99, 10, true, false);
    assert(!added.sourceIsNew);
    assert(added.stagedModified);
    assert(itemNameColorRole(added) == ItemNameColorRole::ModifiedText);

    const auto addedThenRemoved = inventoryItemPresentationState(baseline, 0, 99, 0, true, false);
    assert(!addedThenRemoved.stagedModified);

    // A source item removed from the staged model is semantically dirty even though its row is no longer drawn.
    const auto removedSource = inventoryItemPresentationState(baseline, 0, 18, 0, false, false);
    assert(removedSource.stagedModified);

    const auto selectedDirty = inventoryItemPresentationState(baseline, 0, 17, 99, true, true);
    assert(selectedDirty.stagedModified);
    assert(itemNameColorRole(selectedDirty) == ItemNameColorRole::SelectedText);

    const auto otherCategory = inventoryItemPresentationState(baseline, 1, 17, 8, false, false);
    assert(!otherCategory.stagedModified);
    const auto categoryChanged = inventoryItemPresentationState(baseline, 1, 17, 9, false, false);
    assert(categoryChanged.stagedModified);

    std::cout << "inventory UI contract: PASS\n";
}
