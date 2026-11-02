// L06 (Round 6, owner: Held Item selection unreliable; "in Gen III neither A nor touch selected an
// item, and some taps do nothing"). Drives UI/SharedHeldItemInput.h, the one grid input step the
// Gen II, Gen III and Gen IV/V Held Item pickers run, with real touch sequences (contact / wobble /
// release through the real TouchScrollState) and D-pad + A, at the top, middle and bottom rows,
// with the search dock closed (4 columns) and open (2 columns), over full and searched lists.
// A tap and A must commit the same item; what is expected is computed from the drawn boxes
// (SharedHeldItemPickerPresentation::drawGrid geometry), independently of the code under test.
#include "UI/SharedHeldItemInput.h"
#include "UI/ItemPickerSearch.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
namespace In = UI::SharedHeldItemInput;
namespace Grid = PokeBank::UIModel::SharedHeldItemPicker;

constexpr int kModalX = (1280 - Grid::modalWidth) / 2;          // as every surface centres it
constexpr int kModalY = (720 - 56 - Grid::modalHeight) / 2;     // kNavBarH = 56

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void has(const std::string& text, const std::string& needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}

struct Picker {
    std::vector<uint16_t> shown;
    bool searchOpen = false;
    int selected = 0;
    UI::TouchScrollState scroll;
    UI::TouchPickerViewport view;
    UI::TouchInput touch;
    int committed = -1;   // the item a commit applied
    int reveals = 0;

    int count() const { return static_cast<int>(shown.size()); }
    In::Result frame(const In::Keys& keys = {}) {
        const auto r = In::update(touch, scroll, view, selected, count(), searchOpen, kModalX, kModalY, keys);
        touch.nextFrame();
        if (r.revealed) ++reveals;
        if (r.choose) committed = shown[static_cast<std::size_t>(selected)];
        return r;
    }
    void settle() { for (int i = 0; i < 160; ++i) frame(); }
    // The drawn box of shown index i on the last frame (drawGrid: cell at gridY + row*pitch +
    // offset, box from cellY - 3, (cellWidth - 10) x (rowPitch - 4)).
    struct Box { int x, y, w, h; };
    Box box(int i) const {
        const int columns = Grid::columnsForSearch(searchOpen);
        const int first = std::clamp(view.firstRow, 0, (count() - 1) / columns) * columns;
        const int offset = i - first;
        const int cellX = kModalX + Grid::gridInsetX + (offset % columns) * Grid::cellWidth;
        const int cellY = kModalY + Grid::gridInsetY + (offset / columns) * Grid::rowPitch + scroll.offset();
        return {cellX, cellY - 3, Grid::cellWidth - 10, Grid::rowPitch - 4};
    }
    // The item a pixel shows: the box under it, else -1 (a dead gap or outside the clip).
    int drawnAt(int px, int py) const {
        const auto v = In::viewport(kModalX, kModalY, searchOpen);
        if (!In::inside(v, px, py)) return -1;
        for (int i = 0; i < count(); ++i) {
            const auto b = box(i);
            if (px >= b.x && px < b.x + b.w && py >= b.y && py < b.y + b.h) return i;
        }
        return -1;
    }
    // A finger: contact, some wobble frames, release.
    In::Result tap(int x, int y, int dx = 0, int dy = 0) {
        touch.contact(x, y); frame();
        touch.contact(x + dx / 2, y + dy / 2); frame();
        touch.contact(x + dx, y + dy); frame();
        touch.release();
        return frame();
    }
    void drag(int x, int y, int dy) {
        touch.contact(x, y); frame();
        for (int step = 1; step <= 8; ++step) { touch.contact(x, y + dy * step / 8); frame(); }
        // A slow last few pixels, so release leaves no coast: exactly the rows dragged.
        const int sign = dy < 0 ? -1 : 1;
        for (int step = 1; step <= 5; ++step) { touch.contact(x, y + dy + sign * step); frame(); }
        touch.release(); frame();
        settle();
    }
};

