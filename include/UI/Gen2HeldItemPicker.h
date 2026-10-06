#pragma once
#include "Inventory/ClassicInventoryCatalog.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/Common.h"
#include "UI/TouchInput.h"
#include <algorithm>
#include <cstdlib>
#include <vector>

namespace PokeBank::UIModel::Gen2HeldItemPicker {
inline constexpr int columns = SharedHeldItemPicker::columns;
inline constexpr int rows = SharedHeldItemPicker::rows;
inline constexpr int pageSize = SharedHeldItemPicker::pageSize;
inline int initialIndex(const std::vector<uint8_t>& items, uint8_t current) {
    return SharedHeldItemPicker::initialIndex(items, current);
}
inline int move(int index, int count, int dx, int dy, int pages = 0) noexcept {
    // The preserved editor calls move(..., 0, +/-1) once when a touch drag is released. Fold the
    // actual number of 32 px grid rows crossed into that one semantic commit so the selection lands
    // exactly where the live, finger-tracked presentation was showing it. Physical controller calls
    // are unchanged because released is true only on the touchscreen release frame.
    const auto& touch = UI::latestTouchGesture();
    if (pages == 0 && touch.released && touch.dragged && dy != 0 && dx == 0) {
        const int rowsCrossed = std::max(1, std::abs(touch.deltaY) / 32);
        dy = dy > 0 ? rowsCrossed : -rowsCrossed;
    }
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
    int visualSelected = std::clamp(selected, 0, count - 1);
    int liveOffsetY = 0;

    const auto& touch = latestTouchGesture();
    const bool fingerInGrid = touch.down &&
        touch.startX >= x && touch.startX < x + width &&
        touch.startY >= y && touch.startY < y + viewportH;
    if (fingerInGrid) {
        const int requestedRows = -touch.deltaY / rowStep;
        const int wanted = selected + requestedRows * Model::columns;
        visualSelected = std::clamp(wanted, 0, count - 1);
        // Keep the selected column stable when the final partial row is short.
        const int selectedColumn = selected % Model::columns;
        int visualRow = visualSelected / Model::columns;
        int candidate = visualRow * Model::columns + selectedColumn;
        if (candidate >= count) candidate = count - 1;
        visualSelected = std::max(0, candidate);
        const int appliedRows = visualSelected / Model::columns - selected / Model::columns;
        liveOffsetY = touch.deltaY + appliedRows * rowStep;
        if ((visualSelected / Model::columns == 0 && liveOffsetY > 0) ||
            (visualSelected / Model::columns == (count - 1) / Model::columns && liveOffsetY < 0))
            liveOffsetY /= 3;
    }

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
