#pragma once
// L06 (Round 6, owner: "Held Item selection is unreliable ... some taps do nothing"): the one input
// step every Held Item grid (Gen II, Gen III, Gen IV/V) runs after its search dock has had the
// frame. A release-confirmed tap and A both end in the same `choose` result for the same index.
//
// Why taps used to be lost:
//   * the grid follows the finger from its first pixel, while the per-cell touch boxes required
//     the touch-down point to lie inside the box drawn AFTER that wobble, so a tap near a cell
//     edge with a few pixels of drift resolved to no box at all;
//   * the boxes left 10 px / 4 px dead gaps between cells;
//   * a box came after any box still published by the covered workspace (first match wins);
//   * a tap on a row the residual scroll had left partly visible took the A path and only
//     "brought the choice into view".
// The tap is now resolved from the grid geometry the frame was drawn with (offset before this
// frame's scroll update): the content moved with the finger, so the row under the release point
// is the row the finger landed on, in the column it landed in. The gesture must have started inside the grid viewport, and
// a drag past the tap slop is never a tap (TouchInput::justTapped).
#include "UI/SharedHeldItemPicker.h"
#include "UI/TouchScroll.h"

#include <algorithm>

namespace UI::SharedHeldItemInput {
namespace Grid = PokeBank::UIModel::SharedHeldItemPicker;

// The scroll viewport the grid is clipped to (SharedHeldItemPickerPresentation::drawGrid).
struct Viewport { int x, y, w, h; };
constexpr Viewport viewport(int modalX, int modalY, bool searchOpen) noexcept {
    return {modalX + Grid::gridInsetX, modalY + Grid::gridInsetY - 3, Grid::gridWidth(searchOpen),
            Grid::rows * Grid::rowPitch};
}
constexpr bool inside(const Viewport& v, int px, int py) noexcept {
    return px >= v.x && px < v.x + v.w && py >= v.y && py < v.y + v.h;
}

// The item whose cell row band holds content y `py` and whose column holds `px` (each cell
// counted at its full pitch, so the gaps between drawn boxes belong to a cell), or -1.
constexpr int cellIndex(int px, int py, int modalX, int modalY, bool searchOpen,
                        int firstRow, int scrollOffset, int count) noexcept {
    const auto v = viewport(modalX, modalY, searchOpen);
    if (count <= 0 || px < v.x || px >= v.x + v.w) return -1;
    const int columns = Grid::columnsForSearch(searchOpen);
    const int first = std::clamp(firstRow, 0, (count - 1) / columns);
    const int contentY = py - v.y - scrollOffset;
    const int rowOffset = contentY >= 0 ? contentY / Grid::rowPitch
                                        : -((-contentY + Grid::rowPitch - 1) / Grid::rowPitch);
    const int row = first + rowOffset;
    const int index = row * columns + (px - v.x) / Grid::cellWidth;
    return row >= 0 && index < count ? index : -1;
}

// The item drawn under a visible grid pixel, or -1.
constexpr int cellAt(int px, int py, int modalX, int modalY, bool searchOpen,
                     int firstRow, int scrollOffset, int count) noexcept {
    return inside(viewport(modalX, modalY, searchOpen), px, py)
        ? cellIndex(px, py, modalX, modalY, searchOpen, firstRow, scrollOffset, count) : -1;
}

// The cell a release-confirmed tap landed on. The grid only moves vertically, with the finger:
// the column is where the finger landed, the row is the content row now under the finger, which
// is the row it landed on. A sub-slop wobble therefore never changes or loses the choice.
template <class Touch>
int tappedCell(const Touch& touch, int modalX, int modalY, bool searchOpen,
               int firstRow, int scrollOffset, int count) noexcept {
    if (!touch.justTapped() || !inside(viewport(modalX, modalY, searchOpen), touch.startX(), touch.startY()))
        return -1;
    return cellIndex(touch.startX(), touch.y(), modalX, modalY, searchOpen, firstRow, scrollOffset, count);
}

struct Keys { bool up = false, down = false, left = false, right = false, l = false, r = false, a = false; };
struct Result {
    bool choose = false;     // apply the item at `selected` and close the picker
    bool revealed = false;   // A on a focus scrolled out of view: it was brought back, not applied
};

// One frame of grid input. `selected` is the focused index into the shown (sorted / searched)
// list; X (sort), B and the search dock are handled by the caller before this step.
inline Result update(const TouchInput& touch, TouchScrollState& scroll, TouchPickerViewport& view,
                     int& selected, int count, bool searchOpen, int modalX, int modalY, const Keys& keys) {
    if (count <= 0) { scroll.stop(); return {}; }
    const int columns = Grid::columnsForSearch(searchOpen);
    selected = std::clamp(selected, 0, count - 1);
    view.ensure(selected, count, Grid::rows, columns);
    // Resolved against the geometry the previous frame drew, before this frame's scroll update.
    const int tapped = tappedCell(touch, modalX, modalY, searchOpen, view.firstRow, scroll.offset(), count);
    const int maxFirst = TouchPickerViewport::maxFirst(count, Grid::rows, columns);
    if (maxFirst > 0) {
        const auto v = viewport(modalX, modalY, searchOpen);
        scroll.updateVertical(touch, v.x, v.y, v.w, v.h, Grid::rowPitch, view.firstRow, maxFirst + 1);
    } else {
        scroll.stop();
    }
    if (tapped >= 0) {
        // A visible cell the finger chose: the same commit A makes, never a "bring into view".
        scroll.stop();
        selected = tapped;
        view.reveal(selected, count, Grid::rows, columns);
        return {true, false};
    }
    const bool moved = keys.up || keys.down || keys.left || keys.right || keys.l || keys.r;
    if (moved) {
        scroll.stop();
        // The shown grid's own column count: with the search dock open it is two columns wide.
        const auto step = [&](int dx, int dy, int pages) {
            if (pages) return Grid::move(selected, count, 0, 0, pages);
            if (dx && (selected % columns + dx < 0 || selected % columns + dx >= columns)) return selected;
            const int next = selected + dx + columns * dy;
            return next >= 0 && next < count ? next : selected;
        };
        if (keys.up) selected = step(0, -1, 0);
        if (keys.down) selected = step(0, 1, 0);
        if (keys.left) selected = step(-1, 0, 0);
        if (keys.right) selected = step(1, 0, 0);
        if (keys.l) selected = step(0, 0, -1);
        if (keys.r) selected = step(0, 0, 1);
        view.reveal(selected, count, Grid::rows, columns);
    }
    if (!keys.a) return {};
    scroll.stop();
    if (!view.containsSelection(selected, count, Grid::rows, columns)) {
        // The finger can browse far from the focus; A never commits a hidden choice.
        view.reveal(selected, count, Grid::rows, columns);
        return {false, true};
    }
    return {true, false};
}

} // namespace UI::SharedHeldItemInput
