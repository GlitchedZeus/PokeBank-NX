#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one replacement target, found {count}")
    p.write_text(text.replace(old, new, 1))


contract = Path("include/UI/InventoryUIContract.h")
contract_text = contract.read_text()
if "useReadOnlyInventoryFooterOverride" not in contract_text:
    replace_once(
        str(contract),
        "\nstruct InventoryPickerLayout {\n",
        "\n[[nodiscard]] constexpr bool useReadOnlyInventoryFooterOverride(\n"
        "    bool sourceReadOnly, bool detailsActive, bool settingsMode,\n"
        "    bool stagedInventoryActionsAvailable) noexcept {\n"
        "    return sourceReadOnly && !detailsActive && !settingsMode &&\n"
        "           !stagedInventoryActionsAvailable;\n"
        "}\n\n"
        "struct InventoryPickerLayout {\n",
    )
    replace_once(
        str(contract),
        "    static constexpr int FooterHeight = 48;\n",
        "    static constexpr int FooterHeight = 64;\n",
    )

header = Path("include/UI/ClassicInventoryOverlay.h")
header_text = header.read_text()
if "stagedEditingAvailable" not in header_text:
    replace_once(
        str(header),
        "[[nodiscard]] bool isClassicSource(const TrainerViewScreen& screen) noexcept;\n",
        "[[nodiscard]] bool isClassicSource(const TrainerViewScreen& screen) noexcept;\n"
        "// True only when PokeBank owns a staged classic editor for this read-only source.\n"
        "[[nodiscard]] bool stagedEditingAvailable(TrainerViewScreen& screen);\n",
    )

cpp = Path("src/UI/ClassicInventoryOverlay.cpp")
cpp_text = cpp.read_text()
if "bool stagedEditingAvailable(TrainerViewScreen& screen)" not in cpp_text:
    replace_once(
        str(cpp),
        "void drawRow(PKSEFramebuffer& fb, int x, int y, int width, const std::string& text,\n"
        "             bool selected, int height = 48) {\n"
        "    fb.drawFilledRoundedRect(x, y, width, height, 10, selected ? Colors::AccentDim : Colors::PanelAlt);\n"
        "    if (selected) fb.drawRoundedRect(x, y, width, height, 10, Colors::Accent, 2);\n"
        "    const int ty = y + (height - fb.lineHeight(TextStyle::Body)) / 2;\n"
        "    fb.drawText(x + 18, ty, text, selected ? Colors::Text : Colors::TextDim, TextStyle::Body);\n"
        "}\n",
        "void drawRow(PKSEFramebuffer& fb, int x, int y, int width, const std::string& text,\n"
        "             bool selected, int height = 48, bool brightUnselected = false) {\n"
        "    fb.drawFilledRoundedRect(x, y, width, height, 10, selected ? Colors::AccentDim : Colors::PanelAlt);\n"
        "    if (selected) fb.drawRoundedRect(x, y, width, height, 10, Colors::Accent, 2);\n"
        "    const int ty = y + (height - fb.lineHeight(TextStyle::Body)) / 2;\n"
        "    const auto textColor = (selected || brightUnselected) ? Colors::Text : Colors::TextDim;\n"
        "    fb.drawText(x + 18, ty, text, textColor, TextStyle::Body);\n"
        "}\n",
    )
    replace_once(
        str(cpp),
        "bool isClassicSource(const TrainerViewScreen& screen) noexcept {\n"
        "    return sourceKind(screen.sourceGameId) != SourceKind::None &&\n"
        "           PokeBank::UIModel::classicInventoryGame(screen.sourceGameId).has_value();\n"
        "}\n",
        "bool isClassicSource(const TrainerViewScreen& screen) noexcept {\n"
        "    return sourceKind(screen.sourceGameId) != SourceKind::None &&\n"
        "           PokeBank::UIModel::classicInventoryGame(screen.sourceGameId).has_value();\n"
        "}\n\n"
        "bool stagedEditingAvailable(TrainerViewScreen& screen) {\n"
        "    if (!isClassicSource(screen)) return false;\n"
        "    std::string error;\n"
        "    return backendAvailable(screen, error);\n"
        "}\n",
    )
    replace_once(
        str(cpp),
        "            drawRow(fb, x + 12, rowY, width - 24, label, i == state.pickerRow, rowH - 4);\n",
        "            drawRow(fb, x + 12, rowY, width - 24, label, i == state.pickerRow, rowH - 4, true);\n",
    )
    replace_once(
        str(cpp),
        "        fb.drawText(x + pad, y + height - 34,\n"
        "                    \"D-pad/Stick Navigate   A Add/Select   B Cancel   L/R Page\",\n"
        "                    Colors::TextDim, TextStyle::Caption);\n",
        "        // Two readable lines fit the fixed 560px panel without shrinking the text.\n"
        "        fb.drawText(x + pad, y + height - 56,\n"
        "                    \"D-pad/Stick Navigate     A Add/Select\",\n"
        "                    Colors::Text, TextStyle::Caption);\n"
        "        fb.drawText(x + pad, y + height - 30,\n"
        "                    \"B Cancel                  L/R Page\",\n"
        "                    Colors::Text, TextStyle::Caption);\n",
    )

