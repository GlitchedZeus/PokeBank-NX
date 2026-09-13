from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one match, found {count}")
    p.write_text(text.replace(old, new, 1))


Path("include/UI/InventoryUIContract.h").write_text(r'''#ifndef POKEBANK_UI_INVENTORY_UI_CONTRACT_H
#define POKEBANK_UI_INVENTORY_UI_CONTRACT_H

#include <cstdint>
#include <string>

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

enum class ItemNameColorRole : uint8_t { NormalText, SelectedText };

[[nodiscard]] constexpr ItemNameColorRole itemNameColorRole(bool selected, bool isNew) noexcept {
    (void)isNew;
    return selected ? ItemNameColorRole::SelectedText : ItemNameColorRole::NormalText;
}

} // namespace PokeBank::UIModel

#endif
''')

Path("tests/test_inventory_ui_contract.cpp").write_text(r'''#include "UI/InventoryUIContract.h"

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
''')

replace_once(
    "include/UI/ClassicInventoryUIModel.h",
    '#include "Inventory/ClassicInventoryCatalog.h"\n',
    '#include "Inventory/ClassicInventoryCatalog.h"\n#include "UI/InventoryUIContract.h"\n',
)
replace_once(
    "include/UI/ClassicInventoryUIModel.h",
    '''enum class ClassicInventoryInput : uint8_t { A, X, Y, L, R, Plus, Minus, B };
enum class ClassicInventoryAction : uint8_t {
    EditAmount, AddItem, RemoveItem, PreviousCategory, NextCategory, Options, Help, Back,
};

struct ClassicInventoryActionAvailability {
    bool editAmount = false;
    bool addItem = false;
    bool removeItem = false;
};
''',
    '''using ClassicInventoryInput = InventoryInput;
using ClassicInventoryAction = InventoryAction;
using ClassicInventoryActionAvailability = InventoryActionAvailability;
''',
)
replace_once(
    "src/UI/ClassicInventoryUIModel.cpp",
    '''ClassicInventoryAction classicInventoryAction(ClassicInventoryInput input) noexcept {
    switch (input) {
        case ClassicInventoryInput::A: return ClassicInventoryAction::EditAmount;
        case ClassicInventoryInput::X: return ClassicInventoryAction::AddItem;
        case ClassicInventoryInput::Y: return ClassicInventoryAction::RemoveItem;
        case ClassicInventoryInput::L: return ClassicInventoryAction::PreviousCategory;
        case ClassicInventoryInput::R: return ClassicInventoryAction::NextCategory;
        case ClassicInventoryInput::Plus: return ClassicInventoryAction::Options;
        case ClassicInventoryInput::Minus: return ClassicInventoryAction::Help;
        case ClassicInventoryInput::B: return ClassicInventoryAction::Back;
    }
    return ClassicInventoryAction::Back;
}

ClassicInventoryActionAvailability classicInventoryActionAvailability(
    bool categorySupported, bool hasSelection, bool quantityEditable) noexcept {
    return {
        categorySupported && hasSelection && quantityEditable,
        categorySupported,
        categorySupported && hasSelection,
    };
}
''',
    '''ClassicInventoryAction classicInventoryAction(ClassicInventoryInput input) noexcept {
    return inventoryAction(input);
}

ClassicInventoryActionAvailability classicInventoryActionAvailability(
    bool categorySupported, bool hasSelection, bool quantityEditable) noexcept {
    return inventoryActionAvailability(true, categorySupported, hasSelection, quantityEditable);
}
''',
)

replace_once(
    "src/UI/Panels/ItemsPanel.cpp",
    '#include "UI/ClassicInventoryUIModel.h"\n',
    '#include "UI/ClassicInventoryUIModel.h"\n#include "UI/InventoryUIContract.h"\n',
)
replace_once(
    "src/UI/Panels/ItemsPanel.cpp",
    '''            const Color nameCol = selected ? Colors::PrimaryText
                                           : (item.isNew ? Colors::Accent : Colors::Text);
''',
    '''            const auto nameRole = PokeBank::UIModel::itemNameColorRole(selected, item.isNew);
            const Color nameCol = nameRole == PokeBank::UIModel::ItemNameColorRole::SelectedText
                                ? Colors::PrimaryText : Colors::Text;
''',
)

