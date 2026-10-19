#pragma once
// R2-BUG-006: ONE compact multi-column item picker for the long Gen I-V item catalogs (Bag "Add
// Item" today; any long item list that wants it). Replaces the single-column lists that needed a
// screen-height of scrolling per pocket.
//
//  * Geometry: the accepted Held Item grid rule (SharedMenuGeometry MenuType::HeldItemPicker:
//    1040 px frame, 40 px row pitch, 30 px leading sprite, Caption labels), up to 4 columns x 10
//    rows, height content-driven so a short pocket draws a short frame and the footer stays free.
//  * Names are never clipped to reach 40 cells: the column count drops 4 -> 3 -> 2 until the
//    widest real label of the list fits its cell (measured with the bundled font's advances).
//  * One rectangle per cell is BOTH the focus highlight and the touch target: drawCells hands the
//    clipped cell to the caller's `cellChrome`, which production passes as
//    SharedEditorChrome::drawMenuCell (Lane S restyles it for the boxed rows of R2-BUG-012). The
//    hook keeps this header free of libnx so the grid is host-tested.
//  * Navigation (pure, host-tested): D-pad / stick moves in the grid (Up/Down wrap by column,
//    Left/Right stay in the row), L/R page by one screen of rows keeping the column, ZL/ZR jump
//    five pages. The selected INDEX is the identity: a reflow (column count, sort, filter) keeps
//    it and the window follows it, so focus can never sit off-screen.
//  * The draggable SharedScrollbar scrolls by grid rows; a scroll keeps the selected column.
//  * R2-BUG-005: when the focused item is a TM/HM, the header shows its move's name, type badge,
//    PWR, ACC and PP ("—" for a non-damaging move / never-miss accuracy) — read from the
//    generation's own machine table by the caller (machineMoveAt) and SharedMoveFacts. A TM
//    number is never assumed to be the same move across generations.

#include "Enums/GameVersion.h"
#include "Names/MoveNames.h"
#include "UI/ClassicTypeBadges.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedMoveFacts.h"
#include "UI/SharedScrollbar.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace UI::SharedItemGridPicker {

namespace MG = PokeBank::UIModel::SharedMenuGeometry;
namespace Facts = PokeBank::UIModel::SharedMoveFacts;

inline constexpr MG::Rule kRule = MG::rule(MG::MenuType::HeldItemPicker);
inline constexpr int kMaxColumns = kRule.columns;   // 4
inline constexpr int kMinColumns = 2;
inline constexpr int kMaxRows = kRule.maxVisibleRows;   // 10
inline constexpr int kCellGapX = 6;                 // same gutter SharedMenuGeometry gives grid cells
inline constexpr int kHeldColumnW = 36;             // "x999" at Caption
inline constexpr int kDetailTop = 64;               // TM/HM detail line, inside the 90 px header
static_assert(kRule.kind == MG::Kind::Grid && kMaxColumns == 4 && kMaxRows == 10);

struct Grid {
    int count = 0;
    int columns = kMaxColumns;
    int totalRows = 0;
    int visibleRows = 0;
    int cellWidth = 0;
    MG::Rect frame{};
    MG::Rect list{};

    [[nodiscard]] constexpr int page() const noexcept { return visibleRows * columns; }
    [[nodiscard]] constexpr bool scrolls() const noexcept { return totalRows > visibleRows; }
    [[nodiscard]] constexpr int maxFirstRow() const noexcept { return std::max(0, totalRows - visibleRows); }
};

constexpr int ceilDiv(int a, int b) noexcept { return b <= 0 ? 0 : (a + b - 1) / b; }

