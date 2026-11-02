// R2-BUG-006 / R2-BUG-005: the shared compact grid item picker (include/UI/SharedItemGridPicker.h)
// used by every Gen I-V Bag "Add Item" picker, and its TM/HM detail line. Host-only: a recording
// framebuffer, SpriteManager stub with the real 200x40 type badge size, touch fakes.
#include "UI/SharedItemGridPicker.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct Text { int x, y, w, h; std::string value; };
struct Box { int x, y, w, h; };
std::vector<Text> texts;
std::vector<Box> highlights;
std::vector<Box> images;
bool clipping = false;
Box clipRect{};
std::array<uint8_t, 4> pixel{};
UI::Sprite typeSprite;

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAIL: " << what << '\n';
    assert(ok);
}
struct Button { int id, x, y, w, h; };
} // namespace

namespace UI {
PKSEFramebuffer::PKSEFramebuffer() = default;
PKSEFramebuffer::~PKSEFramebuffer() = default;
void PKSEFramebuffer::measureText(const std::string& s, int& w, int& h, TextStyle) {
    w = static_cast<int>(s.size()) * 9; h = 18;
}
void PKSEFramebuffer::drawText(int x, int y, const std::string& s, Color, TextStyle style) {
    int w = 0, h = 0; measureText(s, w, h, style);
    texts.push_back({x, y, w, h, s});
}
void PKSEFramebuffer::drawText(int x, int y, const char* s, Color c, TextStyle style) {
    drawText(x, y, std::string(s), c, style);
}
void PKSEFramebuffer::drawFilledRect(int, int, int, int, Color) {}
void PKSEFramebuffer::drawFilledRoundedRect(int, int, int, int, int, Color) {}
void PKSEFramebuffer::drawRoundedRect(int, int, int, int, int, Color, int) {}
void PKSEFramebuffer::drawSelectionHighlight(int x, int y, int w, int h) { highlights.push_back({x, y, w, h}); }
void PKSEFramebuffer::setClipRect(int x, int y, int w, int h) { clipping = true; clipRect = {x, y, w, h}; }
void PKSEFramebuffer::clearClip() { clipping = false; }
void PKSEFramebuffer::drawImageScaled(int x, int y, int, int, int w, int h, const uint8_t*, int) {
    images.push_back({x, y, w, h});
}
Sprite::~Sprite() = default;
Sprite* SpriteManager::getTypeSprite(uint8_t typeId) {
    if (typeId >= 18) return nullptr;
    typeSprite.width = 200; typeSprite.height = 40; typeSprite.channels = 4; typeSprite.data = pixel.data();
    return &typeSprite;
}
} // namespace UI