replace_once(
    "src/UI/Dialogs/PickerDialog.cpp",
    '#include "UI/ScreenChrome.h"     // drawScrollbar\n',
    '#include "UI/ScreenChrome.h"     // drawScrollbar\n#include "UI/InventoryUIContract.h"\n',
)
replace_once(
    "src/UI/Dialogs/PickerDialog.cpp",
    '''        const int pw = 560, ph = H - 120;
        const int px = (W - pw) / 2, py = 60;
''',
    '''        const int pw = PokeBank::UIModel::InventoryPickerLayout::Width;
        const int py = PokeBank::UIModel::InventoryPickerLayout::VerticalMargin;
        const int ph = H - 2 * py;
        const int px = (W - pw) / 2;
''',
)
replace_once(
    "src/UI/Dialogs/PickerDialog.cpp",
    '''        const int rowH = 40;
        const int listTop = py + 64, listBottom = py + ph - 48;
''',
    '''        const int rowH = PokeBank::UIModel::InventoryPickerLayout::RowHeight;
        const int listTop = py + PokeBank::UIModel::InventoryPickerLayout::ListTopOffset;
        const int listBottom = py + ph - PokeBank::UIModel::InventoryPickerLayout::FooterHeight;
''',
)
replace_once(
    "src/UI/Dialogs/PickerDialog.cpp",
    '        fb.drawText(px + 20, py + ph - 34, "A: Select    B: Cancel    L/R: Page", Colors::TextDim, TextStyle::Caption);\n',
    '        fb.drawText(px + 20, py + ph - 34, "D-pad/Stick Navigate   A Select   B Cancel   L/R Page", Colors::TextDim, TextStyle::Caption);\n',
)

