#pragma once

// SharedMenuGeometry.h - one geometry contract per KIND of menu, for every Gen I-V editor.
//
// The same kind of menu looks and hit-tests the same way in every generation; a call site picks
// a MenuType and draws inside the rectangles this header returns. Pure constexpr (no framebuffer),
// so the host tests can prove the layout before it reaches hardware:
//   * the frame is centred in the area above the screen footer and never overlaps it;
//   * height is content-driven: min(rows, maxVisibleRows) * rowHeight + chrome, so a 3-row menu is
//     not drawn as a 10-row box;
//   * one cell rectangle per row/cell is BOTH the focus highlight and the touch target
//     (cellRect); nothing computes them separately;
//   * a leading sprite sits a fixed spriteGap from its label, so there are no wide empty gaps;
//   * text is measured with the bundled font's real advances (NunitoMetrics.h) and must fit its
//     column (fitsWidth) instead of being clipped.
//
// Widths are compact: each list is only as wide as its longest real native label plus its
// trailing column (tests/test_shared_menu_geometry.cpp measures every Gen I-V name), so a short
// name never sits across a wide empty band from its sprite or its value.
// Compact, not tiny: list rows are 44 px (the accepted species/move chooser pitch, 43-44), short
// command menus keep 52 px rows, the reference Held Item grid keeps its accepted 40 px pitch.
// Captions use the 17 px style, labels the 20 px style (PKSEFramebuffer kFontSizes).

#include "UI/NunitoMetrics.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace PokeBank::UIModel::SharedMenuGeometry {

inline constexpr int kScreenW = 1280;
inline constexpr int kScreenH = 720;
inline constexpr int kFooterH = 56;     // ScreenChrome kNavBarH; the footer row is never covered
inline constexpr int kMargin = 16;      // minimum gap between a frame and the screen / footer edge

// Font sizes of PKSEFramebuffer's TextStyle::Caption / Body / Heading (kFontSizes).
enum class Text : uint8_t { Caption, Body, Heading };
constexpr int fontPx(Text t) noexcept { return t == Text::Caption ? 17 : t == Text::Body ? 20 : 28; }

// Upper bound of the drawn width of UTF-8 text in the app font (kerning only narrows a run).
constexpr int textWidth(std::string_view utf8, Text style) noexcept {
    namespace M = NunitoMetrics;
    long units = 0;
    for (std::size_t i = 0; i < utf8.size();) {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        if (c >= 0x20 && c < 0x7F) { units += M::kAsciiAdvance[c - 0x20]; ++i; continue; }
        // One non-ASCII code point (accented letter, ♂/♀/★ fallback glyph): widest advance.
        units += M::kWidestAdvance;
        i += c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
    }
    const long px = units * fontPx(style);
    return static_cast<int>((px + M::kUnitsPerEm - 1) / M::kUnitsPerEm);
}
constexpr bool fitsWidth(std::string_view utf8, Text style, int width) noexcept {
    return textWidth(utf8, style) <= width;
}

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    constexpr int right() const noexcept { return x + w; }
    constexpr int bottom() const noexcept { return y + h; }
    constexpr bool contains(int px, int py) const noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    constexpr bool inside(const Rect& o) const noexcept {
        return x >= o.x && y >= o.y && right() <= o.right() && bottom() <= o.bottom();
    }
    constexpr bool overlaps(const Rect& o) const noexcept {
        return x < o.right() && o.x < right() && y < o.bottom() && o.y < bottom();
    }
    constexpr bool operator==(const Rect&) const noexcept = default;
};

// The screen footer band every menu leaves free (its glyphs stay visible above any modal).
inline constexpr Rect kFooterBand{0, kScreenH - kFooterH, kScreenW, kFooterH};

enum class MenuType : uint8_t {
    ItemPicker,       // item catalogs (Bag Add Item...): the compact 4-column item grid
    KeyItemWarning,   // "This is a Key Item" notice before staging one
    RemoveConfirm,    // Remove item / Release Pokemon: Y commits (frozen input rule)
    HeldItemPicker,   // Held Item: an alias of ItemPicker (the accepted Gen III grid)
    BallPicker,       // Poke Ball grid
    SpeciesPicker,    // species list (Lane P owns its search; geometry is shared)
    MovePicker,       // Move chooser: No. / name / type / category / Pwr / Acc / PP
    LocationPicker,   // Met Location: name left, method / level right
    BoxMenu,          // short command menu over the box grid
    BackupMenu,       // backup list (Home)
    LaunchConfirm,    // launch the game: Y commits
    SaveDiscard,      // save / discard staged work
    ChoicePicker,     // short editor value lists: Language, Ability, Form, Gender, Pokerus
    Count
};

