#pragma once

// SharedOriginLegalityPanel.h - E03: the ONE Origin & Legality panel drawn by every Gen I-V
// surface (Gen I / II / III / IV / V Actions -> Origin & Legality, and the Gen I-III Party popup).
// The content is UI/SharedOriginLegality.h's fixed rows; this file only draws them. The panel is
// read-only: it owns the frame's touch layer and its only control is B (Back).

#include "UI/SharedEditorChrome.h"
#include "UI/SharedOriginLegality.h"

#include <algorithm>
#include <string>

namespace UI::SharedOriginLegalityPanel {

namespace Model = PokeBank::UIModel::OriginLegality;

inline Color toneColor(Model::Tone tone) {
    switch (tone) {
        case Model::Tone::Good: return Colors::Success;
        case Model::Tone::Caution: return Colors::Warning;
        case Model::Tone::Locked: return Colors::Accent;
        case Model::Tone::Dim: return Colors::TextDim;
        case Model::Tone::Plain: break;
    }
    return Colors::Text;
}

inline constexpr int kWidth = 860;
inline constexpr int kHeaderH = 92;
inline constexpr int kRowStep = 36;
inline constexpr int kMoveRowStep = 30;
inline constexpr int kBottomPad = 22;
inline constexpr int kValueX = 290;

// Content-driven height: the fixed rows, plus a compact move block when the generation ran its
// move guardrail for this record.
constexpr int height(int moveRows) noexcept {
    return kHeaderH + Model::kFixedRows * kRowStep + (moveRows > 0 ? 34 + moveRows * kMoveRowStep : 0) + kBottomPad;
}
static_assert(height(4) <= 720 - 56 - 16, "the panel fits above the footer with four move rows");

// `subject` names the Pokemon (species, or species + slot); the panel title is the shared heading.
template <class Buttons>
inline void draw(PKSEFramebuffer& fb, Buttons& touchButtons, const std::string& subject, const Model::Report& report) {
    SharedEditorChrome::beginModal(touchButtons);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 120));
    const int moveRows = static_cast<int>(std::min<std::size_t>(report.moves.size(), 4));
    const int h = height(moveRows);
    const int x = (fb.getWidth() - kWidth) / 2;
    const int y = std::max(16, (fb.getHeight() - kNavBarH - h) / 2);
    fb.drawFilledRoundedRect(x, y, kWidth, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, kWidth, h, 18, Colors::Divider, 1);
    fb.drawText(x + 24, y + 18, PokeBank::UIModel::SharedEditorPresentation::kOriginLegalityHeading,
                Colors::Text, TextStyle::Heading);
    if (!subject.empty()) {
        int sw = 0, sh = 0;
        fb.measureText(subject, sw, sh, TextStyle::Body);
        fb.drawText(x + kWidth - 24 - sw, y + 22, subject, Colors::TextSecondary, TextStyle::Body);
    }
    fb.drawText(x + 24, y + 54, Model::kSubtitle, Colors::TextDim, TextStyle::Caption);
    int rowY = y + kHeaderH;
    for (const auto& row : Model::rows(report)) {
        fb.drawText(x + 34, rowY, row.label, Colors::TextDim, TextStyle::Body);
        fb.drawText(x + kValueX, rowY, row.value, toneColor(row.tone), TextStyle::Body);
        rowY += kRowStep;
    }
    if (moveRows > 0) {
        fb.drawHDivider(x + 24, rowY + 4, kWidth - 48);
        fb.drawText(x + 34, rowY + 12, "Move guardrail", Colors::TextDim, TextStyle::Caption);
        rowY += 34;
        for (int i = 0; i < moveRows; ++i) {
            const auto& m = report.moves[static_cast<std::size_t>(i)];
            fb.drawText(x + 34, rowY, "Move " + std::to_string(i + 1), Colors::TextDim, TextStyle::Caption);
            fb.drawText(x + 150, rowY, m.move, Colors::Text, TextStyle::Caption);
            fb.drawText(x + kValueX + 260, rowY, m.status, toneColor(m.tone), TextStyle::Caption);
            rowY += kMoveRowStep;
        }
    }
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::SharedOriginLegalityPanel
