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
#include "UI/SharedFieldStateColors.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SharedReviewPresentation.h"

#include <string>
#include <vector>
#include <algorithm>
#include <utility>

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
        case Presentation::LegalityState::ProblemsFound:
        case Presentation::LegalityState::Invalid: return Colors::Error;
        case Presentation::LegalityState::Incomplete:
        case Presentation::LegalityState::NotChecked: return Colors::Warning;
        default: return Colors::TextDim;
    }
}

template<class Buttons>
inline void drawReviewPanel(PKSEFramebuffer& fb,Buttons& buttons,
    const PokeBank::UIModel::SharedReview::Rows& rows,size_t page,bool changed,
    const std::string& context) {
    namespace R=PokeBank::UIModel::SharedReview;
    beginModal(buttons);page=R::clamp(page,rows.size());
    const int w=1100,h=584,x=(fb.getWidth()-w)/2,y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRect(0,0,fb.getWidth(),fb.getHeight()-kNavBarH,Colors::Background);
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+24,y+18,"Review Pending Changes",Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+56,context+" / original source remains unchanged",Colors::TextDim,TextStyle::Caption);
    fb.drawText(x+24,y+90,std::to_string(rows.size())+" changes / Page "+std::to_string(page+1)+
        " of "+std::to_string(R::pages(rows.size())),Colors::TextDim,TextStyle::Caption);
    if(rows.empty())fb.drawText(x+32,y+145,changed?"Staged changes could not be verified":"No staged changes",changed?Colors::Warning:Colors::TextDim);
    for(size_t row=0;row<R::kPageSize;++row) {
        const size_t i=page*R::kPageSize+row;if(i>=rows.size())break;
        const int yy=y+128+static_cast<int>(row)*102;
        fb.drawFilledRoundedRect(x+18,yy,w-36,94,10,Colors::PanelAlt);
        fb.drawText(x+32,yy+12,rows[i].first,Colors::Text,TextStyle::Body);
        fb.drawText(x+32,yy+49,rows[i].second,Colors::TextSecondary,TextStyle::Caption);
    }
    drawFooter(fb,R::footer(changed,rows.size()));
}

template<class Buttons>
inline void drawReviewDiscardConfirm(PKSEFramebuffer& fb,Buttons& buttons) {
    beginModal(buttons);
    const int w=900,h=250,x=(fb.getWidth()-w)/2,y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRect(0,0,fb.getWidth(),fb.getHeight()-kNavBarH,Color(0,0,0,145));
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawText(x+28,y+24,"Discard all staged changes?",Colors::Text,TextStyle::Heading);
    fb.drawText(x+28,y+82,"Export Copy first to keep them. Original saves remain unchanged.",Colors::TextDim,TextStyle::Caption);
    SharedBoxedRows::drawGlyphChoices(fb,x+28,y+145,w-56,
        std::array<SharedBoxedRows::GlyphChoice,2>{{{"B","Keep Changes"},{"Y","Discard All"}}});
    drawNavBar(fb,{{"Y","Discard All"},{"B","Keep Changes"}});
}

// The accepted four-choice staged exit, including identical touch IDs and glyphs
// for every generation. Generation adapters supply actions, never source writers.
template<class Buttons>
inline void drawStagedExitPanel(PKSEFramebuffer& fb, Buttons& buttons) {
    beginModal(buttons);
    constexpr int w=840,h=330,pad=28,gap=16,bh=60,bw=(w-2*pad-gap)/2;
    const int x=(fb.getWidth()-w)/2,y=(fb.getHeight()-h)/2;
    fb.drawFilledRect(0,0,fb.getWidth(),fb.getHeight()-kNavBarH,Color(0,0,0,145));
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawText(x+pad,y+24,"Unsaved Changes",Colors::Text,TextStyle::Heading);
    fb.drawText(x+pad,y+76,"Your edits are staged. The original save stays unchanged.",Colors::Text);
    fb.drawText(x+pad,y+108,"Review them, export a separate copy, or continue editing.",Colors::TextDim);
    const int row=y+h-2*bh-gap-20;
    const auto choice=[&](int bx,int by,const char* glyph,const char* label,int id) {
        drawGlyphButton(fb,bx,by,bw,bh,glyph,label);
        buttons.push_back({id,bx,by,bw,bh});
    };
    choice(x+pad,row,"A","Review Changes",2);
    choice(x+pad+bw+gap,row,"B","Continue Editing",0);
    choice(x+pad,row+bh+gap,"X","Export Copy",3);
    choice(x+pad+bw+gap,row+bh+gap,"Y","Discard & Exit",1);
    drawNavBar(fb,{{"A","Review Changes"},{"B","Continue Editing"},
                  {"Y","Discard & Exit"},{"X","Export Copy"}});
}

// Generation adapters supply only stored facts. One layout and one verdict
// vocabulary serve every Origin & Legality modal; structure validity never
// implies that an encounter or a complete legality check passed.
template<class Buttons>
inline void drawOriginLegalityPanel(PKSEFramebuffer& fb,Buttons& touchButtons,
    const std::vector<std::pair<std::string,std::string>>& rows,
    Presentation::LegalityState verdict) {
    beginModal(touchButtons);
    const int w=std::min(1100,fb.getWidth()-48);
    const int columns=rows.size()>10?2:1;
    const int perColumn=std::max(1,static_cast<int>((rows.size()+columns-1)/columns));
    const int available=fb.getHeight()-kNavBarH-24;
    const int rowH=std::min(46,(available-134)/perColumn);
    const int h=134+perColumn*rowH,x=(fb.getWidth()-w)/2;
    const int y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRect(0,0,fb.getWidth(),fb.getHeight()-kNavBarH,Color(0,0,0,145));
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Divider,1);
    fb.drawText(x+24,y+16,Presentation::kOriginLegalityHeading,Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+48,"Stored origin and format checks; full legality analysis has not run",
                Colors::TextDim,TextStyle::Caption);
    const int cellW=(w-48)/columns;
    const auto fit=[&](std::string value,int maxW) {
        int tw=0,th=0;fb.measureText(value,tw,th,TextStyle::Caption);
        if(tw<=maxW)return value;
        while(!value.empty()) {
            do {const auto c=static_cast<unsigned char>(value.back());value.pop_back();
                if((c&0xC0)!=0x80)break;} while(!value.empty());
            fb.measureText(value+"...",tw,th,TextStyle::Caption);
            if(tw<=maxW)return value+"...";
        }
        return std::string();
    };
    for(size_t i=0;i<rows.size();++i) {
        const int col=static_cast<int>(i)/perColumn,row=static_cast<int>(i)%perColumn;
        const int xx=x+24+col*cellW,yy=y+78+row*rowH;
        fb.drawText(xx,yy,fit(rows[i].first,cellW-24),Colors::TextDim,TextStyle::Caption);
        fb.drawText(xx,yy+20,fit(rows[i].second,cellW-24),Colors::Text,TextStyle::Caption);
    }
    fb.drawHDivider(x+24,y+h-48,w-48);
    fb.drawText(x+24,y+h-36,"Final verdict",Colors::TextDim,TextStyle::Caption);
    fb.drawText(x+210,y+h-36,Presentation::legalityLabel(verdict),legalityColor(verdict),TextStyle::Caption);
    drawNavBar(fb,{{"B","Back"}});
}

} // namespace UI::SharedEditorChrome