base = Path("src/UI/TrainerViewScreenBase.inc")
base_text = base.read_text()
if "stagedInventoryActionsAvailable" not in base_text:
    replace_once(
        str(base),
        "            if (selectedMode == ViewMode::Items) {\n"
        "                if (ClassicInventory::isClassicSource(*this)) {\n",
        "            if (selectedMode == ViewMode::Items) {\n"
        "                if (ClassicInventory::isClassicSource(*this) &&\n"
        "                    ClassicInventory::stagedEditingAvailable(*this)) {\n",
    )
    replace_once(
        str(base),
        "        if (standardScreen) {\n"
        "            if (sourceReadOnly() && !details.active && selectedMode != ViewMode::Settings) {\n",
        "        if (standardScreen) {\n"
        "            const bool stagedInventoryActionsAvailable =\n"
        "                detailViewActive && selectedMode == ViewMode::Items &&\n"
        "                ClassicInventory::stagedEditingAvailable(*this);\n"
        "            if (PokeBank::UIModel::useReadOnlyInventoryFooterOverride(\n"
        "                    sourceReadOnly(), details.active, selectedMode == ViewMode::Settings,\n"
        "                    stagedInventoryActionsAvailable)) {\n",
    )

ui_test = Path("tests/test_inventory_ui_contract.cpp")
test_text = ui_test.read_text()
if "staged editor must bypass generic read-only footer" not in test_text:
    replace_once(
        str(ui_test),
        "    constexpr auto readOnly = inventoryActionAvailability(false, true, true, true);\n"
        "    static_assert(!readOnly.editAmount && !readOnly.addItem && !readOnly.removeItem);\n",
        "    constexpr auto readOnly = inventoryActionAvailability(false, true, true, true);\n"
        "    static_assert(!readOnly.editAmount && !readOnly.addItem && !readOnly.removeItem);\n"
        "    constexpr auto fixedQuantity = inventoryActionAvailability(true, true, true, false);\n"
        "    static_assert(!fixedQuantity.editAmount && fixedQuantity.addItem && fixedQuantity.removeItem);\n\n"
        "    // A read-only source with a real PokeBank staged editor must bypass generic read-only footer.\n"
        "    static_assert(useReadOnlyInventoryFooterOverride(true, false, false, false));\n"
        "    static_assert(!useReadOnlyInventoryFooterOverride(true, false, false, true));\n"
        "    static_assert(!useReadOnlyInventoryFooterOverride(false, false, false, false));\n"
        "    static_assert(!useReadOnlyInventoryFooterOverride(true, true, false, false));\n"
        "    static_assert(!useReadOnlyInventoryFooterOverride(true, false, true, false));\n",
    )
    replace_once(
        str(ui_test),
        "    const std::string emptyFooter = inventoryFooter(true, true, false, false);\n"
        "    assert(emptyFooter.find(\"X: Add Item\") != std::string::npos);\n"
        "    assert(emptyFooter.find(\"A: Edit Amount\") == std::string::npos);\n"
        "    assert(emptyFooter.find(\"Y: Remove Item\") == std::string::npos);\n",
        "    const std::string emptyFooter = inventoryFooter(true, true, false, false);\n"
        "    assert(emptyFooter.find(\"X: Add Item\") != std::string::npos);\n"
        "    assert(emptyFooter.find(\"A: Edit Amount\") == std::string::npos);\n"
        "    assert(emptyFooter.find(\"Y: Remove Item\") == std::string::npos);\n"
        "    assert(emptyFooter.find(\"L/R: Category\") != std::string::npos);\n"
        "    assert(emptyFooter.find(\"+: Options\") != std::string::npos);\n"
        "    assert(emptyFooter.find(\"-: Help\") != std::string::npos);\n"
        "    assert(emptyFooter.find(\"B: Back\") != std::string::npos);\n\n"
        "    const std::string fixedFooter = inventoryFooter(true, true, true, false);\n"
        "    assert(fixedFooter.find(\"A: Edit Amount\") == std::string::npos);\n"
        "    assert(fixedFooter.find(\"X: Add Item\") != std::string::npos);\n"
        "    assert(fixedFooter.find(\"Y: Remove Item\") != std::string::npos);\n",
    )
    replace_once(
        str(ui_test),
        "    static_assert(InventoryPickerLayout::RowHeight == 40);\n"
        "    static_assert(InventoryPickerLayout::Width < 900);\n",
        "    static_assert(InventoryPickerLayout::RowHeight == 40);\n"
        "    static_assert(InventoryPickerLayout::FooterHeight == 64);\n"
        "    static_assert(InventoryPickerLayout::Width < 900);\n",
    )

print("inventory footer/readability polish applied")
