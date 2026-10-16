#pragma once

// Drawing side of UI/SharedEditorPresentation.h: the shared picker/modal footer, the
// "Origin & Legality" heading. Footers go through the app's
// existing ScreenChrome::drawNavBar, so A is the green face badge and B the red one
// (ScreenChrome buttonGlyph) everywhere; nothing here introduces a new glyph style.

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedEditorPresentation.h"

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
