#pragma once
#include "Inventory/ClassicInventoryCatalog.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/Common.h"
#include <algorithm>
#include <vector>

namespace PokeBank::UIModel::Gen2HeldItemPicker {
inline constexpr int columns = 2, rows = 12, pageSize = columns * rows;
inline int initialIndex(const std::vector<uint8_t>& items, uint8_t current) {
    const auto found = std::find(items.begin(), items.end(), current);
    return found == items.end() ? 0 : static_cast<int>(found - items.begin());
}
constexpr int move(int index, int count, int dx, int dy, int pages = 0) noexcept {
    if (count <= 0) return 0;
    index = std::clamp(index, 0, count - 1);
    if (pages) {
        const int page = std::clamp(index / pageSize + pages, 0, (count - 1) / pageSize);
        int next = page * pageSize + index % pageSize;
        while (next >= count && next >= page * pageSize + columns) next -= columns;
        return next < count ? next : page * pageSize;
    }
    if (dx && (index % columns + dx < 0 || index % columns + dx >= columns)) return index;
    const int next = index + dx + columns * dy;
    return next >= 0 && next < count ? next : index;
}
inline std::string itemName(uint8_t item) {
    if (item == 0) return "None";
    // Gold, Silver and Crystal share this exact machine map and catalog formatter.
    using namespace PokeVault::Inventory;
    return displayItemName(ClassicGame::Gold, ClassicPocket::PCItems, item);
}
}

namespace UI::Gen2HeldItemPickerPresentation {
inline void drawList(PKSEFramebuffer& fb, int x, int y, int width,
                     const std::vector<uint8_t>& items, int selected) {
    namespace Model = PokeBank::UIModel::Gen2HeldItemPicker;
    const int first = selected / Model::pageSize * Model::pageSize;
    const int last = std::min(static_cast<int>(items.size()), first + Model::pageSize);
    const int cellWidth = width / Model::columns;
    for (int i = first; i < last; ++i) {
        const int cellX = x + (i - first) % Model::columns * cellWidth;
        const int cellY = y + (i - first) / Model::columns * 32;
        if (i == selected) fb.drawRoundedRect(cellX, cellY, cellWidth - 12, 30, 6, Colors::FocusBorder, 2);
        fb.drawText(cellX + 10, cellY + 6, Model::itemName(items[static_cast<std::size_t>(i)]),
                    Colors::Text, TextStyle::Caption);
    }
}
}
