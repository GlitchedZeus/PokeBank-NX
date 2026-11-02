#pragma once

#include "UI/ClassicTrainerPage.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"

#include <string>

// Round 7 (owner Round 6 B06 / C06): the ONE Trainer painter. Gen V's Trainer page
// (Gen5SharedReadOnlySurface drawTrainerEditor) and the Gen I-IV Trainer page
// (TrainerViewScreenBase drawClassicTrainerView) both paint through these functions, so the panel,
// header bar, editable cards with value pills, divider and read-only rows share one set of style
// tokens: geometry from UI/ClassicTrainerPage.h (the Gen V metrics), colours from the colour roles
// below. A read-only value is neutral text (Colors::Text) exactly as on Gen V, never a highlight
// colour; every row card has the same PanelAlt background.
namespace UI::SharedTrainerPanel {

namespace Page = PokeBank::UIModel::ClassicTrainerPage;

// Colour roles (the tests compare these between generations).
inline Color labelColor(bool selected) noexcept { return selected ? Colors::SelectedText : Colors::TextDim; }
inline Color valueColor() noexcept { return Colors::Text; }
inline Color cardColor() noexcept { return Colors::PanelAlt; }
inline Color pillColor() noexcept { return Colors::Background; }

// The panel and its header bar ("Trainer" + one dim caption line).
inline void drawPanel(PKSEFramebuffer& fb, int x, int y, int w, int h, const std::string& caption) {
    fb.drawFilledRoundedRect(x, y, w, h, 16, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 16, Colors::Border, 1);
    fb.drawFilledRoundedRect(x, y, w, Page::kHeaderH, 16, Colors::AccentDim);
    fb.drawFilledRect(x, y + Page::kHeaderH - 16, w, 16, Colors::AccentDim);
    fb.drawText(x + 22, y + (Page::kHeaderH - fb.lineHeight(TextStyle::Heading)) / 2, "Trainer", Colors::Text,
                TextStyle::Heading);
    fb.drawText(x + 170, y + (Page::kHeaderH - fb.lineHeight(TextStyle::Caption)) / 2, caption, Colors::TextDim,
                TextStyle::Caption);
}

// A selectable row: shadowed card, focus outline when selected, label left, value in a pill right.
inline void drawEditableRow(PKSEFramebuffer& fb, int x, int y, int w, int h, const std::string& label,
                            const std::string& value, bool selected) {
    fb.drawSoftShadow(x, y, w, h, 14);
    fb.drawFilledRoundedRect(x, y, w, h, 14, cardColor());
    if (selected) fb.drawRoundedRect(x, y, w, h, 14, Colors::FocusBorder, 2);
    fb.drawText(x + Page::kTextInset, y + (h - fb.lineHeight(TextStyle::Body)) / 2, label, labelColor(selected),
                TextStyle::Body);
    int vw = 0, vh = 0;
    fb.measureText(value, vw, vh, TextStyle::Body);
    const int pillW = vw + Page::kPillPad, pillH = Page::kPillH;
    const int px = x + w - pillW - Page::kPillInset, py = y + (h - pillH) / 2;
    fb.drawPill(px, py, pillW, pillH, pillColor());
    fb.drawText(px + (pillW - vw) / 2, py + (pillH - vh) / 2, value, valueColor(), TextStyle::Body);
}

// The divider between the selectable rows and the read-only rows.
inline void drawDivider(PKSEFramebuffer& fb, int cardX, int y, int cardW) { fb.drawHDivider(cardX + 20, y, cardW - 40); }

// A read-only row: flat card, label left, neutral value right-aligned.
inline void drawInfoRow(PKSEFramebuffer& fb, int x, int y, int w, int h, const std::string& label,
                        const std::string& value) {
    fb.drawFilledRoundedRect(x, y, w, h, 12, cardColor());
    fb.drawText(x + Page::kTextInset, y + (h - fb.lineHeight(TextStyle::Body)) / 2, label, labelColor(false),
                TextStyle::Body);
    int vw = 0, vh = 0;
    fb.measureText(value, vw, vh, TextStyle::Body);
    fb.drawText(x + w - Page::kTextInset - vw, y + (h - vh) / 2, value, valueColor(), TextStyle::Body);
}

} // namespace UI::SharedTrainerPanel
