#pragma once

#include "UI/Common.h"
#include "UI/ItemPickerArtwork.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedHeldItemPicker.h"

#include <algorithm>
#include <string>
#include <vector>

namespace UI::SharedHeldItemPickerPresentation {

// The accepted Generation III four-column layout is the presentation contract
// for every generation with native held items. Game-specific labels and sprite
// lookups stay at the call site; no item IDs are converted across generations.
namespace Grid = PokeBank::UIModel::SharedHeldItemPicker;

inline void drawHeading(PKSEFramebuffer& fb, int x, int y,
                        const char* title, int selected, int count) {
    fb.drawText(x + 28, y + 20, title, Colors::Text, TextStyle::Heading);
    const int pages = std::max(1, (count + Grid::pageSize - 1) / Grid::pageSize);
    const int current = count > 0 ? std::clamp(selected, 0, count - 1) : 0;
    fb.drawText(x + 28, y + 55,
                "Page " + std::to_string(current / Grid::pageSize + 1) +
                    " / " + std::to_string(pages),
                Colors::TextDim, TextStyle::Caption);
}

template <typename Item, typename LabelFor, typename SpriteNameFor>
inline void drawGrid(PKSEFramebuffer& fb, int x, int y,
                     const std::vector<Item>& items, int selected,
                     LabelFor labelFor, SpriteNameFor spriteNameFor) {
    const int count = static_cast<int>(items.size());
    if (count == 0) return;
    const int focused = std::clamp(selected, 0, count - 1);
    const int first = focused / Grid::pageSize * Grid::pageSize;
    const int last = std::min(count, first + Grid::pageSize);
    const int gridX = x + Grid::gridInsetX;
    const int gridY = y + Grid::gridInsetY;
    const int cellWidth = (Grid::modalWidth - 2 * Grid::gridInsetX) / Grid::columns;
    for (int i = first; i < last; ++i) {
        const int offset = i - first;
        const int cellX = gridX + (offset % Grid::columns) * cellWidth;
        const int cellY = gridY + (offset / Grid::columns) * Grid::rowPitch;
        if (i == focused)
            fb.drawSelectionHighlight(cellX, cellY - 3, cellWidth - 10, Grid::rowPitch - 4);
        const Item value = items[static_cast<std::size_t>(i)];
        fb.drawText(cellX + 10, cellY + 7, labelFor(value),
                    Colors::Text, TextStyle::Caption);
        if (value != 0)
            ItemPickerArtwork::draw(fb, cellX + cellWidth - 24,
                                    cellY + 2, 30, spriteNameFor(value));
    }
}

} // namespace UI::SharedHeldItemPickerPresentation
