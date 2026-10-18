#pragma once

// Geometry of the emulator "Backups / Emulator save" list (Product Home, game workspace,
// non-Switch titles). The frame, rows, focus highlight and touch targets all come from
// SharedMenuGeometry's BackupMenu rule (the accepted Save Backups card: 1200 px, 62 px rows,
// 8 visible), so this list and Save Backups (BackupSelectionScreen) share one contract. This header only places the text inside those rectangles:
//   * header: the game heading and one explanatory caption, inside rule.headerH;
//   * row: label (Body) over detail (Caption), a right-aligned tag column, all inside the cell;
//   * notice: an action's reason (A Details / a refused Y Delete) as a toast centred above the
//     screen footer, outside the frame, sized to its text.
// Pure and host-tested (tests/test_emulator_backup_layout.cpp).

#include "UI/SharedMenuGeometry.h"

#include <string_view>

namespace PokeBank::UIModel::EmulatorBackupLayout {

namespace G = SharedMenuGeometry;

inline constexpr std::string_view kExplainer =
    "The emulator's save is read only; only PokeBank NX export records can be deleted.";

inline constexpr int kHeadingTop = 4;     // from frame.y (Heading, 28 px)
inline constexpr int kExplainerTop = 40;  // from frame.y (Caption, 17 px)
inline constexpr int kLabelTop = 2;       // from cell.y (Body, 20 px)
inline constexpr int kDetailTop = 27;     // from cell.y (Caption, 17 px)
inline constexpr int kTagColumnW = 112;   // "READ ONLY" / "APP-OWNED" / "IN EDITOR"
inline constexpr int kNoticePadX = 18;
inline constexpr int kNoticeGap = 8;      // toast bottom to the footer band (clears a full 8-row frame)

// Rendered line height of a text style (PKSEFramebuffer draws ~1.25 em from the text top).
constexpr int lineH(G::Text style) noexcept { return G::fontPx(style) * 5 / 4; }

[[nodiscard]] constexpr G::Layout layout(int rows, int screenW = G::kScreenW,
                                         int screenH = G::kScreenH) noexcept {
    return G::layout(G::MenuType::BackupMenu, rows, screenW, screenH);
}

// First row shown for a scroll position, clamped to the rule's viewport.
[[nodiscard]] constexpr int clampScroll(const G::Layout& l, int scroll) noexcept {
    const int maxFirst = l.totalRows > l.visibleRows ? l.totalRows - l.visibleRows : 0;
    return scroll < 0 ? 0 : scroll > maxFirst ? maxFirst : scroll;
}

// Scroll that keeps `index` visible after controller navigation.
[[nodiscard]] constexpr int revealRow(const G::Layout& l, int scroll, int index) noexcept {
    if (index < scroll) scroll = index;
    if (index >= scroll + l.visibleRows) scroll = index - l.visibleRows + 1;
    return clampScroll(l, scroll);
}

// Label column of a row (the tag column is trailing).
[[nodiscard]] constexpr G::Rect labelRect(const G::Layout& l, const G::Rect& cell) noexcept {
    return G::labelRect(l, cell, kTagColumnW);
}

// The notice toast for text of drawn width `textW`, centred above the footer band.
[[nodiscard]] constexpr G::Rect noticeRect(int textW, int screenW = G::kScreenW,
                                           int screenH = G::kScreenH) noexcept {
    const int w = textW + 2 * kNoticePadX;
    const int h = lineH(G::Text::Caption) + 14;
    return {(screenW - w) / 2, screenH - G::kFooterH - kNoticeGap - h, w, h};
}

} // namespace PokeBank::UIModel::EmulatorBackupLayout
