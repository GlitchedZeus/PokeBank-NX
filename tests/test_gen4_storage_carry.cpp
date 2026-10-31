// QA Round 3 Part 9 item 8: the Gen IV Storage view's game-save pane uses the SAME shared Y
// carry as the Boxes view.
//   * with a staged Gen IV editor, the save pane owns Y (Move / Hold Multi) instead of the
//     base Storage handler stripping it as a read-only source key; the Bank pane, a missing
//     staged editor, an open modal or a legacy carry keep the old behaviour;
//   * a pickup at stSaveBox / stSaveSlot moves, drops and is drawn in that pane: the cursor
//     steps inside the pane's grid and never changes pane, the drop destination is the save
//     pane cursor, the source cell renders empty, and the shared carried sprite and footer
//     are drawn on the pane's own cells;
//   * Y tap / hold multi-select and touch drag reuse the Boxes code path, with the pane recorded
//     when the interaction begins.
#include "UI/ClassicCarryGrid.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace G = PokeBank::UIModel::ClassicCarryGrid;

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "missing " << path << '\n';
        std::exit(EXIT_FAILURE);
    }
    return {std::istreambuf_iterator<char>(in), {}};
}

std::string between(const std::string& text, const std::string& from, const std::string& to) {
    const auto a = text.find(from);
    if (a == std::string::npos) return {};
    const auto b = text.find(to, a + from.size());
    return text.substr(a, b == std::string::npos ? std::string::npos : b - a);
}

} // namespace

