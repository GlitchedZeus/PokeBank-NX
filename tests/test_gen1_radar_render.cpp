#include "UI/StatsRadar.h"
#include <cassert>
#include <iostream>
#include <vector>

namespace {
struct Text { int x, y, w, h; std::string value; };
std::vector<Text> texts;
struct Rect { int x,y,w,h; };
std::vector<Rect> marks;
bool intersects(const Text& a, const Text& b) {
    return a.x < b.x+b.w && a.x+a.w > b.x && a.y < b.y+b.h && a.y+a.h > b.y;
}
}
// Recording framebuffer exercises the real production renderer without a GPU.
// Conservative caption metrics make the bounds check stricter than Nunito 15px.
namespace UI {
PKSEFramebuffer::PKSEFramebuffer() = default;
PKSEFramebuffer::~PKSEFramebuffer() = default;
void PKSEFramebuffer::measureText(const std::string& s, int& w, int& h, TextStyle) { w = s.size()*9; h = 18; }
void PKSEFramebuffer::drawText(int x,int y,const std::string& s,Color,TextStyle style) {
    int w,h; measureText(s,w,h,style); texts.push_back({x,y,w,h,s});
}
void PKSEFramebuffer::drawFilledRect(int x,int y,int w,int h,Color) { marks.push_back({x,y,w,h}); }
void PKSEFramebuffer::drawFilledCircle(int x,int y,int r,Color) { marks.push_back({x-r,y-r,2*r,2*r}); }
}
int main() {
    UI::PKSEFramebuffer fb;
    // Actual split-pane geometry, summary-sized region, and extreme valid stats.
    for (const auto& size : {std::pair{252,176}, std::pair{416,236}, std::pair{284,236}}) {
        for (const auto stats : {std::array<uint16_t,5>{19,10,10,10,12},
                                 std::array<uint16_t,5>{703,298,338,278,308}, std::array<uint16_t,5>{}}) {
            texts.clear(); marks.clear();
            UI::StatsRadar::drawGen1Labeled(fb, 10,20,size.first,size.second,stats);
            assert(texts.size() == 6 && !marks.empty());
            for (size_t i=0;i<texts.size();++i) {
                const auto& t=texts[i];
                assert(t.x>=10 && t.y>=20 && t.x+t.w<=10+size.first && t.y+t.h<=20+size.second);
                if (i<5) assert(t.value == std::string(PokeBank::UIModel::gen1RadarLabels[i])+" "+std::to_string(stats[i]));
                for (size_t j=i+1;j<texts.size();++j) assert(!intersects(t,texts[j]));
            }
            for (const auto& m:marks) assert(m.x>=10 && m.y>=20 && m.x+m.w<=10+size.first && m.y+m.h<=20+size.second);
        }
    }
    std::cout << "Gen I production radar drawing: five labels, raw values, bounded nonoverlapping text/geometry PASS\n";
}
