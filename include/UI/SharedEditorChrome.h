#pragma once

// Drawing side of UI/SharedEditorPresentation.h: the shared picker/modal footer, the
// "Origin & Legality" heading. Footers go through the app's
// existing ScreenChrome::drawNavBar, so A is the green face badge and B the red one
// (ScreenChrome buttonGlyph) everywhere; nothing here introduces a new glyph style.

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedEditorPresentation.h"
#include "UI/SharedFieldState.h"
#include "UI/SharedMenuGeometry.h"

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

inline Color fieldColor(FieldState::Token token) {
    switch (token) {
        case FieldState::Token::Text: return Colors::Text;
        case FieldState::Token::TextDim: return Colors::TextDim;
        case FieldState::Token::TextMuted: return Colors::TextMuted;
        case FieldState::Token::Gold: return Colors::ShinyStar;
        case FieldState::Token::Selected: return Colors::SelectedText;
        case FieldState::Token::FocusBorder: return Colors::FocusBorder;
    }
    return Colors::Text;
}

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
template <class Buttons>
inline void drawMenuCell(PKSEFramebuffer& fb, Buttons& touchButtons, int touchId,
                         const MenuGeometry::Rect& cell, bool focused) {
    if (focused) fb.drawSelectionHighlight(cell.x, cell.y, cell.w, cell.h);
    touchButtons.push_back({touchId, cell.x, cell.y, cell.w, cell.h});
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

} // namespace UI::SharedEditorChrome
