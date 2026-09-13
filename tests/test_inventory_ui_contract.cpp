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

    const std::string readOnlyFooter = inventoryFooter(false, true, true, true);
    assert(readOnlyFooter.find("A: Edit Amount") == std::string::npos);
    assert(readOnlyFooter.find("X: Add Item") == std::string::npos);
    assert(readOnlyFooter.find("Y: Remove Item") == std::string::npos);
    assert(readOnlyFooter.find("B: Back") != std::string::npos);

    static_assert(InventoryPickerLayout::Width == 560);
    static_assert(InventoryPickerLayout::VerticalMargin == 60);
    static_assert(InventoryPickerLayout::RowHeight == 40);
    static_assert(InventoryPickerLayout::Width < 900);
    static_assert(InventoryPickerLayout::ClassicRowsPerPage == 11);

    static_assert(itemNameColorRole(false, false) == ItemNameColorRole::NormalText);
    static_assert(itemNameColorRole(false, true) == ItemNameColorRole::NormalText);
    static_assert(itemNameColorRole(true, false) == ItemNameColorRole::SelectedText);
    static_assert(itemNameColorRole(true, true) == ItemNameColorRole::SelectedText);

    std::cout << "inventory UI contract: PASS\n";
}