std::vector<uint16_t> items(int n) {
    std::vector<uint16_t> v;
    for (int i = 0; i < n; ++i) v.push_back(static_cast<uint16_t>(i == 0 ? 0 : 100 + i));
    return v;
}

// Scroll the grid with a real finger drag until shown row `row` is the first visible row.
void scrollTo(Picker& p, int row) {
    const auto v = In::viewport(kModalX, kModalY, p.searchOpen);
    for (int guard = 0; guard < 200 && p.view.firstRow != row; ++guard) {
        const int rows = std::clamp(row - p.view.firstRow, -6, 6);
        p.drag(v.x + 30, v.y + (rows > 0 ? v.h - 60 : 60), -rows * Grid::rowPitch);
    }
    assert(p.view.firstRow == row);
}

// Every visible top / middle / bottom cell, tapped at its box's edges, centre and the dead gap
// beneath it, with up to tap-slop wobble in every direction, commits the item drawn there.
void testTapsCommitTheDrawnItem() {
    int taps = 0;
    for (const bool open : {false, true})
        for (const int firstRow : {0, 7, -1 /* last page */}) {
            Picker base;
            base.shown = items(130);
            base.searchOpen = open;
            base.frame();
            const int columns = Grid::columnsForSearch(open);
            const int lastFirst = UI::TouchPickerViewport::maxFirst(base.count(), Grid::rows, columns);
            scrollTo(base, firstRow < 0 ? lastFirst : firstRow);
            for (const int visibleRow : {0, 5, Grid::rows - 1})
                for (int column = 0; column < columns; ++column) {
                    const int index = (base.view.firstRow + visibleRow) * columns + column;
                    if (index >= base.count()) continue;
                    const auto b = base.box(index);
                    const int ys[] = {b.y + 1, b.y + b.h / 2, b.y + b.h - 1, b.y + b.h + 2 /* gap */};
                    const int xs[] = {b.x + 1, b.x + b.w / 2, b.x + b.w + 4 /* gap */};
                    for (const int y : ys)
                        for (const int x : xs)
                            for (const int dy : {-20, -9, 0, 9, 20})
                                for (const int dx : {-15, 0, 15}) {
                                    if (dx * dx + dy * dy > 22 * 22) continue;   // a drag, not a tap
                                    if (!In::inside(In::viewport(kModalX, kModalY, open), x, y)) continue;
                                    Picker p = base;
                                    p.touch = UI::TouchInput{};
                                    p.selected = (index + 3) % p.count();   // focus elsewhere
                                    const int expected = p.drawnAt(x, y) >= 0 ? p.drawnAt(x, y) : index;
                                    const auto r = p.tap(x, y, dx, dy);
                                    if (!r.choose || p.committed != p.shown[static_cast<std::size_t>(expected)])
                                        std::cerr << "lost tap open=" << open << " first=" << base.view.firstRow
                                                  << " index=" << index << " at " << x << ',' << y
                                                  << " wobble " << dx << ',' << dy << '\n';
                                    assert(r.choose && p.committed == p.shown[static_cast<std::size_t>(expected)]);
                                    assert(p.reveals == 0);
                                    ++taps;
                                }
                }
        }
    std::cout << taps << " taps at top / middle / bottom rows commit the drawn item ok\n";
}

// D-pad to the same top / middle / bottom cells and press A: the same item as the tap.
void testAMatchesTap() {
    for (const bool open : {false, true}) {
        const int columns = Grid::columnsForSearch(open);
        for (const int target : {0, 5 * columns + 1, 9 * columns + columns - 1, 57, 129}) {
            Picker a;
            a.shown = items(130);
            a.searchOpen = open;
            a.frame();
            In::Keys right{}; right.right = true;
            In::Keys down{}; down.down = true;
            while (a.selected / columns < target / columns) a.frame(down);
            while (a.selected % columns < target % columns) a.frame(right);
            assert(a.selected == target);
            In::Keys press{}; press.a = true;
            assert(a.frame(press).choose && a.committed == a.shown[static_cast<std::size_t>(target)]);

            Picker t;
            t.shown = items(130);
            t.searchOpen = open;
            t.frame();
            scrollTo(t, std::min(target / columns, UI::TouchPickerViewport::maxFirst(t.count(), Grid::rows, columns)));
            const auto b = t.box(target);
            assert(t.tap(b.x + b.w / 2, b.y + b.h / 2).choose && t.committed == a.committed);
        }
    }
    std::cout << "A and tap commit the same item ok\n";
}

