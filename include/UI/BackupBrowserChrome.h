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
inline void drawRow(PKSEFramebuffer& fb,int x,int y,int w,int h,const std::string& title,
    const std::string& detail,const std::string& badge,bool selected) {
    SharedBoxedRows::drawBox(fb,SharedBoxedRows::rect(x,y,w,h),{selected});
    int bw=0,bh=0;fb.measureText(badge,bw,bh,TextStyle::Caption);
    fb.drawText(x+16,y+2,fit(fb,title,w-bw-52,TextStyle::Body),Colors::Text,TextStyle::Body);
    fb.drawText(x+16,y+27,fit(fb,detail,w-32,TextStyle::Caption),Colors::TextDim,TextStyle::Caption);
    fb.drawText(x+w-16-bw,y+8,badge,Colors::TextSecondary,TextStyle::Caption);
}
}
