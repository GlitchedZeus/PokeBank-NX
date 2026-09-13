#include "UI/ClassicInventoryUIModel.h"

#include <array>

namespace PokeBank::UIModel {
namespace {

constexpr std::array<ClassicPocket, 6> kGen1Categories{{
    ClassicPocket::Items, ClassicPocket::Medicines, ClassicPocket::Balls,
    ClassicPocket::KeyItems, ClassicPocket::TMHM, ClassicPocket::PCItems,
}};
constexpr std::array<ClassicPocket, 5> kGen2Categories{{
    ClassicPocket::TMHM, ClassicPocket::Items, ClassicPocket::KeyItems,
    ClassicPocket::Balls, ClassicPocket::PCItems,
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

bool classicInventoryNeedsStoryWarning(ClassicGame game, ClassicPocket pocket,
                                       uint16_t itemId) noexcept {
    return PokeVault::Inventory::isStorySensitiveKeyItem(game, pocket, itemId);
}

std::vector<uint16_t> classicInventoryAddableItems(ClassicGame game, ClassicPocket pocket) {
    const auto values = PokeVault::Inventory::addableItems(game, pocket);
    return {values.begin(), values.end()};
}

bool classicInventoryEmptyState(bool supported, std::size_t visibleItems) noexcept {
    return supported && visibleItems == 0;
}

} // namespace PokeBank::UIModel
