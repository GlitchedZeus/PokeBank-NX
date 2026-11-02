#ifndef POKEBANK_UI_INVENTORY_UI_CONTRACT_H
#define POKEBANK_UI_INVENTORY_UI_CONTRACT_H

#include <cstdint>
#include <string>
#include <string_view>
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

// ---- The ONE Bag footer model (Gen I-V) ------------------------------------------------------
// Round 14 R08 (owner, verbatim: "gen 4 footer says edit quanitty not edit amout or what gen 1-3 have for
// the A button, gen 5 has almost none of the proper footers and im guessing controls"): every
// generation's Bag list draws THIS list, in the Gen I-III wording and order the owner calls correct:
//   D-pad/Stick Navigate, A Edit Amount, X Add Item, Y Remove Item, L/R Category, ZR Top Up, + <plus>,
//   - Help, B Back.
// Each entry appears only when its handler acts (inventoryActionAvailability): A only for a
// quantity-editable selected item, Y only with a selection, X / ZR only in an editable category.
// `plus` names what + does: "Options" (installed titles), "Review" (Gen I-V staged Bags,
// ReviewEntry.h). `topUp` adds ZR Top Up (owner Oct 28 Feature B) for a Bag with a staged editor.
struct InventoryFooterHint {
    const char* glyph;
    const char* label;
};

[[nodiscard]] inline std::vector<InventoryFooterHint> inventoryFooterHints(
    bool editable, bool categorySupported, bool hasSelection, bool quantityEditable,
    const char* plusLabel = "Options", bool topUp = false) {
    const auto available = inventoryActionAvailability(
        editable, categorySupported, hasSelection, quantityEditable);
    std::vector<InventoryFooterHint> out{{"D-pad/Stick", "Navigate"}};
    if (available.editAmount) out.push_back({"A", "Edit Amount"});
    if (available.addItem) out.push_back({"X", "Add Item"});
    if (available.removeItem) out.push_back({"Y", "Remove Item"});
    if (categorySupported) out.push_back({"L/R", "Category"});
    if (topUp && editable && categorySupported) out.push_back({"ZR", "Top Up"});
    if (plusLabel && *plusLabel) out.push_back({"+", plusLabel});
    out.push_back({"-", "Help"});
    out.push_back({"B", "Back"});
    return out;
}

// The "K: label  |  K: label" form ScreenChrome::drawNavBar(const std::string&) draws as badges.
[[nodiscard]] inline std::string serializeInventoryFooter(const std::vector<InventoryFooterHint>& hints) {
    std::string out;
    for (const auto& hint : hints) {
        if (!out.empty()) out += "  |  ";
        out += hint.glyph;
        out += ": ";
        out += hint.label;
    }
    return out;
}

// `plusHint` is the old "+: Label" segment (ReviewEntry::footerText); only its label is used, the
// + glyph is fixed by the model.
[[nodiscard]] inline std::string inventoryFooter(bool editable, bool categorySupported,
                                                 bool hasSelection, bool quantityEditable,
                                                 const char* plusHint = "+: Options", bool topUp = false) {
    std::string_view plus = plusHint ? plusHint : "";
    if (plus.size() > 3 && plus.substr(0, 3) == "+: ") plus.remove_prefix(3);
    const std::string label(plus);
    return serializeInventoryFooter(inventoryFooterHints(
        editable, categorySupported, hasSelection, quantityEditable, label.c_str(), topUp));
}

// The ONE Add Item picker footer (Gen I-V; the picker itself is UI/SharedBagAddPicker.h /
// SharedItemGridPicker). The Gen I-III wording; the picker is a grid, so navigation is the whole
// D-pad / Stick (never "Up/Down"). X Sort is shown only where the picker's X handler sorts.
[[nodiscard]] inline std::vector<InventoryFooterHint> addItemPickerFooterHints(bool sort = true) {
    std::vector<InventoryFooterHint> out{
        {"D-pad/Stick", "Navigate"}, {"A", "Add / Select"}, {"B", "Cancel"}, {"L/R", "Page"}};
    if (sort) out.push_back({"X", "Sort"});
    return out;
}

[[nodiscard]] constexpr bool useReadOnlyInventoryFooterOverride(
    bool sourceReadOnly, bool detailsActive, bool settingsMode,
    bool stagedInventoryActionsAvailable) noexcept {
    return sourceReadOnly && !detailsActive && !settingsMode &&
           !stagedInventoryActionsAvailable;
}

// Non-item hit targets for the bag header. Negative IDs cannot collide
// with item rows (0..N-1); -1 is reserved for no touch target.
struct InventoryCategoryTouch {
    static constexpr int PreviousId = -20;
    static constexpr int NextId = -21;
    static constexpr int Width = 56;
    static constexpr int Height = 40;
    // Leave a full finger-width buffer between touch targets: adjacent
    // category arrows must not feel like one small two-state toggle.
    static constexpr int Gap = 40;
    static constexpr int RightInset = 20;

    [[nodiscard]] static constexpr int previousX(int panelX, int panelWidth) noexcept {
        return panelX + panelWidth - RightInset - 2 * Width - Gap;
    }
    [[nodiscard]] static constexpr int nextX(int panelX, int panelWidth) noexcept {
        return panelX + panelWidth - RightInset - Width;
    }
    [[nodiscard]] static constexpr int topY(int panelY, int headerHeight) noexcept {
        return panelY + (headerHeight - Height) / 2;
    }
};

struct InventoryPickerViewport {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int visibleRows = 0;

    [[nodiscard]] constexpr bool contains(int px, int py) const noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

struct InventoryPickerLayout {
    // Generic item picker geometry (independent of classic Add Item).
    static constexpr int Width = 560;
    static constexpr int VerticalMargin = 60;
    static constexpr int RowHeight = 56;
    static constexpr int FooterHeight = 64;
    static constexpr int HorizontalPadding = 20;
    static constexpr int TitleOffsetY = 16;
    static constexpr int DividerOffsetY = 52;
    static constexpr int ListTopOffset = 64;
    static constexpr int ClassicListTopOffset = 90;
    static constexpr int ClassicRowsPerPage = 11;
    static constexpr int TouchInsetX = 12;
    static constexpr int BottomInset = 18;

    [[nodiscard]] static constexpr InventoryPickerViewport viewport(
        int screenW, int screenH) noexcept {
        const int panelY = VerticalMargin;
        const int panelH = screenH - 2 * VerticalMargin;
        const int panelX = (screenW - Width) / 2;
        const int listY = panelY + ListTopOffset;
        const int listBottom = panelY + panelH - BottomInset;
        const int rows = (listBottom - listY) / RowHeight;
        const int visible = rows > 0 ? rows : 1;
        return {
            panelX + TouchInsetX,
            listY,
            Width - 2 * TouchInsetX,
            visible * RowHeight,
            visible,
        };
    }
};

// The Gen I-V Add Item picker has ONE geometry: SharedMenuGeometry MenuType::ItemPicker, drawn
// and hit-tested through UI/SharedBagAddPicker.h by the classic overlay and the Gen IV/V surfaces.

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
    const auto* source = findInventoryBaselineItem(baseline, category, itemId);
    // The raw truth value is never a dirty signal: source isNew=true + current isNew=true is clean.
    // But if PokeBank changes that game-owned metadata bit, the staged save does differ semantically.
    const bool sourceIsNew = source ? source->sourceIsNew : false;
    const bool metadataChanged = source && currentIsNew != sourceIsNew;
    return {
        sourceIsNew,
        source ? (source->quantity != currentQuantity || metadataChanged) : currentQuantity != 0,
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