enum class Kind : uint8_t { List, Grid, Confirm };

struct Rule {
    Kind kind = Kind::List;
    int width = 0;            // frame width
    int columns = 1;          // grid columns (1 for lists)
    int rowHeight = 44;       // row pitch; the cell rectangle is rowHeight - rowGap tall
    int rowGap = 4;           // vertical gap between cells (keeps focus outlines apart)
    int maxVisibleRows = 9;   // rows shown before the list scrolls
    int headerH = 64;         // title + divider (+ page/search line where the type has one)
    int padX = 20;            // frame edge to cell edge
    int padBottom = 16;       // last cell to frame bottom (or to the button row)
    int cellPadX = 12;        // cell edge to sprite / text
    int spriteSize = 0;       // leading sprite (0 = none)
    int spriteGap = 8;        // sprite to label
    Text label = Text::Body;  // label style
    int buttonRowH = 0;       // in-frame glyph buttons (Confirm kinds); 0 = screen footer only
    int bodyLines = 0;        // Confirm kinds: message lines under the title
};

constexpr Rule rule(MenuType type) noexcept {
    switch (type) {
        case MenuType::ItemPicker:
            // R2-BUG-006 / CG13: the compact item GRID (the accepted Gen III Held Item grid),
            // content-driven row count, fitted to 1280x720 above the footer.
            return {Kind::Grid, 1040, 4, 40, 4, 10, 90, 22, 16, 8, 30, 6, Text::Caption};
        case MenuType::HeldItemPicker:
            return rule(MenuType::ItemPicker);  // alias, not a copy (coordinator CG13)
        case MenuType::BallPicker:
            // One column, ball sprite leading its name (was a 640 px row with the sprite at the far
            // right edge). Same rows as ChoicePicker so every editor value list reads alike.
            return {Kind::List, 440, 1, 42, 4, 8, 86, 22, 39, 16, 32, 8, Text::Body};
        case MenuType::SpeciesPicker:
            return {Kind::List, 600, 1, 44, 4, 9, 84, 20, 16, 12, 34, 8, Text::Body};
        case MenuType::MovePicker:
            // The accepted Move chooser (M01-M08 PASS): 760 px, 86 px title + captions,
            // 10 rows of 42 px (38 px highlight), 14 px bottom margin -> 520 px when full.
            return {Kind::List, 760, 1, 42, 4, 10, 86, 24, 14, 16, 0, 8, Text::Body};
        case MenuType::LocationPicker:
            // The accepted Met Location window (owner round 1 L03/L04 PASS): 900 px, name left,
            // method / level right, editor value-list rows. NEW-10 (Round 3 L03/L04 notes): 14 px
            // under the last row instead of 39, so the window carries no unused band.
            return {Kind::List, 900, 1, 42, 4, 8, 86, 22, 14, 16, 0, 8, Text::Body};
        case MenuType::ChoicePicker:
            // Editor value lists: 86 px title + counter, 42 px rows (38 px cells), 8 visible.
            return {Kind::List, 440, 1, 42, 4, 8, 86, 22, 39, 16, 0, 8, Text::Body};
        case MenuType::BoxMenu:
            // The narrow Pokemon actions sheet over the box grid (NarrowPokemonActionMenu.h):
            // context + title header, up to 11 commands (the sheets list 3-9), never scrolls.
            return {Kind::List, 440, 1, 46, 6, 11, 78, 10, 34, 18, 0, 8, Text::Body};
        case MenuType::BackupMenu:
            // The accepted Save Backups screen (BackupSelectionScreen.cpp): full-width 1200 px card,
            // 62 px header (CARD_Y + 62), 62 px rows (LIST_ROW_H) with 52 px tiles
            // (itemY + LIST_ROW_H - 10), tiles inset 14 px (CARD_X + 14), 8 visible (LIST_MAX_VISIBLE).
            return {Kind::List, 1200, 1, 62, 10, 8, 62, 14, 14, 16, 0, 8, Text::Body};
        case MenuType::KeyItemWarning:
            return {Kind::Confirm, 600, 1, 0, 0, 0, 60, 28, 18, 0, 0, 8, Text::Body, 56, 2};
        case MenuType::RemoveConfirm:
            return {Kind::Confirm, 560, 1, 0, 0, 0, 60, 28, 18, 0, 0, 8, Text::Body, 56, 2};
        case MenuType::LaunchConfirm:
            return {Kind::Confirm, 640, 1, 0, 0, 0, 60, 28, 18, 0, 0, 8, Text::Body, 56, 3};
        case MenuType::SaveDiscard:
            // The accepted Gen I-V exit / save dialog width (Dialogs/SaveConfirmDialog.cpp, 560).
            return {Kind::Confirm, 560, 1, 0, 0, 0, 60, 24, 16, 0, 0, 8, Text::Body, 56, 3};
        case MenuType::Count: break;
    }
    return {};
}