// Content-driven layout of `count` items in `columns` columns on the 1280x720 screen.
constexpr Grid grid(int count, int columns, int screenW = MG::kScreenW, int screenH = MG::kScreenH) noexcept {
    Grid g;
    g.count = std::max(0, count);
    g.columns = std::clamp(columns, 1, kMaxColumns);
    g.totalRows = ceilDiv(g.count, g.columns);
    const int usableH = screenH - MG::kFooterH - 2 * MG::kMargin;
    const int fit = (usableH - kRule.headerH - kRule.padBottom) / kRule.rowHeight;
    g.visibleRows = std::max(1, std::min({kMaxRows, fit, std::max(1, g.totalRows)}));
    const int w = std::min(kRule.width, screenW - 2 * MG::kMargin);
    const int h = kRule.headerH + g.visibleRows * kRule.rowHeight + kRule.padBottom;
    g.frame = {(screenW - w) / 2, (screenH - MG::kFooterH - h) / 2, w, h};
    g.list = {g.frame.x + kRule.padX, g.frame.y + kRule.headerH, w - 2 * kRule.padX,
              g.visibleRows * kRule.rowHeight};
    g.cellWidth = g.list.w / g.columns;
    return g;
}

// Width left for a cell's name after the sprite (and the held-quantity column when shown).
constexpr int labelWidth(const Grid& g, bool heldColumn) noexcept {
    return g.cellWidth - kCellGapX - 2 * kRule.cellPadX - kRule.spriteSize - kRule.spriteGap -
           (heldColumn ? kHeldColumnW + 6 : 0);
}

// The widest column count whose cells fit every real label of the list (never clip a name).
template <class LabelAt>
inline int columnsFor(int count, LabelAt&& labelAt, bool heldColumn) {
    for (int columns = kMaxColumns; columns > kMinColumns; --columns) {
        const int room = labelWidth(grid(count, columns), heldColumn);
        bool fits = true;
        for (int i = 0; i < count && fits; ++i)
            fits = MG::fitsWidth(labelAt(i), MG::Text::Caption, room);
        if (fits) return columns;
    }
    return kMinColumns;
}
template <class LabelAt>
inline Grid gridFor(int count, LabelAt&& labelAt, bool heldColumn = true) {
    return grid(count, columnsFor(count, labelAt, heldColumn));
}

// ---- Window / mapping ------------------------------------------------------------------------

constexpr int rowOf(const Grid& g, int index) noexcept { return std::max(0, index) / g.columns; }
constexpr int clampFirstRow(const Grid& g, int firstRow) noexcept {
    return std::clamp(firstRow, 0, g.maxFirstRow());
}
// Window that centres the selection (cursor-driven pickers).
constexpr int firstRowFor(const Grid& g, int selected) noexcept {
    return clampFirstRow(g, rowOf(g, selected) - g.visibleRows / 2);
}
constexpr bool containsSelection(const Grid& g, int firstRow, int selected) noexcept {
    if (selected < 0 || selected >= g.count) return false;
    const int row = rowOf(g, selected);
    return row >= firstRow && row < firstRow + g.visibleRows;
}
// Smallest window move that shows the selection (viewport-driven pickers).
constexpr int revealFirstRow(const Grid& g, int firstRow, int selected) noexcept {
    const int row = rowOf(g, std::clamp(selected, 0, std::max(0, g.count - 1)));
    if (row < firstRow) firstRow = row;
    else if (row >= firstRow + g.visibleRows) firstRow = row - g.visibleRows + 1;
    return clampFirstRow(g, firstRow);
}
// A scroll to `firstRow` in a cursor-driven picker: keep the column, land in the middle row.
constexpr int indexForFirstRow(const Grid& g, int selected, int firstRow) noexcept {
    if (g.count <= 0) return 0;
    const int column = std::clamp(selected, 0, g.count - 1) % g.columns;
    const int row = std::clamp(clampFirstRow(g, firstRow) + g.visibleRows / 2, 0, g.totalRows - 1);
    return std::min(g.count - 1, row * g.columns + column);
}

