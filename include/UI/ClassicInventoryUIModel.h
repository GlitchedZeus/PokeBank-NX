#ifndef POKEBANK_UI_CLASSIC_INVENTORY_UI_MODEL_H
#define POKEBANK_UI_CLASSIC_INVENTORY_UI_MODEL_H

#include "Inventory/ClassicInventoryCatalog.h"
#include "UI/InventoryUIContract.h"

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
// All category arrows and physical L/R share this exact-game wrap rule.
[[nodiscard]] int classicInventoryShiftCategory(ClassicGame game, int current,
                                                int delta) noexcept;
[[nodiscard]] std::string_view classicInventoryCategoryName(ClassicGame game, int category) noexcept;
[[nodiscard]] bool classicInventoryQuantityEditable(ClassicGame game, ClassicPocket pocket,
                                                    uint16_t itemId) noexcept;
[[nodiscard]] uint16_t classicInventoryMaximumQuantity(ClassicGame game, ClassicPocket pocket,
                                                       uint16_t itemId) noexcept;
[[nodiscard]] bool classicInventoryNeedsStoryWarning(ClassicGame game, ClassicPocket pocket,
                                                     uint16_t itemId) noexcept;
[[nodiscard]] std::vector<uint16_t> classicInventoryAddableItems(ClassicGame game,
                                                                 ClassicPocket pocket);
using ClassicInventoryInput = InventoryInput;
using ClassicInventoryAction = InventoryAction;
using ClassicInventoryActionAvailability = InventoryActionAvailability;

[[nodiscard]] ClassicInventoryAction classicInventoryAction(ClassicInventoryInput input) noexcept;
[[nodiscard]] ClassicInventoryActionAvailability classicInventoryActionAvailability(
    bool categorySupported, bool hasSelection, bool quantityEditable) noexcept;
[[nodiscard]] bool classicInventoryEmptyState(bool supported, std::size_t visibleItems) noexcept;

// Y on an item asks first ("Remove <item>?"); the owner's Round 4 policy uses A INSIDE
// that explicit Bag Remove question. B wins over A. Pokemon Release and Backup Delete stay Y.
enum class ClassicRemoveConfirm : uint8_t { None, Remove, Cancel };
[[nodiscard]] constexpr ClassicRemoveConfirm classicRemoveConfirmStep(bool a, bool b) noexcept {
    if (b) return ClassicRemoveConfirm::Cancel;
    if (a) return ClassicRemoveConfirm::Remove;
    return ClassicRemoveConfirm::None;
}

// The one per-item quantity question every Gen I/II bag action asks: the maximum the item may
// hold, and whether it has a quantity editor at all (key items, bikes, rods, HMs: fixed at 1, no
// editor). Thread I's ItemQuantityPolicy can replace the body without touching callers.
struct ClassicItemQuantity {
    bool editable = false;
    uint16_t minimum = 0;
    uint16_t maximum = 0;
};
[[nodiscard]] ClassicItemQuantity classicItemQuantity(ClassicGame game, ClassicPocket pocket,
                                                      uint16_t itemId) noexcept;
// The same gate as classicItemQuantity, as the full QuantityPolicy rule the shared Gen I-V Bag Add
// flow (UI/SharedBagAddFlow.h) consumes; invalid (valid == false) outside the classic catalog.
[[nodiscard]] PokeVault::Inventory::QuantityPolicy::ItemQuantityRule classicItemQuantityRule(
    ClassicGame game, ClassicPocket pocket, uint16_t itemId) noexcept;


} // namespace PokeBank::UIModel

#endif