inline constexpr int kBodyLineH = 30;   // message line pitch in Confirm kinds

struct Layout {
    MenuType type = MenuType::Count;
    Rule rule{};
    Rect frame{};
    Rect list{};          // the row/cell viewport (empty for Confirm kinds)
    Rect buttons{};       // in-frame glyph-button row (empty when the kind has none)
    int itemCount = 0;
    int visibleRows = 0;  // rows the viewport shows
    int totalRows = 0;
    int cellWidth = 0;
    constexpr bool scrolls() const noexcept { return totalRows > visibleRows; }
};

constexpr int ceilDiv(int a, int b) noexcept { return b <= 0 ? 0 : (a + b - 1) / b; }

// Content-driven layout of one menu holding itemCount entries (Confirm kinds ignore it).
constexpr Layout layout(MenuType type, int itemCount, int screenW = kScreenW,
                        int screenH = kScreenH) noexcept {
    Layout l;
    l.type = type;
    l.rule = rule(type);
    const Rule& r = l.rule;
    l.itemCount = itemCount < 0 ? 0 : itemCount;
    const int usableH = screenH - kFooterH - 2 * kMargin;
    int h = 0;
    if (r.kind == Kind::Confirm) {
        h = r.headerH + r.bodyLines * kBodyLineH + r.padBottom + r.buttonRowH + r.padBottom;
    } else {
        l.totalRows = ceilDiv(l.itemCount, r.columns);
        const int fit = (usableH - r.headerH - r.padBottom) / r.rowHeight;
        const int cap = r.maxVisibleRows < fit ? r.maxVisibleRows : fit;
        const int rows = l.totalRows < 1 ? 1 : l.totalRows;
        l.visibleRows = rows < cap ? rows : cap;
        h = r.headerH + l.visibleRows * r.rowHeight + r.padBottom;
    }
    const int w = r.width < screenW - 2 * kMargin ? r.width : screenW - 2 * kMargin;
    l.frame = {(screenW - w) / 2, (screenH - kFooterH - h) / 2, w, h};
    if (r.kind == Kind::Confirm) {
        l.buttons = {l.frame.x + r.padX, l.frame.bottom() - r.padBottom - r.buttonRowH,
                     w - 2 * r.padX, r.buttonRowH};
    } else {
        l.list = {l.frame.x + r.padX, l.frame.y + r.headerH, w - 2 * r.padX,
                  l.visibleRows * r.rowHeight};
        l.cellWidth = l.list.w / r.columns;
    }
    return l;
}

// The ONE rectangle of a visible cell: drawn as the focus highlight AND used as its touch target.
// visibleIndex counts from the first visible cell (row-major in a grid).
constexpr Rect cellRect(const Layout& l, int visibleIndex) noexcept {
    if (l.rule.kind == Kind::Confirm || visibleIndex < 0) return {};
    const int col = visibleIndex % l.rule.columns;
    const int row = visibleIndex / l.rule.columns;
    if (row >= l.visibleRows) return {};
    const int gapX = l.rule.columns > 1 ? 6 : 0;
    return {l.list.x + col * l.cellWidth, l.list.y + row * l.rule.rowHeight,
            l.cellWidth - gapX, l.rule.rowHeight - l.rule.rowGap};
}

// Which visible cell a touch lands in (-1 = none). Uses cellRect, so hit == highlight.
constexpr int hitCell(const Layout& l, int px, int py) noexcept {
    const int cells = l.visibleRows * l.rule.columns;
    for (int i = 0; i < cells; ++i)
        if (cellRect(l, i).contains(px, py)) return i;
    return -1;
}

