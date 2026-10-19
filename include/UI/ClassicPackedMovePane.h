#pragma once

// QA Part 9 (Gen IV Move / Y in the Storage screen's GAME SAVE pane): the classic packed-move
// carry (UI/ClassicPackedMoveOverlay.inc) runs on two grids that show the same staged Gen IV
// boxes: the Boxes tab and the Storage save pane. Each has its own cursor. The pane a pickup or
// selection starts in owns the cursor until the carry ends, so a drop lands at the cursor the
// player is looking at. Before this, a Storage pickup dropped at the hidden Boxes-tab cursor.
namespace PokeBank::UIModel::ClassicPackedMove {

struct GridCursor {
    int box = 0;
    int slot = 0;
};

// The Storage save pane is a packed-move grid only for a Gen IV source, while it is focused.
constexpr bool storageSavePaneOwns(bool gen4Source, bool detailView, bool storageMode, int focusPane) noexcept {
    return gen4Source && detailView && storageMode && focusPane == 0;
}

// The cursor a carry started in a pane reads and writes (pickup source, resize, drop target).
constexpr GridCursor paneCursor(bool storagePane, GridCursor storageSave, GridCursor boxesTab) noexcept {
    return storagePane ? storageSave : boxesTab;
}

static_assert(storageSavePaneOwns(true, true, true, 0));
static_assert(!storageSavePaneOwns(true, true, true, 1));   // Bank pane: legacy storage flow
static_assert(!storageSavePaneOwns(false, true, true, 0));  // Gen I-III keep their Storage flow
static_assert(paneCursor(true, {3, 7}, {0, 1}).box == 3 && paneCursor(true, {3, 7}, {0, 1}).slot == 7);
static_assert(paneCursor(false, {3, 7}, {0, 1}).box == 0 && paneCursor(false, {3, 7}, {0, 1}).slot == 1);

} // namespace PokeBank::UIModel::ClassicPackedMove
