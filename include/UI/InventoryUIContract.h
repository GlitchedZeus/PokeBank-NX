#ifndef POKEBANK_UI_INVENTORY_UI_CONTRACT_H
#define POKEBANK_UI_INVENTORY_UI_CONTRACT_H

#include <cstdint>
#include <string>
#include <vector>

namespace PokeBank::UIModel {

enum class InventoryInput : uint8_t { A, X, Y, L, R, Plus, Minus, B };
enum class InventoryAction : uint8_t {
    EditAmount, AddItem, RemoveItem, PreviousCategory, NextCategory, Options, Help, Back,
};

struct InventoryActionAvailability {
    bool editAmount = false;
    bool addItem = false;
    bool removeItem = false;
};

[[nodiscard]] constexpr InventoryAction inventoryAction(InventoryInput input) noexcept {
    switch (input) {
        case InventoryInput::A: return InventoryAction::EditAmount;
        case InventoryInput::X: return InventoryAction::AddItem;
        case InventoryInput::Y: return InventoryAction::RemoveItem;
        case InventoryInput::L: return InventoryAction::PreviousCategory;
        case InventoryInput::R: return InventoryAction::NextCategory;
        case InventoryInput::Plus: return InventoryAction::Options;
        case InventoryInput::Minus: return InventoryAction::Help;
        case InventoryInput::B: return InventoryAction::Back;
    }
    return InventoryAction::Back;
}

[[nodiscard]] constexpr InventoryActionAvailability inventoryActionAvailability(
    bool editable, bool categorySupported, bool hasSelection, bool quantityEditable) noexcept {
    return {
        editable && categorySupported && hasSelection && quantityEditable,
        editable && categorySupported,
        editable && categorySupported && hasSelection,
    };
}

[[nodiscard]] inline std::string inventoryFooter(bool editable, bool categorySupported,
                                                 bool hasSelection, bool quantityEditable) {
    const auto available = inventoryActionAvailability(
        editable, categorySupported, hasSelection, quantityEditable);
    std::string out = "D-pad/Stick: Navigate";
    if (available.editAmount) out += "  |  A: Edit Amount";
    if (available.addItem) out += "  |  X: Add Item";
    if (available.removeItem) out += "  |  Y: Remove Item";
    if (categorySupported) out += "  |  L/R: Category";
    out += "  |  +: Options  |  -: Help  |  B: Back";
    return out;
}

struct InventoryPickerLayout {
    static constexpr int Width = 560;
    static constexpr int VerticalMargin = 60;
    static constexpr int RowHeight = 40;
    static constexpr int FooterHeight = 48;
    static constexpr int HorizontalPadding = 20;
    static constexpr int TitleOffsetY = 16;
    static constexpr int DividerOffsetY = 52;
    static constexpr int ListTopOffset = 64;
    static constexpr int ClassicListTopOffset = 90;
    static constexpr int ClassicRowsPerPage = 11;
};

struct InventoryBaselineItem {
    int category = -1;
    uint16_t itemId = 0;
    uint16_t quantity = 0;
    bool sourceIsNew = false;
};

using InventoryBaseline = std::vector<InventoryBaselineItem>;

struct InventoryItemPresentationState {
    bool sourceIsNew = false;
    bool stagedModified = false;
    bool selected = false;
};

[[nodiscard]] inline const InventoryBaselineItem* findInventoryBaselineItem(
    const InventoryBaseline& baseline, int category, uint16_t itemId) noexcept {
    for (const auto& item : baseline)
        if (item.category == category && item.itemId == itemId) return &item;
    return nullptr;
}

[[nodiscard]] inline InventoryItemPresentationState inventoryItemPresentationState(
    const InventoryBaseline& baseline, int category, uint16_t itemId, uint16_t currentQuantity,
    bool currentIsNew, bool selected) noexcept {
    (void)currentIsNew; // game-owned marker is deliberately not a PokeBank dirty signal
    const auto* source = findInventoryBaselineItem(baseline, category, itemId);
    return {
        source ? source->sourceIsNew : false,
        source ? source->quantity != currentQuantity : currentQuantity != 0,
        selected,
    };
}

enum class ItemNameColorRole : uint8_t { NormalText, ModifiedText, SelectedText };

[[nodiscard]] constexpr ItemNameColorRole itemNameColorRole(
    const InventoryItemPresentationState& state) noexcept {
    if (state.selected) return ItemNameColorRole::SelectedText;
    return state.stagedModified ? ItemNameColorRole::ModifiedText : ItemNameColorRole::NormalText;
}

} // namespace PokeBank::UIModel

#endif
