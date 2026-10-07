#pragma once
#include "Inventory/ClassicInventoryCatalog.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/Common.h"
#include <algorithm>
#include <vector>

namespace PokeBank::UIModel::Gen2HeldItemPicker {
inline constexpr int columns = SharedHeldItemPicker::columns;
inline constexpr int rows = SharedHeldItemPicker::rows;
inline constexpr int pageSize = SharedHeldItemPicker::pageSize;
inline int initialIndex(const std::vector<uint8_t>& items, uint8_t current) {
    return SharedHeldItemPicker::initialIndex(items, current);
}
inline int move(int index, int count, int dx, int dy, int pages = 0) noexcept {
    return SharedHeldItemPicker::move(index, count, dx, dy, pages);
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
    const int count = static_cast<int>(items.size());
    if (count <= 0) return;

    const int cellWidth = width / Model::columns;
    constexpr int rowStep = 32;
    const int viewportH = Model::rows * rowStep;
    const int visualSelected = std::clamp(selected, 0, count - 1);
    constexpr int liveOffsetY = 0;

    const int first = visualSelected / Model::pageSize * Model::pageSize;
    const int last = std::min(count, first + Model::pageSize);
    const int drawFirst = std::max(0, first - Model::columns);
    const int drawLast = std::min(count, last + Model::columns);

    fb.setClipRect(x, y, width, viewportH);
    for (int i = drawFirst; i < drawLast; ++i) {
        const int off = i - first;
        const int cellX = x + (off % Model::columns) * cellWidth;
        const int cellY = y + (off / Model::columns) * rowStep + liveOffsetY;
        const bool focused = i == visualSelected;
        if (focused) {
            fb.drawFilledRoundedRect(cellX, cellY, cellWidth - 12, 30, 6, Colors::SurfaceSelected);
            fb.drawRoundedRect(cellX, cellY, cellWidth - 12, 30, 6, Colors::FocusBorder, 2);
        }
        fb.drawText(cellX + 10, cellY + 6, Model::itemName(items[static_cast<std::size_t>(i)]),
                    focused ? Colors::SelectedText : Colors::Text, TextStyle::Caption);
    }
    fb.clearClip();
}
}