replace_once(
    "src/UI/ClassicInventoryOverlay.cpp",
    '#include "UI/ClassicInventoryUIModel.h"\n',
    '#include "UI/ClassicInventoryUIModel.h"\n#include "UI/InventoryUIContract.h"\n',
)
replace_once(
    "src/UI/ClassicInventoryOverlay.cpp",
    '''void drawRow(PKSEFramebuffer& fb, int x, int y, int width, const std::string& text, bool selected) {
    constexpr int h = 48;
    fb.drawFilledRoundedRect(x, y, width, h, 10, selected ? Colors::AccentDim : Colors::PanelAlt);
    if (selected) fb.drawRoundedRect(x, y, width, h, 10, Colors::Accent, 2);
    fb.drawText(x + 18, y + 13, text, selected ? Colors::Text : Colors::TextDim, TextStyle::Body);
}
''',
    '''void drawRow(PKSEFramebuffer& fb, int x, int y, int width, const std::string& text,
             bool selected, int height = 48) {
    fb.drawFilledRoundedRect(x, y, width, height, 10, selected ? Colors::AccentDim : Colors::PanelAlt);
    if (selected) fb.drawRoundedRect(x, y, width, height, 10, Colors::Accent, 2);
    const int ty = y + (height - fb.lineHeight(TextStyle::Body)) / 2;
    fb.drawText(x + 18, ty, text, selected ? Colors::Text : Colors::TextDim, TextStyle::Body);
}
''',
)
replace_once(
    "src/UI/ClassicInventoryOverlay.cpp",
    '''        if (down & HidNpadButton_Up) state.pickerRow = (state.pickerRow - 1 + count) % count;
        if (down & HidNpadButton_Down) state.pickerRow = (state.pickerRow + 1) % count;
        if (down & HidNpadButton_B) { state.pickerActive = false; return true; }
''',
    '''        if (down & HidNpadButton_Up) state.pickerRow = (state.pickerRow - 1 + count) % count;
        if (down & HidNpadButton_Down) state.pickerRow = (state.pickerRow + 1) % count;
        constexpr int page = PokeBank::UIModel::InventoryPickerLayout::ClassicRowsPerPage;
        if (down & (HidNpadButton_L | HidNpadButton_Left))
            state.pickerRow = std::max(0, state.pickerRow - page);
        if (down & (HidNpadButton_R | HidNpadButton_Right))
            state.pickerRow = std::min(count - 1, state.pickerRow + page);
        if (down & HidNpadButton_B) { state.pickerActive = false; return true; }
''',
)
replace_once(
    "src/UI/ClassicInventoryOverlay.cpp",
    '''    screen.touchButtons.clear();
    constexpr int width = 900;
    constexpr int height = 610;
    const int x = (fb.getWidth() - width) / 2;
    const int y = 66;
    fb.drawSoftShadow(x, y, width, height, 18);
    fb.drawFilledRoundedRect(x, y, width, height, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, width, height, 18, Colors::Accent, 2);
''',
    '''    screen.touchButtons.clear();
    const bool compactPicker = state.pickerActive;
    const int width = compactPicker ? PokeBank::UIModel::InventoryPickerLayout::Width : 900;
    const int y = compactPicker ? PokeBank::UIModel::InventoryPickerLayout::VerticalMargin : 66;
    const int height = compactPicker ? fb.getHeight() - 2 * y : 610;
    const int x = (fb.getWidth() - width) / 2;
    const int radius = compactPicker ? 16 : 18;
    fb.drawSoftShadow(x, y, width, height, radius);
    fb.drawFilledRoundedRect(x, y, width, height, radius, Colors::Panel);
    fb.drawRoundedRect(x, y, width, height, radius, Colors::Accent, 2);
''',
)
replace_once(
    "src/UI/ClassicInventoryOverlay.cpp",
    '''    if (state.pickerActive && game && pocket) {
        fb.drawText(x + 30, y + 20, "Add Item to " + std::string(PokeVault::Inventory::pocketName(*pocket)),
                    Colors::Text, TextStyle::Heading);
        fb.drawText(x + 30, y + 56, "Exact-game catalog only — stored item IDs remain native to this game.",
                    Colors::TextDim, TextStyle::Caption);
        constexpr int visibleRows = 8;
        const int count = static_cast<int>(state.pickerItems.size());
        const int start = std::clamp(state.pickerRow - visibleRows / 2, 0, std::max(0, count - visibleRows));
        int rowY = y + 94;
        for (int i = start; i < std::min(count, start + visibleRows); ++i) {
            const uint16_t itemId = state.pickerItems[static_cast<std::size_t>(i)];
            std::string label = PokeVault::Inventory::displayItemName(*game, *pocket, itemId);
            if (stagedQuantity(screen, *pocket, itemId) != 0) label += "  (Already in pouch)";
            drawRow(fb, x + 30, rowY, width - 60, label, i == state.pickerRow);
            rowY += 57;
        }
        fb.drawText(x + 30, y + height - 42, "D-pad / Left Stick Select    A Add / Focus Existing    B Cancel",
                    Colors::TextDim, TextStyle::Caption);
        return;
    }
''',
    '''    if (state.pickerActive && game && pocket) {
        constexpr int pad = PokeBank::UIModel::InventoryPickerLayout::HorizontalPadding;
        fb.drawText(x + pad, y + 16, "Add Item to " + std::string(PokeVault::Inventory::pocketName(*pocket)),
                    Colors::Text, TextStyle::Heading);
        const int count = static_cast<int>(state.pickerItems.size());
        std::string pos = std::to_string(state.pickerRow + 1) + " / " + std::to_string(count);
        int posW = 0, posH = 0;
        fb.measureText(pos, posW, posH, TextStyle::Caption);
        fb.drawText(x + width - pad - posW, y + 22, pos, Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + pad, y + 54, "Exact game + pocket catalog", Colors::TextDim, TextStyle::Caption);
        fb.drawHDivider(x + pad, y + 76, width - pad * 2);

        const int rowH = PokeBank::UIModel::InventoryPickerLayout::RowHeight;
        const int listTop = y + PokeBank::UIModel::InventoryPickerLayout::ClassicListTopOffset;
        const int listBottom = y + height - PokeBank::UIModel::InventoryPickerLayout::FooterHeight;
        const int visibleRows = std::max(1, (listBottom - listTop) / rowH);
        const int start = std::clamp(state.pickerRow - visibleRows / 2, 0, std::max(0, count - visibleRows));
        int rowY = listTop;
        for (int i = start; i < std::min(count, start + visibleRows); ++i) {
            const uint16_t itemId = state.pickerItems[static_cast<std::size_t>(i)];
            std::string label = PokeVault::Inventory::displayItemName(*game, *pocket, itemId);
            if (stagedQuantity(screen, *pocket, itemId) != 0) label += "  (Already in pouch)";
            drawRow(fb, x + 12, rowY, width - 24, label, i == state.pickerRow, rowH - 4);
            screen.touchButtons.push_back({i, x + 12, rowY, width - 24, rowH - 4});
            rowY += rowH;
        }
        fb.drawText(x + pad, y + height - 34,
                    "D-pad/Stick Navigate   A Add/Select   B Cancel   L/R Page",
                    Colors::TextDim, TextStyle::Caption);
        return;
    }
''',
)

