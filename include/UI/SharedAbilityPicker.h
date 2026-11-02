#pragma once
#include "UI/AbilityEffects.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedMovesPanel.h"

namespace UI::SharedAbilityPicker {
namespace G=PokeBank::UIModel::SharedMenuGeometry;
// Keep the owner's compact Location width. Name and full short effect use
// separate lines so narrowing never takes space away from either text.
inline constexpr int kRowPitch=60;
inline constexpr G::Layout layout(int count) {
    auto out=G::layout(G::MenuType::LocationPicker,count);
    const int extra=out.visibleRows*(kRowPitch-out.rule.rowHeight);
    out.rule.rowHeight=kRowPitch;
    out.frame.h+=extra;
    out.frame.y=(G::kScreenH-G::kFooterH-out.frame.h)/2;
    out.list.y=out.frame.y+out.rule.headerH;
    out.list.h=out.visibleRows*kRowPitch;
    return out;
}
inline void drawRow(PKSEFramebuffer& fb,int x,int y,int width,int height,
    const std::string& label,bool selected) {
    const auto name=AbilityEffects::nameOf(label);
    const auto effect=label.size()>name.size()?label.substr(name.size()+AbilityEffects::kSplit.size()):std::string{};
    const int nameH=fb.lineHeight(TextStyle::Body),effectH=fb.lineHeight(TextStyle::Caption);
    const int top=y+std::max(0,(height-nameH-effectH-2)/2);
    fb.drawText(x+16,top,SharedMovesPanel::fit(fb,name,width-32,TextStyle::Body),Colors::Text,TextStyle::Body);
    fb.drawText(x+16,top+nameH+2,SharedMovesPanel::fit(fb,effect,width-32,TextStyle::Caption),
        AbilityEffects::isDrawbackEffect(effect)?Colors::Error:
            selected?Colors::Text:Colors::TextSecondary,TextStyle::Caption);
}
static_assert(layout(3).frame.w==620 && layout(3).frame.h==280);
static_assert(layout(8).frame.bottom()<=G::kFooterBand.y-G::kMargin);
}
