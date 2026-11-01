#pragma once

// ClassicCarryGrid.h - which grid the shared classic box carry runs on, and how its cursor moves
// in the Gen IV Storage view (QA Round 3 Part 9 item 8).
//
// The Storage view's game-save pane shows the SAME staged PK4 boxes as the Boxes view. With a
// staged Gen IV editor it gets the Boxes behaviour: Y taps pick up (Move), a held Y sweeps a
// multi-select, and the pickup, the drop and the drawn carry all stay in the save pane the
// Pokemon came from. Without a staged editor, or on the app-owned Bank pane, nothing changes:
// the source stays read-only there. Pure model: the device overlay (ClassicPackedMoveOverlay)
// feeds it the screen state and applies the result.

#include "UI/ClassicPackedMultiSelect.h"

namespace PokeBank::UIModel::ClassicCarryGrid {

enum class Grid : unsigned char { None, Boxes, StorageSave };

struct StorageSurface {
    bool gen4 = false;            // exact Gen IV source title
    bool storageView = false;     // Storage view is selected and entered
    bool saveFocused = false;     // the game-save pane (pane 0), not the Bank
    bool stagedEditor = false;    // a validated staged Gen IV editor exists
    bool modalOpen = false;       // help, details, a sheet / menu / confirm, a picker...
    bool legacyCarry = false;     // the base Storage view is already holding Pokemon
    int saveBox = 0;
    int saveSlot = 0;
};

// The save pane owns Y and the carry only for a real staged Gen IV editor on a real slot.
constexpr bool storageOwnsCarry(const StorageSurface& s, int boxes = 18, int slots = 30) noexcept {
    return s.gen4 && s.storageView && s.saveFocused && s.stagedEditor && !s.modalOpen && !s.legacyCarry &&
           s.saveBox >= 0 && s.saveBox < boxes && s.saveSlot >= 0 && s.saveSlot < slots;
}

struct Cursor {
    int box = 0;
    int slot = 0;
    constexpr bool operator==(const Cursor&) const noexcept = default;
};

struct Keys {
    bool up = false, down = false, left = false, right = false;  // D-pad / stick (repeat-filtered)
    bool previousBox = false, nextBox = false;                   // L / R or the header arrows
};

// One carry-navigation step inside the save pane. Boxes wrap; the slot moves inside the native
// grid (ClassicPackedMultiSelect::moveCursor). The pane itself never changes, so a drop always
// lands in the pane the carry came from.
constexpr Cursor step(Cursor c, const Keys& k, int boxes, int slots) noexcept {
    namespace Multi = PokeBank::UIModel::ClassicPackedMove;
    if (boxes <= 0 || slots <= 0) return c;
    if (k.previousBox) c.box = (c.box + boxes - 1) % boxes;
    if (k.nextBox) c.box = (c.box + 1) % boxes;
    if (k.up) c.slot = Multi::moveCursor(c.slot, Multi::Direction::Up, slots);
    if (k.down) c.slot = Multi::moveCursor(c.slot, Multi::Direction::Down, slots);
    if (k.left) c.slot = Multi::moveCursor(c.slot, Multi::Direction::Left, slots);
    if (k.right) c.slot = Multi::moveCursor(c.slot, Multi::Direction::Right, slots);
    return c;
}

static_assert(storageOwnsCarry({true, true, true, true, false, false, 3, 7}));
static_assert(!storageOwnsCarry({true, true, false, true, false, false, 3, 7}), "Bank pane never");
static_assert(!storageOwnsCarry({true, true, true, false, false, false, 3, 7}), "read-only without a staged editor");
static_assert(step({0, 0}, {false, false, false, false, true, false}, 18, 30) == Cursor{17, 0});
static_assert(step({17, 29}, {false, true, false, true, false, true}, 18, 30) == Cursor{0, 29});

} // namespace PokeBank::UIModel::ClassicCarryGrid
