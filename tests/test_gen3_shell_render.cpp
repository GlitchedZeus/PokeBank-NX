#include "UI/SharedPokemonShell.h"
#include <cassert>
#include <iostream>
#include <vector>
namespace {
struct Text { int x,y,w,h; std::string value; };
std::vector<Text> texts;
struct Rect { int x,y,w,h; };
std::vector<Rect> marks;
bool intersects(const Text& a, const Text& b) {
    return a.x < b.x+b.w && a.x+a.w > b.x && a.y < b.y+b.h && a.y+a.h > b.y;
}
}
namespace UI {
PKSEFramebuffer::PKSEFramebuffer() = default;
PKSEFramebuffer::~PKSEFramebuffer() = default;
void PKSEFramebuffer::measureText(const std::string& s,int& w,int& h,TextStyle) { w=s.size()*9; h=18; }
void PKSEFramebuffer::drawText(int x,int y,const std::string& s,Color,TextStyle style) {
    int w,h; measureText(s,w,h,style); texts.push_back({x,y,w,h,s});
}
void PKSEFramebuffer::drawText(int x,int y,const char* s,Color color,TextStyle style) { drawText(x,y,std::string(s),color,style); }
void PKSEFramebuffer::drawFilledRect(int x,int y,int w,int h,Color) { marks.push_back({x,y,w,h}); }
void PKSEFramebuffer::drawFilledCircle(int x,int y,int r,Color) { marks.push_back({x-r,y-r,2*r,2*r}); }
void PKSEFramebuffer::drawFilledRoundedRect(int x,int y,int w,int h,int,Color) { marks.push_back({x,y,w,h}); }
void PKSEFramebuffer::drawRoundedRect(int x,int y,int w,int h,int,Color,int) { marks.push_back({x,y,w,h}); }
void PKSEFramebuffer::setClipRect(int,int,int,int) {}
void PKSEFramebuffer::clearClip() {}
}
int main() {
    UI::PKSEFramebuffer fb;
    constexpr UI::SharedPokemonShell::Geometry g(1280,720,46);
    // Ellipsis must fit and preserve complete UTF-8 characters (the font stub measures bytes).
    assert(UI::SharedPokemonShell::fitDetailsText(fb, "Pokémon", 200) == "Pokémon");
    assert(UI::SharedPokemonShell::fitDetailsText(fb, "Pokémon", 63) == "Pok...");
    assert(UI::SharedPokemonShell::fitDetailsText(fb, "Pokémon", 9).empty());
    for (const auto theme : {UI::ThemeMode::Dark, UI::ThemeMode::Light}) {
        UI::applyTheme(theme);
        const std::array<const char*,15> labels{{"Species", "Nickname", "Level", "Experience",
            "OT", "Trainer ID", "SID (read-only)", "Held Item", "Friendship", "Language",
            "Origin", "Ball", "Met Level", "Met Location", "Pokerus"}};
        for (std::size_t focus=0; focus<15; ++focus) {
            texts.clear(); marks.clear();
            UI::SharedPokemonShell::drawScrollableDetails(fb,g.leftX,g.y,g.leftW,g.h,15,focus,true,
                [&](std::size_t row) { return labels[row]; },
                [](std::size_t row) { return "Full native value " + std::to_string(row); });
            bool selectedLabelVisible=false;
            for (std::size_t i=0; i<texts.size(); ++i) {
                const auto& t=texts[i];
                assert(t.x>=g.leftX && t.x+t.w<=g.leftX+g.leftW);
                assert(t.y>=g.y+224 && t.y+t.h<=g.y+g.h);
                if (t.value == labels[focus]) selectedLabelVisible=true;
                for (std::size_t j=i+1; j<texts.size(); ++j) assert(!intersects(t,texts[j]));
            }
            bool clearedViewport = false;
            bool focusOutline = false;
            for (const auto& mark : marks) {
                clearedViewport |= mark.y == g.y + 216 && mark.h > 100;
                focusOutline |= mark.h == 30 && mark.y >= g.y + 220 &&
                                mark.y + mark.h <= g.y + g.h;
            }
            assert(selectedLabelVisible && clearedViewport && focusOutline);
            // Every field/value pair shares its baseline and leaves the scrollbar gutter clear.
            for (std::size_t i=0; i+1<texts.size()-1; i+=2) {
                assert(texts[i].y == texts[i+1].y);
                assert(texts[i].x + texts[i].w + 12 <= texts[i+1].x);
                assert(texts[i+1].x + texts[i+1].w <= g.leftX + g.leftW - 28);
            }
        }
        texts.clear(); marks.clear();
        struct Row { std::string label,value; };
        const std::vector<Row> rows{{"Origin","3"},{"PID fields","READ ONLY"},{"Legality","Not checked"}};
        UI::SharedPokemonShell::drawDataAndGraph(fb,g.rightX,g.y+244,g.rightW,g.h-256,
            "GEN III DATA",rows,{301,200,180,245,279,185});
        bool radar=false, data=false;
        for (const auto& t:texts) {
            assert(t.x>=g.rightX && t.x+t.w<=g.rightX+g.rightW);
            assert(t.y>=g.y+244 && t.y+t.h<=g.y+g.h);
            radar |= t.value=="BATTLE STATS"; data |= t.value=="GEN III DATA";
        }
        assert(radar && data && !marks.empty());
    }
    std::cout << "Shared Gen III Details and data/radar renderer bounds: PASS\n";
}