replace_once(
    "src/UI/TrainerViewScreenBase.inc",
    '#include "UI/ClassicInventoryUIModel.h"\n',
    '#include "UI/ClassicInventoryUIModel.h"\n#include "UI/InventoryUIContract.h"\n',
)
replace_once(
    "src/UI/TrainerViewScreenBase.inc",
    '''                if (sourceReadOnly() && (kDown & (HidNpadButton_A | HidNpadButton_X | HidNpadButton_Y))) {
                    requireMutableWorkspace();
                    kDown &= ~(HidNpadButton_A | HidNpadButton_X | HidNpadButton_Y);
                }
                // Get current pouch size for bounds checking
''',
    '''                if (sourceReadOnly() && (kDown & (HidNpadButton_A | HidNpadButton_X | HidNpadButton_Y))) {
                    requireMutableWorkspace();
                    kDown &= ~(HidNpadButton_A | HidNpadButton_X | HidNpadButton_Y);
                }
                const auto inventoryActionPressed = [&](PokeBank::UIModel::InventoryAction action) {
                    using PokeBank::UIModel::InventoryInput;
                    return ((kDown & HidNpadButton_A) && PokeBank::UIModel::inventoryAction(InventoryInput::A) == action)
                        || ((kDown & HidNpadButton_X) && PokeBank::UIModel::inventoryAction(InventoryInput::X) == action)
                        || ((kDown & HidNpadButton_Y) && PokeBank::UIModel::inventoryAction(InventoryInput::Y) == action)
                        || ((kDown & HidNpadButton_L) && PokeBank::UIModel::inventoryAction(InventoryInput::L) == action)
                        || ((kDown & HidNpadButton_R) && PokeBank::UIModel::inventoryAction(InventoryInput::R) == action);
                };
                // Get current pouch size for bounds checking
''',
)
replace_once(
    "src/UI/TrainerViewScreenBase.inc",
    '''                    // A button to edit item amount (map the visible selection back to the raw pouch slot)
                    if (kDown & HidNpadButton_A) {
''',
    '''                    // A edits amount under the shared PokeBank inventory control contract.
                    if (inventoryActionPressed(PokeBank::UIModel::InventoryAction::EditAmount)) {
''',
)
replace_once(
    "src/UI/TrainerViewScreenBase.inc",
    '''                    // Y opens the add-item picker: the ids that legally belong in THIS pouch,
                    // minus what the bag already holds. Nothing is offered for a pouch the game
                    // doesn't have, or when appending isn't supported and every legal id is present.
                    if (kDown & HidNpadButton_Y) {
''',
    '''                    // X opens the add-item picker under the shared PokeBank inventory contract.
                    // The ids still come from THIS game's pouch provider; no raw-id namespace is mixed.
                    if (inventoryActionPressed(PokeBank::UIModel::InventoryAction::AddItem)) {
''',
)
replace_once(
    "src/UI/TrainerViewScreenBase.inc",
    '''                    // X asks to remove the selected item, behind a confirm dialog. The delete
                    // runs in the itemRemoveConfirmActive handler on A; here we just open the prompt.
                    if ((kDown & HidNpadButton_X) && totalItems > 0
''',
    '''                    // Y removes the selected item under the shared PokeBank inventory contract.
                    // The delete still runs behind the existing confirmation dialog.
                    if (inventoryActionPressed(PokeBank::UIModel::InventoryAction::RemoveItem) && totalItems > 0
''',
)
replace_once(
    "src/UI/TrainerViewScreenBase.inc",
    '''                    if (kDown & HidNpadButton_L) {
                        switch(trainer.getGameGroup()) {
''',
    '''                    if (inventoryActionPressed(PokeBank::UIModel::InventoryAction::PreviousCategory)) {
                        switch(trainer.getGameGroup()) {
''',
)
replace_once(
    "src/UI/TrainerViewScreenBase.inc",
    '''                    if (kDown & HidNpadButton_R) {
                        switch(trainer.getGameGroup()) {
''',
    '''                    if (inventoryActionPressed(PokeBank::UIModel::InventoryAction::NextCategory)) {
                        switch(trainer.getGameGroup()) {
''',
)
replace_once(
    "src/UI/TrainerViewScreenBase.inc",
    '''                    const auto available = PokeBank::UIModel::classicInventoryActionAvailability(
                        pocket.has_value(), hasSelection, quantityEditable);
                    instructions = "D-pad/Stick: Navigate";
                    if (available.editAmount) instructions += "  |  A: Edit Amount";
                    if (available.addItem) instructions += "  |  X: Add Item";
                    if (available.removeItem) instructions += "  |  Y: Remove Item";
                    instructions += "  |  L/R: Category  |  +: Options  |  -: Help  |  B: Back";
                } else {
                    instructions = "Up/Down: Select  |  A: Edit Amount  |  Y: Add Item  |  X: Remove Item  |  Left/Right: Page  |  L/R: Category  |  B: Back";
                }
''',
    '''                    instructions = PokeBank::UIModel::inventoryFooter(
                        true, pocket.has_value(), hasSelection, quantityEditable);
                } else {
                    const bool categorySupported = selectedCategory >= 0 &&
                        selectedCategory < static_cast<int>(trainer.items.size());
                    bool hasSelection = false;
                    if (categorySupported) {
                        const auto visible = visibleItemIndices();
                        hasSelection = selectedItemIndex >= 0 &&
                            selectedItemIndex < static_cast<int>(visible.size());
                    }
                    instructions = PokeBank::UIModel::inventoryFooter(
                        !sourceReadOnly(), categorySupported, hasSelection, hasSelection);
                }
''',
)

