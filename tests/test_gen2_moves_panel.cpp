// Gen I and Gen II MOVES panels run through the ONE shared Moves panel (UI/SharedMovesPanel.h): native
// max PP (pret/pokecrystal ComputeMaxPP), shared column layout, and touch rows that are exactly
// the rows the panel outlines.
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
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

    // Gen I goes through the same adapter with its native pret/pokered maximum.
    using Gen1Editor = PokeVault::Integration::Gen1::StagedPokemonEditor;
    const std::array<uint16_t, 4> gen1Moves{45, 85, 0, 165};
    const std::array<uint8_t, 4> gen1PP{61, 24, 0, 1}, gen1Ups{3, 3, 0, 0};
    std::array<int, 4> gen1Max{};
    for (std::size_t i = 0; i < 4; ++i) gen1Max[i] = Gen1Editor::moveMaxPP(gen1Moves[i], gen1Ups[i]);
    require(gen1Max[0] == 61 && gen1Max[1] == 24, "Gen I max PP: 40 PP + 3 Ups = 61, 15 + 3 = 24");
    const auto gen1Slots = UI::ClassicMovesPanel::slots(gen1Moves, gen1PP, gen1Ups, gen1Max,
        {Status::Ok, Status::Invalid, Status::Ok, Status::Preserved});
    require(MP::ppText(gen1Slots[0], Enums::GameVersion::RBY) == "61/61", "Gen I panel shows the native maximum");
    texts.clear(); rects.clear();
    UI::ClassicMovesPanel::draw(fb, g.rightX, g.y, g.rightW, gen1Slots, Enums::GameVersion::RBY, 3);
    for (std::size_t i = 0; i < texts.size(); ++i) {
        require(texts[i].x + texts[i].w <= g.rightX + g.rightW - 8 && texts[i].y + texts[i].h <= g.y + MP::kDataPaneOffset,
                "Gen I text inside the panel: " + texts[i].value);
        for (std::size_t j = i + 1; j < texts.size(); ++j)
            require(!overlaps(texts[i], texts[j]), "Gen I no overlap: " + texts[i].value);
    }
    const auto r = UI::ClassicMovesPanel::slotRect(3, g.rightX, g.y, g.rightW);
    require(rects.size() == 1 && rects[0].x == r.x && rects[0].y == r.y && rects[0].w == r.w && rects[0].h == r.h,
            "Gen I touch button equals the focus outline");

    // Shared Move chooser rows in the 760 px Gen I/II pickers: real Gen I/II types (Bite is Normal in
    // Gen I and Dark in Gen II), every caption and cell inside the row, nothing overlapping.
    const auto chooser = MP::chooserColumns((1280 - 760) / 2 + 40, 760 - 80);
    for (const auto game : {Enums::GameVersion::RBY, Enums::GameVersion::GSC}) {
        texts.clear();
        MP::drawChooserCaptions(fb, chooser, 58, game);
        MP::drawChooserRow(fb, chooser, 100, 44, game, true);
        bool biteType = false, unknown = false;
        for (std::size_t i = 0; i < texts.size(); ++i) {
            const auto& t = texts[i];
            biteType |= t.value == (game == Enums::GameVersion::RBY ? "Normal" : "Dark");
            unknown |= t.value == "???";
            require(t.x >= chooser.x && t.x + t.w <= chooser.x + chooser.w, "chooser text inside row: " + t.value);
            for (std::size_t j = i + 1; j < texts.size(); ++j)
                require(!overlaps(t, texts[j]), "chooser overlap: " + t.value + " / " + texts[j].value);
        }
        require(biteType && !unknown, "chooser shows the native Gen I/II type");
    }
    std::cout << "Gen I/II shared moves panel: PASS\n";
    return 0;
}
