#include "UI/ClassicInventoryUIModel.h"
#include "UI/InventoryUIContract.h"
#include "Inventory/ClassicInventoryCatalog.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using PokeVault::Inventory::ClassicPocket;
using namespace PokeBank::UIModel;

int main() {
    const auto yellow = classicInventoryGame("yellow_gb");
    const auto gold = classicInventoryGame("gold_gbc");
    const auto emerald = classicInventoryGame("emerald_gba");
    const auto firered = classicInventoryGame("firered_gba");
    assert(yellow && gold && emerald && firered);

    assert(classicInventoryCategories(*yellow).size() == 6);
    assert(classicInventoryPocketAt(*yellow, 0) == ClassicPocket::Items);
    assert(classicInventoryPocketAt(*yellow, 1) == ClassicPocket::Medicines);
    assert(classicInventoryPocketAt(*yellow, 2) == ClassicPocket::Balls);
    assert(classicInventoryPocketAt(*yellow, 5) == ClassicPocket::PCItems);
    assert(!classicInventoryPocketAt(*yellow, 6));

    assert(classicInventoryCategories(*gold).size() == 5);
    assert(classicInventoryPocketAt(*gold, 0) == ClassicPocket::Items);
    assert(classicInventoryPocketAt(*gold, 1) == ClassicPocket::Balls);
    assert(classicInventoryPocketAt(*gold, 2) == ClassicPocket::KeyItems);
    assert(classicInventoryPocketAt(*gold, 3) == ClassicPocket::TMHM);
    assert(classicInventoryPocketAt(*gold, 4) == ClassicPocket::PCItems);

    assert(classicInventoryCategories(*emerald).size() == 6);
    assert(classicInventoryPocketAt(*emerald, 4) == ClassicPocket::Berries);
    assert(classicInventoryCategories(*firered).size() == 6);

    const auto yellowBalls = classicInventoryAddableItems(*yellow, ClassicPocket::Balls);
    assert(std::find(yellowBalls.begin(), yellowBalls.end(), 4) != yellowBalls.end());
    assert(std::find(yellowBalls.begin(), yellowBalls.end(), 851) == yellowBalls.end());
    const auto goldBalls = classicInventoryAddableItems(*gold, ClassicPocket::Balls);
    assert(std::find(goldBalls.begin(), goldBalls.end(), 851) == goldBalls.end());
    assert(!PokeVault::Inventory::isAddableItem(*emerald, ClassicPocket::Items, 500));
    assert(!PokeVault::Inventory::isAddableItem(*firered, ClassicPocket::TMHM, 500));

    assert(classicInventoryEmptyState(true, 0));
    assert(!classicInventoryEmptyState(true, 1));
    assert(!classicInventoryEmptyState(false, 0));
    assert(!classicInventoryQuantityEditable(*gold, ClassicPocket::KeyItems, 54));
    assert(!classicInventoryQuantityEditable(*gold, ClassicPocket::TMHM, 247));
    assert(classicInventoryQuantityEditable(*gold, ClassicPocket::Items, 18));
    assert(classicInventoryMaximumQuantity(*gold, ClassicPocket::Items, 18) == 99);
    assert(classicInventoryNeedsStoryWarning(*gold, ClassicPocket::KeyItems, 54));
    assert(!classicInventoryNeedsStoryWarning(*gold, ClassicPocket::Balls, 5));

    assert(classicInventoryAction(ClassicInventoryInput::A) == ClassicInventoryAction::EditAmount);
    assert(classicInventoryAction(ClassicInventoryInput::X) == ClassicInventoryAction::AddItem);
    assert(classicInventoryAction(ClassicInventoryInput::Y) == ClassicInventoryAction::RemoveItem);
    assert(classicInventoryAction(ClassicInventoryInput::L) == ClassicInventoryAction::PreviousCategory);
    assert(classicInventoryAction(ClassicInventoryInput::R) == ClassicInventoryAction::NextCategory);
    assert(classicInventoryAction(ClassicInventoryInput::Plus) == ClassicInventoryAction::Options);
    assert(classicInventoryAction(ClassicInventoryInput::Minus) == ClassicInventoryAction::Help);
    assert(classicInventoryAction(ClassicInventoryInput::B) == ClassicInventoryAction::Back);
    assert(classicInventoryAction(ClassicInventoryInput::X) == inventoryAction(InventoryInput::X));
    assert(classicInventoryAction(ClassicInventoryInput::Y) == inventoryAction(InventoryInput::Y));

    const auto emptyActions = classicInventoryActionAvailability(true, false, false);
    assert(!emptyActions.editAmount);
    assert(emptyActions.addItem);
    assert(!emptyActions.removeItem);
    const auto quantityActions = classicInventoryActionAvailability(true, true, true);
    assert(quantityActions.editAmount);
    assert(quantityActions.addItem);
    assert(quantityActions.removeItem);
    const auto fixedActions = classicInventoryActionAvailability(true, true, false);
    assert(!fixedActions.editAmount);
    assert(fixedActions.addItem);
    assert(fixedActions.removeItem);
    const auto unsupportedActions = classicInventoryActionAvailability(false, false, false);
    assert(!unsupportedActions.editAmount && !unsupportedActions.addItem && !unsupportedActions.removeItem);

    std::cout << "classic inventory UI model: PASS\n";
}
