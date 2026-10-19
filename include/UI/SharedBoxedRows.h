#pragma once

// SharedBoxedRows.h - R2-BUG-012: the ONE boxed-row style of every interactive Gen I-V modal.
//
// Owner reference: the Pokemon Actions popup (ScreenChrome drawNarrowPokemonActions). Every choice
// is its own dark rounded row; the active row carries a clear yellow outline; a held fingertip
// tints the row it will select. Explanatory text, status and help lines stay unboxed.
//
// Geometry comes from UI/SharedMenuGeometry.h: the box IS the cell rectangle, and the same
// rectangle is published as the touch target, so what is outlined is exactly what a tap selects
// and a tap can never land on a neighbouring row. Lists and grids (Kind::Grid, e.g. the item
// picker) use the same box; only the content inside a box differs per menu type.
//
// Colours are the reference popup's: Colors::Surface rows, Colors::Divider 1 px border,
// Colors::PartyNumber (the established yellow) 2 px focus / pressed outline. The theme's
// dark-blue modal surface, fonts and artwork are unchanged.

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedMenuGeometry.h"
// The fingertip-pressed tint needs the device touch state (ScreenChrome touchPressedRect, which
// pulls in libnx). Host render tests have no touch input, so there a box is never "pressed".
#if __has_include(<switch.h>)
#include "UI/ScreenChrome.h"
#define POKEBANK_BOXED_ROWS_TOUCH 1
#endif

#include <string>

namespace UI::SharedBoxedRows {

namespace Geo = PokeBank::UIModel::SharedMenuGeometry;

// Pure style numbers (tests/test_shared_boxed_rows.cpp checks them against the reference popup).
inline constexpr int kRadius = 10;         // reference row corner radius
inline constexpr int kBorder = 1;          // unfocused border
inline constexpr int kFocusBorder = 2;     // focused / pressed outline
inline constexpr int kLabelInset = 18;     // reference label x inset inside the box
inline constexpr int kDetailInset = 16;    // right-aligned detail text inset
inline constexpr int kPressedAlpha = 112;  // reference pressed tint

struct State {
    bool focused = false;
    bool enabled = true;
};

inline Color labelColor(const State& s) {
    if (!s.enabled) return Color(Colors::TextDim.r, Colors::TextDim.g, Colors::TextDim.b, 105);
    return s.focused ? Colors::TextPrimary : Colors::TextSecondary;
}

// The box alone (for cells whose content the caller draws: sprites, columns, glyph labels).
// Returns true when a fingertip is currently pressing it.
inline bool pressedNow(const Geo::Rect& cell) noexcept {
#ifdef POKEBANK_BOXED_ROWS_TOUCH
    return touchPressedRect(cell.x, cell.y, cell.w, cell.h);
#else
    (void)cell;
    return false;
#endif
}

inline bool drawBox(PKSEFramebuffer& fb, const Geo::Rect& cell, const State& s) {
    const bool pressed = s.enabled && pressedNow(cell);
    Color tint = Colors::PartyNumber;
    tint.a = kPressedAlpha;
    fb.drawFilledRoundedRect(cell.x, cell.y, cell.w, cell.h, kRadius, pressed ? tint : Colors::Surface);
    const bool outline = s.enabled && (s.focused || pressed);
    fb.drawRoundedRect(cell.x, cell.y, cell.w, cell.h, kRadius,
                       outline ? Colors::PartyNumber : Colors::Divider,
                       outline ? kFocusBorder : kBorder);
    return pressed;
}

// Publish the box as its touch target (disabled rows never get one). touchButtons is the screen's
// TouchButton list ({id, x, y, w, h}).
template <class Buttons>
inline void publish(Buttons& touchButtons, int touchId, const Geo::Rect& cell, const State& s) {
    if (s.enabled) touchButtons.push_back({touchId, cell.x, cell.y, cell.w, cell.h});
}

// One labelled boxed row: label left (fitted), optional detail right-aligned in caption style.
template <class Buttons>
inline void drawRow(PKSEFramebuffer& fb, Buttons& touchButtons, int touchId, const Geo::Rect& cell,
                    const std::string& label, const State& s, const std::string& detail = {},
                    TextStyle labelStyle = TextStyle::Body) {
    drawBox(fb, cell, s);
    publish(touchButtons, touchId, cell, s);
    int dw = 0, dh = 0;
    if (!detail.empty()) {
        fb.measureText(detail, dw, dh, TextStyle::Caption);
        fb.drawText(cell.right() - kDetailInset - dw, cell.y + (cell.h - dh) / 2, detail,
                    s.focused ? Colors::Text : Colors::TextSecondary, TextStyle::Caption);
    }
    int lw = 0, lh = 0;
    fb.measureText(label, lw, lh, labelStyle);
    fb.drawText(cell.x + kLabelInset, cell.y + (cell.h - lh) / 2, label, labelColor(s), labelStyle);
}

// Same, for a row whose geometry the caller already owns (an existing accepted list), as a Rect.
inline Geo::Rect rect(int x, int y, int w, int h) noexcept { return Geo::Rect{x, y, w, h}; }

} // namespace UI::SharedBoxedRows