// Searched lists: the filtered index maps to the shown item, for tap and A alike.
void testSearchedList() {
    const std::vector<uint16_t> native = items(130);
    const auto name = [](uint16_t id) { return id == 0 ? std::string("None") : (id % 3 ? "Berry " : "Ball ") + std::to_string(id); };
    for (const bool open : {false, true}) {
        Picker p;
        p.shown = native;
        p.searchOpen = open;
        p.selected = UI::ItemPickerSearch::apply(p.shown, native, UI::ItemPickerSort::Mode::NameAZ, "ball", 0, name);
        assert(p.selected == 0 && !p.shown.empty() && p.shown.size() < native.size());
        p.frame();
        const int columns = Grid::columnsForSearch(open);
        const int last = p.count() - 1;
        scrollTo(p, UI::TouchPickerViewport::maxFirst(p.count(), Grid::rows, columns));
        const auto b = p.box(last);
        assert(p.tap(b.x + 20, b.y + 10, 0, 12).choose && p.committed == p.shown.back());
        assert(name(static_cast<uint16_t>(p.committed)).rfind("Ball", 0) == 0);

        Picker a;
        a.shown = p.shown;
        a.searchOpen = open;
        a.frame();
        In::Keys page{}; page.r = true;
        In::Keys press{}; press.a = true;
        In::Keys down{}; down.down = true;
        In::Keys right{}; right.right = true;
        if (!open) for (int i = 0; i < 5; ++i) a.frame(page);
        for (int i = 0; i < 80; ++i) a.frame(down);
        for (int i = 0; i < 4; ++i) a.frame(right);
        assert(a.selected == last);
        assert(a.frame(press).choose && a.committed == p.committed);
    }
    std::cout << "searched list: tap and A commit the shown item ok\n";
}

void testNonTaps() {
    Picker p;
    p.shown = items(130);
    p.frame();
    const auto v = In::viewport(kModalX, kModalY, false);
    // A drag scrolls and never commits the row under the finger.
    p.touch.contact(v.x + 40, v.y + 300); p.frame();
    for (int y = 290; y >= 100; y -= 10) { p.touch.contact(v.x + 40, v.y + y); assert(!p.frame().choose); }
    p.touch.release(); assert(!p.frame().choose);
    p.settle();
    assert(p.view.firstRow > 0 && p.committed < 0);
    // A gesture that began outside the grid (the search dock, the header) never picks a cell.
    p.touch.contact(v.x + 40, v.y - 10); p.frame();
    p.touch.contact(v.x + 40, v.y + 5); p.frame();
    p.touch.release(); assert(!p.frame().choose);
    Picker docked;
    docked.shown = items(130);
    docked.searchOpen = true;
    docked.frame();
    const auto dv = In::viewport(kModalX, kModalY, true);
    assert(!docked.tap(dv.x + dv.w + 10, dv.y + 100).choose);   // keyboard side of the dock
    assert(docked.committed < 0);
    // A focus scrolled out of view: A brings it back first, the next A commits it.
    Picker hidden;
    hidden.shown = items(130);
    hidden.frame();
    scrollTo(hidden, 20);
    In::Keys press{}; press.a = true;
    const auto first = hidden.frame(press);
    assert(!first.choose && first.revealed && hidden.reveals == 1);
    assert(hidden.frame(press).choose && hidden.committed == hidden.shown[0]);
    // Empty list: nothing to commit.
    Picker empty;
    assert(!empty.frame(press).choose && !empty.tap(v.x + 40, v.y + 40).choose);
    std::cout << "drags, outside starts and hidden focus never commit ok\n";
}

