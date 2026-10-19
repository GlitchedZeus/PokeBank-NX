#pragma once

#include "UI/Common.h"
#include "UI/ItemPickerArtwork.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/SharedMenuGeometry.h"

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
                     LabelFor labelFor, SpriteNameFor spriteNameFor,
                     int firstVisibleRow = -1, int scrollOffset = 0) {
    const int count = static_cast<int>(items.size());
    if (count == 0) return;
    const int focused = std::clamp(selected, 0, count - 1);
    const int first = firstVisibleRow < 0 ? focused / Grid::pageSize * Grid::pageSize
        : std::clamp(firstVisibleRow, 0, (count - 1) / Grid::columns) * Grid::columns;
    const int last = std::min(count, first + Grid::pageSize + Grid::columns);
    const int drawFirst = std::max(0, first - Grid::columns);
    const int gridX = x + Grid::gridInsetX;
    const int gridY = y + Grid::gridInsetY;
    const int cellWidth = (Grid::modalWidth - 2 * Grid::gridInsetX) / Grid::columns;
    fb.setClipRect(gridX, gridY - 3, Grid::modalWidth - 2 * Grid::gridInsetX,
                   Grid::rows * Grid::rowPitch);
    for (int i = drawFirst; i < last; ++i) {
        const int offset = i - first;
        const int cellX = gridX + (offset % Grid::columns) * cellWidth;
        const int cellY = gridY + (offset / Grid::columns) * Grid::rowPitch + scrollOffset;
        // R2-BUG-012: every cell is its own boxed row; the focused one has the yellow outline.
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(cellX, cellY - 3, cellWidth - 10, Grid::rowPitch - 4),
                                 {i == focused});
        const Item value = items[static_cast<std::size_t>(i)];
        // Phase 2 (owner round 1: "the held item menu just needs a smidge the text and sprites
        // closer"): the sprite leads the cell and the name follows it at the one fixed gap of the
        // HeldItemPicker rule (UI/SharedMenuGeometry.h), instead of sitting at the far right.
        constexpr auto rule = PokeBank::UIModel::SharedMenuGeometry::rule(
            PokeBank::UIModel::SharedMenuGeometry::MenuType::HeldItemPicker);
        const int spriteRight = cellX + rule.cellPadX + rule.spriteSize;
        if (value != 0)
            ItemPickerArtwork::draw(fb, spriteRight, cellY + 2, rule.spriteSize, spriteNameFor(value));
        fb.drawText(spriteRight + rule.spriteGap, cellY + 7, labelFor(value),
                    Colors::Text, TextStyle::Caption);
    }
    fb.clearClip();
}

} // namespace UI::SharedHeldItemPickerPresentation