// Cell `index` drawn with the window at `firstRow` plus a live pixel offset.
constexpr MG::Rect cellRect(const Grid& g, int index, int firstRow, int pixelOffset = 0) noexcept {
    const int row = index / g.columns - firstRow;
    const int column = index % g.columns;
    return {g.list.x + column * g.cellWidth, g.list.y + row * kRule.rowHeight + pixelOffset,
            g.cellWidth - kCellGapX, kRule.rowHeight - kRule.rowGap};
}
constexpr MG::Rect clip(const MG::Rect& r, const MG::Rect& to) noexcept {
    const int left = std::max(r.x, to.x), top = std::max(r.y, to.y);
    const int right = std::min(r.right(), to.right()), bottom = std::min(r.bottom(), to.bottom());
    return right > left && bottom > top ? MG::Rect{left, top, right - left, bottom - top} : MG::Rect{};
}
// The item a touch at (x, y) lands on with the window at `firstRow`, or -1 (gaps and outside miss).
constexpr int hitIndex(const Grid& g, int firstRow, int x, int y) noexcept {
    if (!g.list.contains(x, y)) return -1;
    const int column = (x - g.list.x) / g.cellWidth;
    const int row = (y - g.list.y) / kRule.rowHeight;
    if (column >= g.columns || row >= g.visibleRows) return -1;
    const int index = (firstRow + row) * g.columns + column;
    if (index >= g.count || !cellRect(g, index, firstRow).contains(x, y)) return -1;
    return index;
}
constexpr bool onGrid(const Grid& g, int x, int y) noexcept { return g.list.contains(x, y); }

// Scrollbar beside the grid, in the frame's right padding; it scrolls by rows.
constexpr MG::Rect scrollbarTrack(const Grid& g, int trackWidth) noexcept {
    return {g.list.right() + (kRule.padX - trackWidth) / 2, g.list.y, trackWidth, g.list.h};
}
constexpr SharedScrollbar::Model scrollModel(const Grid& g, int firstRow) noexcept {
    return SharedScrollbar::rows(g.totalRows, g.visibleRows, clampFirstRow(g, firstRow));
}

// ---- Navigation --------------------------------------------------------------------------------

struct Keys {
    bool up = false, down = false, left = false, right = false;
    bool l = false, r = false, zl = false, zr = false;
    [[nodiscard]] constexpr bool any() const noexcept { return up || down || left || right || l || r || zl || zr; }
};

constexpr int navigate(const Grid& g, int index, const Keys& k) noexcept {
    if (g.count <= 0) return 0;
    index = std::clamp(index, 0, g.count - 1);
    const int cols = g.columns;
    const int lastRow = g.totalRows - 1;
    if (k.left && index % cols > 0) --index;
    if (k.right && index % cols < cols - 1 && index + 1 < g.count) ++index;
    if (k.up) {
        const int row = index / cols, column = index % cols;
        index = row > 0 ? index - cols : std::min(g.count - 1, lastRow * cols + column);   // wrap to bottom
    }
    if (k.down) {
        const int row = index / cols, column = index % cols;
        if (row >= lastRow) index = column;                                   // wrap to top, same column
        else index = std::min(g.count - 1, index + cols);                     // partial last row: last item
    }
    const auto page = [&](int pages) {
        const int column = index % cols;
        const int row = std::clamp(index / cols + pages * g.visibleRows, 0, lastRow);
        return std::min(g.count - 1, row * cols + column);
    };
    if (k.l) index = page(-1);
    if (k.r) index = page(1);
    if (k.zl) index = page(-5);
    if (k.zr) index = page(5);
    return index;
}

// ---- Drawing -----------------------------------------------------------------------------------

// Dim the screen above the footer and draw the frame, title and subtitle (position appended).
inline void drawFrame(PKSEFramebuffer& fb, const Grid& g, std::string_view title,
                      const std::string& subtitle, int selected) {
    const auto& f = g.frame;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - MG::kFooterH, Color(0, 0, 0, 160));
    fb.drawFilledRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Panel);
    fb.drawRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Divider, 1);
    fb.drawText(f.x + kRule.padX, f.y + 8, std::string(title), Colors::Text, TextStyle::Heading);
    const std::string position = g.count > 0
        ? std::to_string(std::clamp(selected, 0, g.count - 1) + 1) + " / " + std::to_string(g.count)
        : std::string("0 / 0");
    fb.drawText(f.x + kRule.padX, f.y + 40, subtitle.empty() ? position : subtitle + "  |  " + position,
                Colors::TextSecondary, TextStyle::Caption);
}

