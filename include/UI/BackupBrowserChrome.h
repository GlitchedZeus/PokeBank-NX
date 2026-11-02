#pragma once
#include "UI/ScreenChrome.h"
#include "UI/SharedBoxedRows.h"

namespace UI::BackupBrowserChrome {
inline std::string fit(PKSEFramebuffer& fb,std::string text,int width,TextStyle style) {
    int w=0,h=0;fb.measureText(text,w,h,style);
    if(w<=width)return text;
    do {
        if(text.empty())return {};
        size_t end=text.size()-1;
        while(end>0 && (static_cast<unsigned char>(text[end])&0xc0)==0x80)--end;
        text.resize(end);
        fb.measureText(text+"...",w,h,style);
    } while(w>width);
    return text+"...";
}
// Round 11 H12: `marked` (default false) draws the shared multi-select style: SharedBoxedRows'
// marked box, the text moved clear of its left bar, and the badge in Colors::Success. Callers
// that never pass it (Switch-save backups, emulator Backups) draw exactly as before.
inline void drawRow(PKSEFramebuffer& fb,int x,int y,int w,int h,const std::string& title,
    const std::string& detail,const std::string& badge,bool selected,bool marked=false) {
    SharedBoxedRows::drawBox(fb,SharedBoxedRows::rect(x,y,w,h),{selected,true,marked});
    const int tx=x+(marked?16+SharedBoxedRows::kMarkBarWidth+8:16);
    int bw=0,bh=0;fb.measureText(badge,bw,bh,TextStyle::Caption);
    fb.drawText(tx,y+2,fit(fb,title,w-bw-36-(tx-x),TextStyle::Body),Colors::Text,TextStyle::Body);
    fb.drawText(tx,y+27,fit(fb,detail,w-16-(tx-x),TextStyle::Caption),Colors::TextDim,TextStyle::Caption);
    fb.drawText(x+w-16-bw,y+8,badge,marked?Colors::Success:Colors::TextSecondary,TextStyle::Caption);
}
// Round 11 H12: a section label inside a boxed list (Recovery files' save headings). It is drawn
// unboxed, with a divider under it, so it never looks like a choice; callers publish no touch
// target for it.
inline void drawHeading(PKSEFramebuffer& fb,int x,int y,int w,int h,const std::string& title,
    const std::string& detail) {
    fb.drawFilledRoundedRect(x+4,y+h-2,w-8,2,1,Colors::Divider);
    const std::string shown=fit(fb,title,w/2,TextStyle::Body);
    int tw=0,th=0;fb.measureText(shown,tw,th,TextStyle::Body);
    fb.drawText(x+4,y+8,shown,Colors::Info,TextStyle::Body);
    fb.drawText(x+4+tw+16,y+12,fit(fb,detail,w-tw-28,TextStyle::Caption),Colors::TextMuted,TextStyle::Caption);
}
}