namespace {
namespace Grid = UI::SharedItemGridPicker;
namespace MG = PokeBank::UIModel::SharedMenuGeometry;

std::vector<std::string> labels(int count, const std::string& stem) {
    std::vector<std::string> out;
    for (int i = 0; i < count; ++i) out.push_back(stem + std::to_string(i));
    return out;
}

void testLayoutFitsScreen() {
    for (int count : {0, 1, 3, 4, 5, 39, 40, 41, 100, 250}) {
        for (int columns : {2, 3, 4}) {
            const auto g = Grid::grid(count, columns);
            require(g.frame.x >= MG::kMargin && g.frame.right() <= MG::kScreenW - MG::kMargin, "frame on screen");
            require(g.frame.y >= MG::kMargin && g.frame.bottom() <= MG::kScreenH - MG::kFooterH - MG::kMargin,
                    "frame above the footer");
            require(g.visibleRows >= 1 && g.visibleRows <= Grid::kMaxRows, "1..10 visible rows");
            require(g.list.y >= g.frame.y + Grid::kDetailTop + 21, "TM/HM detail line sits above the cells");
            for (int i = 0; i < std::min(count, g.page()); ++i) {
                const auto c = Grid::cellRect(g, i, 0);
                require(c.inside(g.list), "every visible cell inside the list");
                require(Grid::hitIndex(g, 0, c.x + c.w / 2, c.y + c.h / 2) == i, "touch centre == drawn cell");
            }
        }
    }
    // Content-driven: a 3-item pocket is one short row; 40+ items fill 10 rows x 4 columns.
    require(Grid::grid(3, 4).visibleRows == 1 && Grid::grid(3, 4).frame.h < Grid::grid(41, 4).frame.h, "short pocket, short frame");
    require(Grid::grid(41, 4).page() == 40, "4 columns x 10 rows");
    // Gaps between cells and the area right of the last item miss.
    const auto g = Grid::grid(41, 4);
    const auto c0 = Grid::cellRect(g, 0, 0);
    require(Grid::hitIndex(g, 0, c0.right() + 1, c0.y + 5) == -1, "column gutter misses");
    require(Grid::hitIndex(g, 0, c0.x + 5, c0.bottom() + 1) == -1, "row gap misses");
    const auto g5 = Grid::grid(5, 4);
    const auto last = Grid::cellRect(g5, 4, 0);
    require(Grid::hitIndex(g5, 0, last.right() + Grid::kCellGapX + 5, last.y + 5) == -1, "empty cell after the last item misses");
    std::cout << "layout fits 1280x720 ok\n";
}

void testShortListsAreBalanced() {
    // R2-BUG-006 Round 3 note: under ~20 items, balanced rows/columns instead of long sparse rows.
    auto names = labels(19, "Potion ");
    auto at = [&names](int i) { return names[static_cast<std::size_t>(i)]; };
    require(Grid::balancedColumns(1, 4) == 1 && Grid::balancedColumns(2, 4) == 2 && Grid::balancedColumns(3, 4) == 3,
            "1-3 items: one row");
    require(Grid::balancedColumns(5, 4) == 3, "5 items: 3+2, not 4+1");
    require(Grid::balancedColumns(6, 4) == 3 && Grid::balancedColumns(9, 4) == 3, "6 and 9 items: 3 columns");
    require(Grid::balancedColumns(7, 4) == 4 && Grid::balancedColumns(16, 4) == 4, "7 and 16 items: 4 columns");
    require(Grid::gridFor(3, at).frame.w < Grid::grid(3, 4).frame.w, "a 3-item pocket gets a narrower frame");
    require(Grid::balancedColumns(5, 2) <= 2, "never more columns than the names fit");
    for (int count = 1; count < Grid::kBalancedBelow; ++count) {
        const auto g = Grid::gridFor(count, at);
        const auto full = Grid::grid(count, 4);
        const int empty = g.totalRows * g.columns - count;
        require(empty < g.columns, "no fully empty row");
        require(g.totalRows == Grid::grid(count, 4).totalRows, "no extra rows: as few rows as 4 columns allow");
        require(g.frame.w <= full.frame.w && g.frame.w >= std::min(Grid::kMinBalancedW, full.frame.w), "frame narrows, not below the minimum");
        require(g.frame.x >= MG::kMargin && g.frame.right() <= MG::kScreenW - MG::kMargin, "balanced frame on screen");
        require(std::abs((g.frame.x + g.frame.w / 2) - (full.frame.x + full.frame.w / 2)) <= 1, "frame stays centred");
        require(g.list.x >= g.frame.x + Grid::kRule.padX && g.list.right() <= g.frame.right() - Grid::kRule.padX, "cells inside the frame");
        require(g.cellWidth == full.cellWidth, "cells keep the width the names were fitted at");
        for (int i = 0; i < count; ++i) {
            const auto c = Grid::cellRect(g, i, 0);
            require(c.inside(g.list), "balanced cell inside the list");
            require(Grid::hitIndex(g, 0, c.x + c.w / 2, c.y + c.h / 2) == i, "balanced touch centre == drawn cell");
            require(MG::fitsWidth(names[static_cast<std::size_t>(i)], MG::Text::Caption, Grid::labelWidth(g, true)), "name fits");
        }
    }
    // 20 or more items keep the full 4-column grid.
    const auto big = labels(20, "Potion ");
    const auto g20 = Grid::gridFor(20, [&big](int i) { return big[static_cast<std::size_t>(i)]; });
    require(g20.columns == 4 && g20.frame.w == Grid::grid(20, 4).frame.w, "20+ items: full grid");
    std::cout << "short lists balanced ok\n";
}

void testNamesNeverClipped() {
    // Short native names: 4 columns.
    const auto shortNames = labels(60, "Potion ");
    auto at = [&](const std::vector<std::string>& v) { return [&v](int i) { return v[static_cast<std::size_t>(i)]; }; };
    const auto g4 = Grid::gridFor(60, at(shortNames));
    require(g4.columns == 4, "short names use 4 columns");
    // A long TM label that does not fit a 4-column cell drops the WHOLE grid to fewer columns.
    auto tms = labels(60, "TM");
    tms[37] = "TM37 \xE2\x80\x94 Thunderbolt Thunderpunch";
    const auto g3 = Grid::gridFor(60, at(tms));
    require(g3.columns < 4, "a long label reflows to fewer columns");
    for (const auto& name : tms)
        require(MG::fitsWidth(name, MG::Text::Caption, Grid::labelWidth(g3, true)), "every label fits: " + name);
    std::cout << "names never clipped ok\n";
}

void testNavigation() {
    const auto g = Grid::grid(42, 4);   // 11 rows, last row has 2 items (40, 41), 10 visible
    using K = Grid::Keys;
    K right; right.right = true; K left; left.left = true; K up; up.up = true; K down; down.down = true;
    K l; l.l = true; K r; r.r = true; K zl; zl.zl = true; K zr; zr.zr = true;
    require(Grid::navigate(g, 0, right) == 1 && Grid::navigate(g, 3, right) == 3, "Right stays in the row");
    require(Grid::navigate(g, 4, left) == 4 && Grid::navigate(g, 5, left) == 4, "Left stays in the row");
    require(Grid::navigate(g, 41, right) == 41, "Right never leaves the item list");
    require(Grid::navigate(g, 1, down) == 5 && Grid::navigate(g, 5, up) == 1, "Up/Down move one row");
    require(Grid::navigate(g, 1, up) == 41, "Up from the top wraps to the bottom (same column, clamped)");
    require(Grid::navigate(g, 2, up) == 41, "column 2 has no bottom cell: lands on the last item");
    require(Grid::navigate(g, 39, down) == 41, "no cell below in the partial last row: lands on the last item");
    require(Grid::navigate(g, 40, down) == 0, "Down from the last row wraps to the top, same column");
    require(Grid::navigate(g, 38, down) == 41, "partial last row: Down lands on the last item");
    require(Grid::navigate(g, 41, down) == 1, "Down from the last row wraps to the top, same column");
    require(Grid::navigate(g, 1, r) == 41, "R pages 10 rows keeping the column (clamped)");
    require(Grid::navigate(g, 6, r) == 41 && Grid::navigate(g, 41, l) == 1, "L pages back keeping the column");
    require(Grid::navigate(Grid::grid(400, 4), 2, zr) == 202 && Grid::navigate(Grid::grid(400, 4), 202, zl) == 2,
            "ZL/ZR jump five pages");
    require(Grid::navigate(Grid::grid(0, 4), 5, down) == 0, "empty list is safe");
    require(Grid::navigate(Grid::grid(400, 4), 2, r, 3) == 14, "an explicit page step is honoured");
    // Every reachable index stays in range from every start.
    for (int count : {1, 2, 5, 13, 42}) {
        const auto gg = Grid::grid(count, 4);
        for (int i = 0; i < count; ++i)
            for (const K& k : {right, left, up, down, l, r, zl, zr}) {
                const int next = Grid::navigate(gg, i, k);
                require(next >= 0 && next < count, "navigation stays inside the list");
            }
    }
    std::cout << "navigation ok\n";
}

void testWindowScrollAndReflow() {
    const auto g = Grid::grid(120, 4);   // 30 rows, 10 visible
    for (int selected = 0; selected < 120; ++selected) {
        const int first = Grid::firstRowFor(g, selected);
        require(Grid::containsSelection(g, first, selected), "cursor-driven window always shows the focus");
    }
    // Scrollbar: a drag to row 20 keeps the selected column and lands in the window.
    const int moved = Grid::indexForFirstRow(g, 6 /* column 2 */, 20);
    require(moved % 4 == 2 && Grid::containsSelection(g, 20, moved), "scroll keeps the column, focus in view");
    require(Grid::scrollModel(g, 99).scroll == g.maxFirstRow(), "scroll model clamps the window");
    require(Grid::scrollModel(g, 0).content == 30 && Grid::scrollModel(g, 0).viewport == 10, "scrollbar counts rows");
    // Viewport-driven (classic) reveal moves the window the minimum.
    require(Grid::revealFirstRow(g, 0, 47) == 2 && Grid::revealFirstRow(g, 15, 47) == 11, "reveal scrolls minimally");
    // Reflow: the selected INDEX (item identity) survives a column change and is still shown.
    const int selected = 77;
    for (int columns : {4, 3, 2}) {
        const auto gg = Grid::grid(120, columns);
        require(Grid::containsSelection(gg, Grid::firstRowFor(gg, selected), selected), "reflow keeps focus visible");
    }
    std::cout << "window / scroll / reflow ok\n";
}

void testDrawCellsPublishesClippedTargets() {
    UI::PKSEFramebuffer fb;
    const auto g = Grid::grid(60, 4);
    std::vector<Button> buttons;
    const auto names = labels(60, "Item ");
    texts.clear(); highlights.clear();
    // A live pixel offset of half a row: the partly visible rows publish only their visible pixels.
    Grid::drawCells(fb, g, 13, 2, -20,
                    [&](const MG::Rect& cell, int i, bool focused) {   // same contract as drawMenuCell
                        if (focused) fb.drawSelectionHighlight(cell.x, cell.y, cell.w, cell.h);
                        buttons.push_back({9000 + i, cell.x, cell.y, cell.w, cell.h});
                    },
                    [&](int i) { return names[static_cast<std::size_t>(i)]; },
                    [](UI::PKSEFramebuffer&, const MG::Rect&, int) {},
                    [](int i) { return static_cast<uint16_t>(i == 13 ? 12 : 0); });
    require(!buttons.empty(), "cells publish touch targets");
    for (const auto& b : buttons) {
        require(b.x >= g.list.x && b.y >= g.list.y && b.x + b.w <= g.list.right() && b.y + b.h <= g.list.bottom(),
                "touch target clipped to the visible grid");
        require(b.id >= 9000 && b.id < 9060, "touch id = base + index");
    }
    require(highlights.size() == 1, "exactly one focused cell");
    bool focusPublished = false;
    for (const auto& b : buttons)
        focusPublished |= b.id == 9013 && b.x == highlights[0].x && b.y == highlights[0].y &&
                          b.w == highlights[0].w && b.h == highlights[0].h;
    require(focusPublished, "highlight rectangle == touch rectangle");
    bool held = false;
    for (const auto& t : texts) held |= t.value == "x12";
    require(held && !clipping, "held quantity drawn; clip released");
    std::cout << "draw cells ok\n";
}

void testMachineDetail() {
    UI::PKSEFramebuffer fb;
    // Thunderbolt (85): damaging, Electric badge, PWR 95 ACC 100 PP 15 (Gen IV).
    texts.clear(); images.clear();
    const int w = Grid::drawMachineDetail(fb, 100, 100, 900, Enums::GameVersion::HGSS, 85);
    require(w > 0 && images.size() == 1 && images[0].h == 16 && images[0].w == 80, "type badge drawn");
    bool name = false, stats = false;
    for (const auto& t : texts) {
        name |= t.value == "Thunderbolt";
        stats |= t.value == "PWR 95   ACC 100   PP 15";
    }
    require(name && stats, "name and PWR/ACC/PP shown");
    // Toxic (92): non-damaging -> PWR "—".
    texts.clear(); images.clear();
    Grid::drawMachineDetail(fb, 100, 100, 900, Enums::GameVersion::HGSS, 92);
    bool dash = false;
    for (const auto& t : texts) dash |= t.value.rfind("PWR \xE2\x80\x94", 0) == 0;
    require(dash, "non-damaging move shows PWR —");
    // Not a machine (move 0) or a move that does not exist in the era: nothing drawn.
    texts.clear(); images.clear();
    require(Grid::drawMachineDetail(fb, 100, 100, 900, Enums::GameVersion::HGSS, 0) == 0 && texts.empty(),
            "non-machine draws nothing");
    require(Grid::drawMachineDetail(fb, 100, 100, 900, Enums::GameVersion::RBY, 200) == 0,
            "a move outside the generation draws nothing");
    // Same move id, different generation facts: Bite is Normal in Gen I, Dark in Gen II.
    texts.clear(); images.clear();
    Grid::drawMachineDetail(fb, 100, 100, 900, Enums::GameVersion::RBY, 44);
    require(images.size() == 1, "Gen I detail draws its native type badge");
    std::cout << "TM/HM detail ok\n";
}

} // namespace

int main() {
    testLayoutFitsScreen();
    testShortListsAreBalanced();
    testNamesNeverClipped();
    testNavigation();
    testWindowScrollAndReflow();
    testDrawCellsPublishesClippedTargets();
    testMachineDetail();
    std::cout << "test_item_grid_picker: all passed\n";
    return 0;
}
