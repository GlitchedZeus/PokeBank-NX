#ifndef POKEBANK_UI_CLASSIC_INVENTORY_UI_MODEL_H
#define POKEBANK_UI_CLASSIC_INVENTORY_UI_MODEL_H

#include "Inventory/ClassicInventoryCatalog.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel {

using PokeVault::Inventory::ClassicGame;
using PokeVault::Inventory::ClassicPocket;

[[nodiscard]] std::optional<ClassicGame> classicInventoryGame(std::string_view sourceGameId) noexcept;
[[nodiscard]] std::span<const ClassicPocket> classicInventoryCategories(ClassicGame game) noexcept;
[[nodiscard]] std::optional<ClassicPocket> classicInventoryPocketAt(ClassicGame game, int category) noexcept;
[[nodiscard]] std::string_view classicInventoryCategoryName(ClassicGame game, int category) noexcept;
[[nodiscard]] bool classicInventoryQuantityEditable(ClassicGame game, ClassicPocket pocket,
                                                    uint16_t itemId) noexcept;
[[nodiscard]] uint16_t classicInventoryMaximumQuantity(ClassicGame game, ClassicPocket pocket,
                                                       uint16_t itemId) noexcept;
[[nodiscard]] bool classicInventoryNeedsStoryWarning(ClassicGame game, ClassicPocket pocket,
                                                     uint16_t itemId) noexcept;
[[nodiscard]] std::vector<uint16_t> classicInventoryAddableItems(ClassicGame game,
                                                                 ClassicPocket pocket);
enum class ClassicInventoryInput : uint8_t { A, X, Y, L, R, Plus, Minus, B };
enum class ClassicInventoryAction : uint8_t {
    EditAmount, AddItem, RemoveItem, PreviousCategory, NextCategory, Options, Help, Back,
};

struct ClassicInventoryActionAvailability {
    bool editAmount = false;
    bool addItem = false;
    bool removeItem = false;
};

[[nodiscard]] ClassicInventoryAction classicInventoryAction(ClassicInventoryInput input) noexcept;
[[nodiscard]] ClassicInventoryActionAvailability classicInventoryActionAvailability(
    bool categorySupported, bool hasSelection, bool quantityEditable) noexcept;
[[nodiscard]] bool classicInventoryEmptyState(bool supported, std::size_t visibleItems) noexcept;

} // namespace PokeBank::UIModel

#endif