int main() {
    // Who owns Y and the carry in the Storage view.
    const G::StorageSurface ready{true, true, true, true, false, false, 4, 12};
    check(G::storageOwnsCarry(ready), "staged Gen IV save pane owns the carry");
    auto s = ready;
    s.saveFocused = false;
    check(!G::storageOwnsCarry(s), "the Bank pane never does");
    s = ready;
    s.stagedEditor = false;
    check(!G::storageOwnsCarry(s), "no staged editor: the save stays read-only");
    s = ready;
    s.gen4 = false;
    check(!G::storageOwnsCarry(s), "other generations keep the base Storage handler");
    s = ready;
    s.modalOpen = true;
    check(!G::storageOwnsCarry(s), "an open sheet / confirm / picker keeps its input");
    s = ready;
    s.legacyCarry = true;
    check(!G::storageOwnsCarry(s), "a base Storage carry is never interrupted");
    s = ready;
    s.saveSlot = -1;
    check(!G::storageOwnsCarry(s), "the box-name header is not a slot");
    s = ready;
    s.saveBox = 18;
    check(!G::storageOwnsCarry(s), "Gen IV has 18 boxes");
    s = ready;
    s.storageView = false;
    check(!G::storageOwnsCarry(s), "only inside the entered Storage view");

    // Carry navigation inside the pane: grid moves, box wrap, never off the grid.
    G::Keys none;
    check(G::step({4, 12}, none, 18, 30) == G::Cursor{4, 12}, "no key, no move");
    G::Keys k;
    k.right = true;
    check(G::step({4, 12}, k, 18, 30) == G::Cursor{4, 13}, "Right moves one cell");
    check(G::step({4, 17}, k, 18, 30) == G::Cursor{4, 17}, "Right stops at the row edge");
    k = {};
    k.down = true;
    check(G::step({4, 12}, k, 18, 30) == G::Cursor{4, 18}, "Down moves one row");
    check(G::step({4, 27}, k, 18, 30) == G::Cursor{4, 27}, "Down stops at the last row");
    k = {};
    k.up = true;
    check(G::step({4, 3}, k, 18, 30) == G::Cursor{4, 3}, "Up never leaves the grid for the header");
    k = {};
    k.nextBox = true;
    check(G::step({17, 5}, k, 18, 30) == G::Cursor{0, 5}, "R wraps to Box 1, slot kept");
    k = {};
    k.previousBox = true;
    check(G::step({0, 5}, k, 18, 30) == G::Cursor{17, 5}, "L wraps to Box 18");
    G::Cursor c{4, 12};
    for (int i = 0; i < 100; ++i) {
        G::Keys any;
        any.up = i % 4 == 0;
        any.down = i % 4 == 1;
        any.left = i % 4 == 2;
        any.right = i % 4 == 3;
        any.nextBox = i % 7 == 0;
        c = G::step(c, any, 18, 30);
        check(c.box >= 0 && c.box < 18 && c.slot >= 0 && c.slot < 30, "cursor stays on a real save-pane slot");
    }

    // Device wiring (ClassicPackedMoveOverlay / StoragePanel / composite update).
    const auto overlay = read("src/UI/ClassicPackedMoveOverlay.inc");
    check(overlay.find("return CarryGrid::storageOwnsCarry(surface);") != std::string::npos &&
              overlay.find("return boxesGridOwnsInput(screen) || storageGridOwnsInput(screen);") != std::string::npos,
          "the shared carry owns the Storage save pane through the model");
    check(overlay.find("return storageMode(screen, state) ? screen.stSaveBox : screen.selectedBoxIndex;") !=
                  std::string::npos &&
              overlay.find("return storageMode(screen, state) ? screen.stSaveSlot : screen.selectedItemIndex;") !=
                  std::string::npos,
          "the carry cursor is the save-pane cursor in Storage");
    const auto placeGen4 = between(overlay, "bool placeGen4(", "\n// Gen V drop:");
    check(placeGen4.find("const int destinationBox = cursorBox(screen, state);") != std::string::npos &&
              placeGen4.find("const int destinationSlot = cursorSlot(screen, state);") != std::string::npos &&
              placeGen4.find("screen.selectedBoxIndex") == std::string::npos,
          "a Gen IV drop lands at the cursor of the pane it came from");
    const auto cancel = between(overlay, "bool cancel(", "\nbool placeGen4(");
    check(cancel.find("cursorBox(screen, state) = state.sourceBox;") != std::string::npos,
          "B cancel returns the cursor to the source in its own pane");
    const auto input = between(overlay, "bool handleInput(TrainerViewScreen& screen, uint64_t down", "\nvoid draw(");
    check(input.find("state.storage = storageGridOwnsInput(screen);\n    state.yPending = true;") != std::string::npos &&
              input.find("state.storage = storageGridOwnsInput(screen);\n                state.touchArmed = true;") !=
                  std::string::npos,
          "Y tap / hold and touch drag record the pane when they begin");
    check(input.find("if (state.storage) return storageCarryNavigation(") != std::string::npos &&
              overlay.find("CarryGrid::step({screen.stSaveBox, screen.stSaveSlot}, keys, boxes") != std::string::npos,
          "a Storage carry moves inside its pane only");
    check(input.find("screen.selectedItemIndex") == std::string::npos &&
              input.find("screen.selectedBoxIndex") == std::string::npos,
          "shared carry input never reads the Boxes cursor directly");
    check(overlay.find("state.storage = screen.selectedMode == TrainerViewScreen::ViewMode::Storage &&") !=
              std::string::npos,
          "Actions -> Move from the save pane carries within that pane");
    const auto draw = between(overlay, "void draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {", "\n} // namespace UI");
    check(draw.find("const auto carryMode = state.storage ? TrainerViewScreen::ViewMode::Storage") != std::string::npos &&
              draw.find("cellRect(screen, state, static_cast<int>(rectangle[i]))") != std::string::npos,
          "the carried sprite, footer and multi-select rectangle draw on the save pane's cells");
    check(overlay.find("if (t.pane == 0 && t.box == screen.stSaveBox && t.slot == slot)") != std::string::npos,
          "save-pane cells come from the pane's own storage touch targets");
    const auto panel = read("src/UI/Panels/StoragePanel.cpp");
    check(panel.find("if (savePane && ClassicPackedMove::hidesGen4CarriedSource(screen, boxIndex, i))") !=
              std::string::npos,
          "the save pane hides the carried source cell, as Boxes does");
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    check(composite.find("!carrying() && !currentlySelecting && !swapActive && !releaseConfirmActive &&") !=
              std::string::npos,
          "a tap while carrying moves the carry instead of opening Actions");
    check(composite.find("ClassicPackedMove::handleInput(*this, down, held, stick.x, stick.y, touch)") <
              composite.find("updateGSCOverlay(pad, touch);"),
          "the carry sees Y before the base Storage handler could strip it");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Gen IV Storage save-pane Move / Y carry (Part 9 item 8, " << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
