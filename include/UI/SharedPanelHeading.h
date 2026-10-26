#ifndef POKEBANK_UI_SHARED_PANEL_HEADING_H
#define POKEBANK_UI_SHARED_PANEL_HEADING_H

// Round 7 P2 (owner Oct 26 typography): the ONE section-heading style of the Pokemon editor
// (DETAILS / STATS / MOVES / Origin & Legality) for every generation's View, Edit and Create. The
// shared Gen IV / V editor is the reference: a small Accent-coloured Caption label inset 14 / 10 px
// from the panel's top-left corner. Gen I-III draw their headings through this helper too, so the
// five generations cannot drift apart again.
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"

namespace PokeBank::UIModel::PanelHeading {

inline constexpr int kInsetX = 14;
inline constexpr int kInsetY = 10;
inline constexpr UI::TextStyle kStyle = UI::TextStyle::Caption;
[[nodiscard]] inline UI::Color color() noexcept { return UI::Colors::Accent; }

// Draws `title` as the heading of the panel whose top-left corner is (panelX, panelY).
inline void draw(UI::PKSEFramebuffer& fb, int panelX, int panelY, const char* title) {
    fb.drawText(panelX + kInsetX, panelY + kInsetY, title, color(), kStyle);
}

} // namespace PokeBank::UIModel::PanelHeading

#endif
