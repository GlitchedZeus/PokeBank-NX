#pragma once

// Drawing side of UI/SharedEditorPresentation.h: the shared picker/modal footer, the
// "Origin & Legality" heading. Footers go through the app's
// existing ScreenChrome::drawNavBar, so A is the green face badge and B the red one
// (ScreenChrome buttonGlyph) everywhere; nothing here introduces a new glyph style.

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedEditorPresentation.h"
#include "UI/SharedOriginLegality.h"
#include "UI/SharedFieldState.h"
#include "UI/SharedFieldStateColors.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedBoxedRows.h"

#include <string>
#include <vector>

namespace UI::SharedEditorChrome {

namespace Presentation = PokeBank::UIModel::SharedEditorPresentation;

inline std::vector<ControllerHint> hints(const Presentation::Footer& footer) {
    std::vector<ControllerHint> result;
    for (const auto& hint : footer) result.push_back({hint.glyph, hint.label});
    return result;
}

inline void drawFooter(PKSEFramebuffer& fb, const Presentation::Footer& footer) {
    drawNavBar(fb, hints(footer));
}

inline void drawPickerFooter(PKSEFramebuffer& fb, const Presentation::PickerFooterSpec& spec = {}) {
    drawFooter(fb, Presentation::pickerFooter(spec));
}

// A modal that covers the workspace owns the frame's touch: drop the covered layer's footer
// badges and glyph buttons (ScreenChrome::beginTopTouchLayer, the P0 #320 rule), clear the
// screen's own row hitboxes, then let the modal publish only what it visibly offers.
// Same pattern as Gen2WorkspaceLayers (ae099c77): a footer under a modal is never tappable.
template <class Buttons>
inline void beginModal(Buttons& screenTouchButtons) {
    beginTopTouchLayer();
    screenTouchButtons.clear();
}

// ---- Field state (UI/SharedFieldState.h) -------------------------------------------------------

namespace FieldState = PokeBank::UIModel::SharedFieldState;

inline Color fieldColor(FieldState::Token token) { return SharedFieldStateColors::color(token); }

// The accepted focus outline (2 px FocusBorder, radius 6), drawn for a focused field of ANY state.
inline void drawFieldFocus(PKSEFramebuffer& fb, int x, int y, int w, int h,
                           const FieldState::Style& style) {
    if (style.focusOutline) fb.drawRoundedRect(x, y, w, h, 6, fieldColor(style.outline), 2);
}

// One label/value field row: focus outline, then label and value in their state colours.
inline void drawField(PKSEFramebuffer& fb, int x, int y, int w, int h, int valueX,
                      const std::string& label, const std::string& value,
                      FieldState::State state, bool focused,
                      TextStyle textStyle = TextStyle::Caption) {
    const auto style = FieldState::style(state, focused);
    drawFieldFocus(fb, x, y, w, h, style);
    const int textY = y + (h - fb.lineHeight(textStyle)) / 2;
    fb.drawText(x + 10, textY, label, fieldColor(style.label), textStyle);
    fb.drawText(valueX, textY, value, fieldColor(style.value), textStyle);
}

// ---- Menu geometry (UI/SharedMenuGeometry.h) ----------------------------------------------------

namespace MenuGeometry = PokeBank::UIModel::SharedMenuGeometry;

// The geometry header works on the 1280x720 contract; this checks the framebuffer agrees.
inline bool menuScreenMatches(const PKSEFramebuffer& fb) {
    return fb.getWidth() == MenuGeometry::kScreenW && fb.getHeight() == MenuGeometry::kScreenH;
}
static_assert(MenuGeometry::kFooterH == kNavBarH, "menu geometry keeps the screen footer free");

// A cell of a shared menu: focus highlight on the SAME rectangle that is published as its touch
// target, so what is highlighted is exactly what a tap selects.
// R2-BUG-012: drawn in the ONE boxed-row style (UI/SharedBoxedRows.h): every cell is a dark
// rounded box, the focused one has the yellow outline, and the box is exactly the touch target.
template <class Buttons>
inline void drawMenuCell(PKSEFramebuffer& fb, Buttons& touchButtons, int touchId,
                         const MenuGeometry::Rect& cell, bool focused, bool enabled = true) {
    const SharedBoxedRows::State state{focused, enabled};
    SharedBoxedRows::drawBox(fb, cell, state);
    SharedBoxedRows::publish(touchButtons, touchId, cell, state);
}

// ---- Origin & Legality ------------------------------------------------------------------------

inline constexpr int kHeadingHeight = 26;

// One heading: accent caption text with a 1px divider under it. Returns the y below it.
inline int drawOriginLegalityHeading(PKSEFramebuffer& fb, int x, int y, int w) {
    fb.drawText(x, y, Presentation::kOriginLegalityHeading, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(x, y + 21, w, 1, Colors::Divider);
    return y + kHeadingHeight;
}

inline Color legalityColor(Presentation::LegalityState state) {
    switch (state) {
        case Presentation::LegalityState::Verified: return Colors::Success;
        case Presentation::LegalityState::ProblemsFound: return Colors::Error;
        case Presentation::LegalityState::Incomplete:
        case Presentation::LegalityState::NotChecked: return Colors::Warning;
        default: return Colors::TextDim;
    }
}

// ---------------------------------------------------------------------------------------------
// E03: the ONE Origin & Legality panel of every Gen I-V surface (box Actions, the Gen I-III Party
// popup). Content: UI/SharedOriginLegality.h's fixed rows from the generation's native record. The
// panel is read-only, owns the frame's touch layer and its only control is B (Back).
// ---------------------------------------------------------------------------------------------
namespace OriginLegalityModel = PokeBank::UIModel::OriginLegality;

inline Color originLegalityToneColor(OriginLegalityModel::Tone tone) {
    switch (tone) {
        case OriginLegalityModel::Tone::Good: return Colors::Success;
        case OriginLegalityModel::Tone::Caution: return Colors::Warning;
        case OriginLegalityModel::Tone::Locked: return Colors::Accent;
        case OriginLegalityModel::Tone::Dim: return Colors::TextDim;
        case OriginLegalityModel::Tone::Plain: break;
    }
    return Colors::Text;
}

inline constexpr int kOriginLegalityWidth = 860;
inline constexpr int kOriginLegalityHeaderH = 92;
inline constexpr int kOriginLegalityRowStep = 36;
inline constexpr int kOriginLegalityMoveRowStep = 30;
inline constexpr int kOriginLegalityBottomPad = 22;
inline constexpr int kOriginLegalityValueX = 290;

// Content-driven height: the fixed rows, plus a compact move block when the generation ran its
// move guardrail for this record.
constexpr int originLegalityPanelHeight(int moveRows) noexcept {
    return kOriginLegalityHeaderH + OriginLegalityModel::kFixedRows * kOriginLegalityRowStep + (moveRows > 0 ? 34 + moveRows * kOriginLegalityMoveRowStep : 0) + kOriginLegalityBottomPad;
}
static_assert(originLegalityPanelHeight(4) <= 720 - 56 - 16, "the panel fits above the footer with four move rows");

// `subject` names the Pokemon (species, or species + slot); the panel title is the shared heading.
template <class Buttons>
inline void drawOriginLegalityPanel(PKSEFramebuffer& fb, Buttons& touchButtons, const std::string& subject, const OriginLegalityModel::Report& report) {
    beginModal(touchButtons);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 120));
    const int moveRows = static_cast<int>(std::min<std::size_t>(report.moves.size(), 4));
    const int h = originLegalityPanelHeight(moveRows);
    const int x = (fb.getWidth() - kOriginLegalityWidth) / 2;
    const int y = std::max(16, (fb.getHeight() - kNavBarH - h) / 2);
    fb.drawFilledRoundedRect(x, y, kOriginLegalityWidth, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, kOriginLegalityWidth, h, 18, Colors::Divider, 1);
    fb.drawText(x + 24, y + 18, PokeBank::UIModel::SharedEditorPresentation::kOriginLegalityHeading,
                Colors::Text, TextStyle::Heading);
    if (!subject.empty()) {
        int sw = 0, sh = 0;
        fb.measureText(subject, sw, sh, TextStyle::Body);
        fb.drawText(x + kOriginLegalityWidth - 24 - sw, y + 22, subject, Colors::TextSecondary, TextStyle::Body);
    }
    fb.drawText(x + 24, y + 54, OriginLegalityModel::kSubtitle, Colors::TextDim, TextStyle::Caption);
    int rowY = y + kOriginLegalityHeaderH;
    for (const auto& row : OriginLegalityModel::rows(report)) {
        fb.drawText(x + 34, rowY, row.label, Colors::TextDim, TextStyle::Body);
        fb.drawText(x + kOriginLegalityValueX, rowY, row.value, originLegalityToneColor(row.tone), TextStyle::Body);
        rowY += kOriginLegalityRowStep;
    }
    if (moveRows > 0) {
        fb.drawHDivider(x + 24, rowY + 4, kOriginLegalityWidth - 48);
        fb.drawText(x + 34, rowY + 12, "Move guardrail", Colors::TextDim, TextStyle::Caption);
        rowY += 34;
        for (int i = 0; i < moveRows; ++i) {
            const auto& m = report.moves[static_cast<std::size_t>(i)];
            fb.drawText(x + 34, rowY, "Move " + std::to_string(i + 1), Colors::TextDim, TextStyle::Caption);
            fb.drawText(x + 150, rowY, m.move, Colors::Text, TextStyle::Caption);
            fb.drawText(x + kOriginLegalityValueX + 260, rowY, m.status, originLegalityToneColor(m.tone), TextStyle::Caption);
            rowY += kOriginLegalityMoveRowStep;
        }
    }
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::SharedEditorChrome
