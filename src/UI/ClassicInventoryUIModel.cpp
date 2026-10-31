#include "UI/ClassicInventoryUIModel.h"
#include "UI/SharedBagAddFlow.h"

#include <array>
#include <algorithm>

namespace PokeBank::UIModel {
namespace {


constexpr std::array<ClassicPocket, 6> kGen1Categories{{
    ClassicPocket::Items, ClassicPocket::Medicines, ClassicPocket::Balls,
    ClassicPocket::KeyItems, ClassicPocket::TMHM, ClassicPocket::PCItems,
}};
constexpr std::array<ClassicPocket, 5> kGen2Categories{{
    ClassicPocket::Items, ClassicPocket::Balls, ClassicPocket::KeyItems,
    ClassicPocket::TMHM, ClassicPocket::PCItems,
}};
constexpr std::array<ClassicPocket, 6> kGen3Categories{{
    ClassicPocket::Items, ClassicPocket::KeyItems, ClassicPocket::Balls,
    ClassicPocket::TMHM, ClassicPocket::Berries, ClassicPocket::PCItems,
}};

bool isGen1(ClassicGame game) noexcept {
    return game == ClassicGame::Red || game == ClassicGame::Blue || game == ClassicGame::Yellow;
}
bool isGen2(ClassicGame game) noexcept {
    return game == ClassicGame::Gold || game == ClassicGame::Silver || game == ClassicGame::Crystal;
}

} // namespace

std::optional<ClassicGame> classicInventoryGame(std::string_view sourceGameId) noexcept {
    return PokeVault::Inventory::classicGameFromId(sourceGameId);
}

std::span<const ClassicPocket> classicInventoryCategories(ClassicGame game) noexcept {
    if (isGen1(game)) return kGen1Categories;
    if (isGen2(game)) return kGen2Categories;
    return kGen3Categories;
}

int classicInventoryShiftCategory(ClassicGame game, int current, int delta) noexcept {
    const int count = static_cast<int>(classicInventoryCategories(game).size());
    if (count <= 0) return 0;
    const int start = std::clamp(current, 0, count - 1);
    const int step = ((delta % count) + count) % count;
    return (start + step) % count;
}

std::optional<ClassicPocket> classicInventoryPocketAt(ClassicGame game, int category) noexcept {
    const auto categories = classicInventoryCategories(game);
    if (category < 0 || category >= static_cast<int>(categories.size())) return std::nullopt;
    const auto pocket = categories[static_cast<std::size_t>(category)];
    if (!PokeVault::Inventory::supportsPocket(game, pocket)) return std::nullopt;
    return pocket;
}

std::string_view classicInventoryCategoryName(ClassicGame game, int category) noexcept {
    const auto pocket = classicInventoryPocketAt(game, category);
    return pocket ? PokeVault::Inventory::pocketName(*pocket) : std::string_view{};
}

bool classicInventoryQuantityEditable(ClassicGame game, ClassicPocket pocket,
                                      uint16_t itemId) noexcept {
    return PokeVault::Inventory::quantityRule(game, pocket, itemId).editable;
}

uint16_t classicInventoryMaximumQuantity(ClassicGame game, ClassicPocket pocket,
                                         uint16_t itemId) noexcept {
    return PokeVault::Inventory::quantityRule(game, pocket, itemId).maximum;
}

// The shared Gen I-V Key Item warning rule (UI/SharedBagAddFlow.h): by the policy's item kind, so
// a Key Item kept in the PC warns too.
bool classicInventoryNeedsStoryWarning(ClassicGame game, ClassicPocket pocket,
                                       uint16_t itemId) noexcept {
    return BagAdd::needsKeyItemWarning(classicItemQuantityRule(game, pocket, itemId));
}

std::vector<uint16_t> classicInventoryAddableItems(ClassicGame game, ClassicPocket pocket) {
    const auto values = PokeVault::Inventory::addableItems(game, pocket);
    return {values.begin(), values.end()};
}

ClassicInventoryAction classicInventoryAction(ClassicInventoryInput input) noexcept {
    return inventoryAction(input);
}

ClassicInventoryActionAvailability classicInventoryActionAvailability(
    bool categorySupported, bool hasSelection, bool quantityEditable) noexcept {
    return inventoryActionAvailability(true, categorySupported, hasSelection, quantityEditable);
}

// The one Gen I-III per-item quantity query: the classic catalog's addable gate, then the shared
// QuantityPolicy rule (native pocket caps; key items and HMs unique at 1).
ClassicItemQuantity classicItemQuantity(ClassicGame game, ClassicPocket pocket, uint16_t itemId) noexcept {
    namespace Inv = PokeVault::Inventory;
    if (!Inv::isAddableItem(game, pocket, itemId)) return {false, 0, 0};
    const auto rule = Inv::QuantityPolicy::quantityRule(Inv::quantityPolicyGame(game),
                                                        Inv::quantityPolicyPocket(pocket), itemId);
    if (!rule.valid) return {false, 0, 0};
    return {rule.editable, rule.minimum, rule.maximum};
}

PokeVault::Inventory::QuantityPolicy::ItemQuantityRule classicItemQuantityRule(
    ClassicGame game, ClassicPocket pocket, uint16_t itemId) noexcept {
    namespace Inv = PokeVault::Inventory;
    if (!Inv::isAddableItem(game, pocket, itemId)) return {};
    return Inv::QuantityPolicy::quantityRule(Inv::quantityPolicyGame(game),
                                             Inv::quantityPolicyPocket(pocket), itemId);
}

bool classicInventoryEmptyState(bool supported, std::size_t visibleItems) noexcept {
    return supported && visibleItems == 0;
}

} // namespace PokeBank::UIModel