// R2-BUG-005: one compact line for the focused TM/HM: move name, type badge, PWR / ACC / PP.
// `moveId` comes from the generation's own machine table (0 = not a machine: nothing drawn).
// Returns the drawn width (0 when nothing was drawn).
inline int drawMachineDetail(PKSEFramebuffer& fb, int x, int y, int maxW, Enums::GameVersion game,
                             uint16_t moveId) {
    if (moveId == 0) return 0;
    const auto f = Facts::facts(moveId, game);
    if (!f.exists) return 0;
    int cx = x;
    const std::string name = Names::getMoveName(moveId);
    int w = 0, h = 0;
    fb.measureText(name, w, h, TextStyle::Caption);
    fb.drawText(cx, y, name, Colors::Text, TextStyle::Caption);
    cx += w + 12;
    const int badge = (!f.typeVaries && f.type <= ClassicTypeBadges::kLastGen15Type)
        ? ClassicTypeBadges::drawCanonical(fb, cx, y + 1, f.type, 16) : 0;
    if (badge > 0) cx += badge + 12;
    else {
        const std::string type = Facts::typeLabel(f);
        fb.measureText(type, w, h, TextStyle::Caption);
        fb.drawText(cx, y, type, Colors::TextSecondary, TextStyle::Caption);
        cx += w + 12;
    }
    const std::string stats = "PWR " + Facts::powerLabel(f) + "   ACC " + Facts::accuracyLabel(f) +
                              "   PP " + (f.basePP > 0 ? std::to_string(f.basePP) : std::string("\xE2\x80\x94"));
    fb.measureText(stats, w, h, TextStyle::Caption);
    if (cx + w <= x + maxW) {
        fb.drawText(cx, y, stats, Colors::TextSecondary, TextStyle::Caption);
        cx += w;
    }
    return cx - x;
}

// The cells. cellChrome(visibleRect, i, focused) draws the focus highlight AND publishes the touch
// target on the cell's visible pixels (production: SharedEditorChrome::drawMenuCell with id
// touchBase + i); labelAt(i) -> std::string; drawSpriteAt(fb, spriteRect, i); heldAt(i) -> quantity
// already held (0 = none; pass a lambda returning 0 for lists without a held column).
template <class CellChrome, class LabelAt, class DrawSpriteAt, class HeldAt>
inline void drawCells(PKSEFramebuffer& fb, const Grid& g, int selected, int firstRow, int pixelOffset,
                      CellChrome&& cellChrome, LabelAt&& labelAt, DrawSpriteAt&& drawSpriteAt,
                      HeldAt&& heldAt) {
    if (g.count <= 0) return;
    firstRow = clampFirstRow(g, firstRow);
    const int firstDraw = std::max(0, firstRow - 1) * g.columns;
    const int lastDraw = std::min(g.count, (firstRow + g.visibleRows + 1) * g.columns);
    fb.setClipRect(g.list.x, g.list.y, g.list.w, g.list.h);
    for (int i = firstDraw; i < lastDraw; ++i) {
        const auto cell = cellRect(g, i, firstRow, pixelOffset);
        const auto visible = clip(cell, g.list);
        if (visible.w <= 0) continue;
        const bool focused = i == selected;
        cellChrome(visible, i, focused);
        const MG::Rect sprite{cell.x + kRule.cellPadX, cell.y + (cell.h - kRule.spriteSize) / 2,
                              kRule.spriteSize, kRule.spriteSize};
        drawSpriteAt(fb, sprite, i);
        const int textX = sprite.right() + kRule.spriteGap;
        const int textY = MG::textTop(cell, MG::Text::Caption);
        fb.drawText(textX, textY, labelAt(i), focused ? Colors::Text : Colors::TextSecondary,
                    TextStyle::Caption);
        const uint16_t held = heldAt(i);
        if (held != 0) {
            const std::string text = "x" + std::to_string(held);
            int w = 0, h = 0;
            fb.measureText(text, w, h, TextStyle::Caption);
            fb.drawText(cell.right() - kRule.cellPadX - w, textY, text, Colors::Accent, TextStyle::Caption);
        }
    }
    fb.clearClip();
}

} // namespace UI::SharedItemGridPicker
