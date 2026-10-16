// Gen II MOVES panel runs through the ONE shared Moves panel (UI/SharedMovesPanel.h): native
// max PP (pret/pokecrystal ComputeMaxPP), shared column layout, and touch rows that are exactly
// the rows the panel outlines.
#include "UI/Gen2MovesPanel.h"
#include "UI/SharedPokemonShell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct Text { int x, y, w, h; std::string value; };
struct Rect { int x, y, w, h; };
std::vector<Text> texts;
std::vector<Rect> rects;
void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}
bool overlaps(const Text& a, const Text& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}
} // namespace

namespace UI {
PKSEFramebuffer::PKSEFramebuffer() = default;
PKSEFramebuffer::~PKSEFramebuffer() = default;
void PKSEFramebuffer::measureText(const std::string& s, int& w, int& h, TextStyle style) {
    w = static_cast<int>(s.size()) * (style == TextStyle::Body ? 10 : 8);
    h = style == TextStyle::Body ? 22 : 18;
}
void PKSEFramebuffer::drawText(int x, int y, const std::string& s, Color, TextStyle style) {
    int w = 0, h = 0; measureText(s, w, h, style); texts.push_back({x, y, w, h, s});
}
void PKSEFramebuffer::drawText(int x, int y, const char* s, Color c, TextStyle style) {
    drawText(x, y, std::string(s), c, style);
}
void PKSEFramebuffer::drawFilledRect(int x, int y, int w, int h, Color) { rects.push_back({x, y, w, h}); }
void PKSEFramebuffer::drawFilledRoundedRect(int x, int y, int w, int h, int, Color) { rects.push_back({x, y, w, h}); }
void PKSEFramebuffer::drawRoundedRect(int x, int y, int w, int h, int, Color, int) { rects.push_back({x, y, w, h}); }
void PKSEFramebuffer::setClipRect(int, int, int, int) {}
void PKSEFramebuffer::clearClip() {}
} // namespace UI

int main() {
    using UI::Gen2MovesPanel::Status;
    namespace MP = UI::SharedMovesPanel;
    UI::PKSEFramebuffer fb;

    // Growl (45, 40 PP) with three PP Ups is 61 in Gen II, never the Gen III+ 64.
    const auto slots = UI::Gen2MovesPanel::slots({45, 85, 0, 251}, {61, 24, 0, 5}, {3, 3, 0, 0},
        {Status::Ok, Status::Preserved, Status::Ok, Status::Invalid});
    require(slots[0].maxPP == 61 && MP::ppText(slots[0], Enums::GameVersion::GSC) == "61/61", "Gen II 40-PP move caps at 61");
    require(slots[1].maxPP == 24, "Thunderbolt 15 PP + 3 PP Ups = 24");
    require(slots[2].move == 0 && *slots[2].status == '\0', "empty slot shows no status");
    require(std::string(slots[1].status) == "Preserved" && std::string(slots[3].status) == "Invalid",
            "statuses use the shared panel words");

    // Same geometry as the shared shell the Gen III-V panels use; nothing clips or overlaps.
    constexpr UI::SharedPokemonShell::Geometry g(1280, 720, 56);
    for (int focus = -1; focus < 4; ++focus) {
        texts.clear(); rects.clear();
        UI::Gen2MovesPanel::draw(fb, g.rightX, g.y, g.rightW, slots, focus);
        bool sawEmpty = false, sawCap = false;
        for (std::size_t i = 0; i < texts.size(); ++i) {
            const auto& t = texts[i];
            sawEmpty |= t.value == "Empty";
            sawCap |= t.value == "61/61";
            require(t.x >= g.rightX + 8 && t.x + t.w <= g.rightX + g.rightW - 8, "text inside panel: " + t.value);
            require(t.y + t.h <= g.y + MP::kDataPaneOffset, "text above the data pane: " + t.value);
            for (std::size_t j = i + 1; j < texts.size(); ++j)
                require(!overlaps(t, texts[j]), "no overlap: " + t.value + " / " + texts[j].value);
        }
        require(sawEmpty && sawCap, "empty slot and capped PP drawn");
        require((focus >= 0) == (rects.size() == 1), "one focus outline only when focused");

        // Touch: the centre of each outlined row hits that slot; gaps and outside miss.
        if (focus >= 0) {
            const auto& r = rects.front();
            require(UI::Gen2MovesPanel::hitSlot(r.x + r.w / 2, r.y + r.h / 2, g.rightX, g.y, g.rightW) == focus,
                    "touch row equals focus outline");
            require(UI::Gen2MovesPanel::hitSlot(r.x - 1, r.y + r.h / 2, g.rightX, g.y, g.rightW) == -1, "left of row misses");
        }
    }
    const auto l = MP::layout(g.rightX, g.y, g.rightW);
    require(UI::Gen2MovesPanel::hitSlot(l.focusX() + 4, l.rowTop(0) + MP::Layout::kRowHeight, g.rightX, g.y, g.rightW) == -1,
            "the gap between rows hits nothing");
    require(UI::Gen2MovesPanel::hitSlot(l.focusX() + 4, g.y + MP::kDataPaneOffset + 10, g.rightX, g.y, g.rightW) == -1,
            "the data pane is not a move row");
    std::cout << "Gen II shared moves panel: PASS\n";
    return 0;
}