// Inside a cell: the leading sprite box and the label column that follows it.
constexpr Rect spriteRect(const Layout& l, const Rect& cell) noexcept {
    if (l.rule.spriteSize <= 0) return {cell.x + l.rule.cellPadX, cell.y, 0, cell.h};
    return {cell.x + l.rule.cellPadX, cell.y + (cell.h - l.rule.spriteSize) / 2,
            l.rule.spriteSize, l.rule.spriteSize};
}
constexpr Rect labelRect(const Layout& l, const Rect& cell, int trailingWidth = 0) noexcept {
    const Rect s = spriteRect(l, cell);
    const int x = l.rule.spriteSize > 0 ? s.right() + l.rule.spriteGap : s.x;
    const int right = cell.right() - l.rule.cellPadX - (trailingWidth > 0 ? trailingWidth + 8 : 0);
    return {x, cell.y, right - x, cell.h};
}
// Baseline-free text top that centres a line of `style` text in a cell (PKSEFramebuffer draws
// text from its top; line height ~1.25 em in this app).
constexpr int textTop(const Rect& cell, Text style) noexcept {
    return cell.y + (cell.h - fontPx(style) * 5 / 4) / 2;
}

// Confirm kinds: n equally wide glyph buttons in the button row, in reading order.
constexpr Rect buttonRect(const Layout& l, int index, int count) noexcept {
    if (count <= 0 || index < 0 || index >= count || l.buttons.w <= 0) return {};
    constexpr int gap = 16;
    const int w = (l.buttons.w - gap * (count - 1)) / count;
    return {l.buttons.x + index * (w + gap), l.buttons.y, w, l.buttons.h};
}
// Confirm kinds: top of message line `line` (0-based).
constexpr int bodyLineTop(const Layout& l, int line) noexcept {
    return l.frame.y + l.rule.headerH + line * kBodyLineH;
}

// First visible row that keeps `selected` on screen, centred where possible (lists and grids).
constexpr int firstVisibleRow(const Layout& l, int selected) noexcept {
    if (l.totalRows <= l.visibleRows) return 0;
    const int row = (selected < 0 ? 0 : selected) / l.rule.columns;
    int first = row - l.visibleRows / 2;
    if (first > l.totalRows - l.visibleRows) first = l.totalRows - l.visibleRows;
    return first < 0 ? 0 : first;
}

// Footer never overlapped, frame on screen, every cell inside the viewport.
constexpr bool wellFormed(const Layout& l, int screenW = kScreenW, int screenH = kScreenH) noexcept {
    const Rect screen{0, 0, screenW, screenH - kFooterH};
    if (!l.frame.inside(screen) || l.frame.overlaps({0, screenH - kFooterH, screenW, kFooterH}))
        return false;
    if (l.rule.kind == Kind::Confirm) return l.buttons.inside(l.frame);
    if (!l.list.inside(l.frame)) return false;
    for (int i = 0; i < l.visibleRows * l.rule.columns; ++i)
        if (!cellRect(l, i).inside(l.list)) return false;
    return true;
}

namespace detail {
constexpr bool allWellFormed() noexcept {
    for (int t = 0; t < static_cast<int>(MenuType::Count); ++t)
        for (int n : {0, 1, 3, 9, 40, 500})
            if (!wellFormed(layout(static_cast<MenuType>(t), n))) return false;
    return true;
}
} // namespace detail
static_assert(detail::allWellFormed(), "every menu type fits above the footer at every size");
static_assert(layout(MenuType::BoxMenu, 3).frame.h < layout(MenuType::BoxMenu, 6).frame.h,
              "height is content-driven");
static_assert(rule(MenuType::BackupMenu).width == 1200 && rule(MenuType::BackupMenu).rowHeight == 62 &&
              rule(MenuType::BackupMenu).rowHeight - rule(MenuType::BackupMenu).rowGap == 52 &&
              rule(MenuType::BackupMenu).maxVisibleRows == 8 && layout(MenuType::BackupMenu, 50).visibleRows == 8,
              "BackupMenu records the accepted Save Backups list");
static_assert(layout(MenuType::HeldItemPicker, 400).cellWidth == 249,
              "the Held Item reference grid keeps its accepted 249 px cells");

} // namespace PokeBank::UIModel::SharedMenuGeometry