// A row the residual scroll leaves partly visible is still a tap target, not a reveal.
void testPartlyVisibleRow() {
    Picker p;
    p.shown = items(130);
    p.frame();
    scrollTo(p, 10);
    const auto v = In::viewport(kModalX, kModalY, false);
    // A slow 30 px drag down (less than a row) and lift: row 9 peeks in at the top while the
    // residual eases back, and the very next tap lands on it.
    p.touch.contact(v.x + 40, v.y + 200); p.frame();
    for (const int y : {220, 225, 226, 227, 228, 229, 230}) { p.touch.contact(v.x + 40, v.y + y); p.frame(); }
    assert(p.scroll.offset() == 30);
    p.touch.release();
    assert(!p.frame().choose && p.scroll.offset() == 30);
    const int peek = p.drawnAt(v.x + 40, v.y + 4);
    assert(peek == 9 * Grid::columns);
    assert(p.tap(v.x + 40, v.y + 4).choose && p.committed == p.shown[static_cast<std::size_t>(peek)]);
    assert(p.reveals == 0);
    std::cout << "partly visible row commits on tap ok\n";
}

void testSurfacesUseTheSharedStep() {
    const std::string gen2 = read("src/UI/Gen2PokemonEditorFoundation.inc");
    const std::string gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    const std::string gen45 = read("src/UI/SharedPokemonEditorSurface.inc");
    has(gen2, "const auto step = SharedHeldItemInput::update(touch, state.itemScroll, state.itemViewport,\n"
              "        state.itemChoiceIndex, count, state.heldItemSearch.open(), px, py,");
    has(gen2, "state.working.heldItem = state.itemChoices[static_cast<std::size_t>(state.itemChoiceIndex)];");
    // The Gen II modal owns the frame's touch, so covered workspace rows cannot shadow its cells.
    has(gen2, "    UI::SharedEditorChrome::beginModal(screen.touchButtons);\n    SharedPopupDismiss::publish(x, y, w, h);   // 7D: a tap outside closes it like B\n    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 165));");
    for (const std::string* src : {&gen3, &gen45}) {
        has(*src, "#include \"UI/SharedHeldItemInput.h\"");
        has(*src, "const auto held = SharedHeldItemInput::update(touch, state.pickerScroll, state.pickerViewport,\n"
                  "            state.pickerRow, count, state.heldItemSearch.open(), px, py,");
        has(*src, "if (gridPicker && !(down & HidNpadButton_B)) {");
        has(*src, "// L06: SharedHeldItemInput::update below owns the grid's scroll, tap and A.");
    }
    has(gen3, "if (!gridPicker && !pickerBar.consumed && touchTap >= 4000 && touchTap < 4000 + count) {");
    has(gen3, "if (held.choose) applyValuePicker(state);");
    has(gen45, "if (!gridPicker && !pickerBar.consumed && !speciesTouchOwned && touchTap >= 6000");
    has(gen45, "down = HidNpadButton_A;   // the shared commit below, on a focus that is now visible");
    // The Gen III / IV-V picker draws still start their modal layer.
    has(gen3, "void drawValuePicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {\n"
              "    if (!state.valuePicker || state.pickerValues.empty()) return;\n"
              "    SharedEditorChrome::beginModal(screen.touchButtons);");
    // Unsupported held items never reach a grid: each generation builds its list from its native
    // held-item set (Gen II selectableHeldItem, Gen III releasedHeldItem / PKHeX Unreleased, Gen V
    // ItemStorage5 without Unreleased).
    has(read("include/UI/Gen2PokemonEditorRules.h"), "return Gen2::selectableHeldItem(item);");
    has(gen3, "if (Gen3::releasedHeldItem(static_cast<uint16_t>(id)) &&");
    has(read("include/Integration/Gen5/Gen5EditorFields.h"), "if (!detail5::contains(BagCatalog5::kUnreleased, id) &&");
    std::cout << "Gen II / III / IV-V surfaces use the shared step ok\n";
}
} // namespace

int main() {
    testTapsCommitTheDrawnItem();
    testAMatchesTap();
    testSearchedList();
    testNonTaps();
    testPartlyVisibleRow();
    testSurfacesUseTheSharedStep();
    std::cout << "test_held_item_input: all passed\n";
    return 0;
}