replace_once(
    "Makefile.host.base",
    '''HOST_TESTS += $(HOST_BUILD)/test_classic_inventory_ui_model
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_inventory_ui_model_sanitize
''',
    '''HOST_TESTS += $(HOST_BUILD)/test_classic_inventory_ui_model $(HOST_BUILD)/test_inventory_ui_contract
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_inventory_ui_model_sanitize $(HOST_BUILD)/test_inventory_ui_contract_sanitize
''',
)
replace_once(
    "Makefile.host.base",
    '''$(HOST_BUILD)/test_classic_inventory_ui_model_sanitize: $(CLASSIC_UI_MODEL_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

''',
    '''$(HOST_BUILD)/test_classic_inventory_ui_model_sanitize: $(CLASSIC_UI_MODEL_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_inventory_ui_contract: tests/test_inventory_ui_contract.cpp include/UI/InventoryUIContract.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_inventory_ui_contract.cpp -o $@

$(HOST_BUILD)/test_inventory_ui_contract_sanitize: tests/test_inventory_ui_contract.cpp include/UI/InventoryUIContract.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_inventory_ui_contract.cpp -o $@

''',
)
replace_once(
    "Makefile.host.classic_inventory",
    '''	$(CXX) $(CXXFLAGS) tests/test_classic_inventory_ui_model.cpp src/UI/ClassicInventoryUIModel.cpp $(CATALOG_SOURCES) -o /tmp/test_classic_inventory_ui_model
	/tmp/test_classic_inventory_ui_model
''',
    '''	$(CXX) $(CXXFLAGS) tests/test_classic_inventory_ui_model.cpp src/UI/ClassicInventoryUIModel.cpp $(CATALOG_SOURCES) -o /tmp/test_classic_inventory_ui_model
	/tmp/test_classic_inventory_ui_model
	$(CXX) $(CXXFLAGS) tests/test_inventory_ui_contract.cpp -o /tmp/test_inventory_ui_contract
	/tmp/test_inventory_ui_contract
''',
)

replace_once(
    "tests/test_classic_inventory_ui_model.cpp",
    '#include "UI/ClassicInventoryUIModel.h"\n',
    '#include "UI/ClassicInventoryUIModel.h"\n#include "UI/InventoryUIContract.h"\n',
)
replace_once(
    "tests/test_classic_inventory_ui_model.cpp",
    '''    assert(classicInventoryAction(ClassicInventoryInput::B) == ClassicInventoryAction::Back);

''',
    '''    assert(classicInventoryAction(ClassicInventoryInput::B) == ClassicInventoryAction::Back);
    assert(classicInventoryAction(ClassicInventoryInput::X) == inventoryAction(InventoryInput::X));
    assert(classicInventoryAction(ClassicInventoryInput::Y) == inventoryAction(InventoryInput::Y));

''',
)
