#pragma once

// ClassicCarryRules.h - the ONE rule set of the Gen I-V box carry (Round 11 H23 / H31 / H32, Round 12
// R12-001 / R12-002 / R12-003).
//
// The shared carry layer (src/UI/ClassicPackedMoveOverlay.inc) runs Move and Clone for every
// generation: Gen I-IV through handleInput, Gen V through handleGen5BoxTouch / handleGen5BoxInput, and
// one draw(). Touch (Round 13 owner decision 4) shares the same selection and carry (carryTap below).
// Both key dispatch sites, the carry footer and the host tests read this pure model, so the
// generations cannot drift apart again.
//
//  * Keys (H23, R12-001, owner): on every generation's Boxes grid a Y tap picks up to Move and a Y
//    hold starts multi-select; while carrying, A places, B cancels, + cancels and lets the key reach
//    the grid (Review, the same + key in Gen I-V). Y and X are CONSUMED: a Y that fell through would
//    reach a Gen I-IV editor handler during a live carry.
//  * Feedback (R12-002, owner Round 12 H03 / H04): the footer names the carry keys, so a routine step
//    (pick up, place, multi-move, clone placed, cancel, a consumed key) posts no toast
//    (EditFeedback::Kind::Carry). What still posts: "There is no Pokemon in this slot to move", a
//    group the destination has no room for, occupied / protected / source-changed refusals and
//    failure diagnostics.
//  * Clone (H31): Actions -> Clone picks up a ghost copy of the Pokemon and returns to the box. Nothing
//    is staged until A confirms an EMPTY destination; an occupied or protected slot is refused with a
//    clear status and the carry stays (never overwritten); B cancels with no change.
//  * Packed boxes (R12-003, owner Round 12 H04): Gen I / II boxes are packed lists, so the carry has
//    ONE focus - the cursor itself is the landing position (packedFocusSlot). A clone always sits at
//    the end of the box being viewed (L/R changes box; it never drifts into another box), a full box
//    is refused, and a Move's cursor stays on the insertion points 0..count.

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace PokeBank::UIModel::ClassicCarry {

// libnx HidNpadButton bits, restated so host tests need no libnx (the overlay static_asserts them).
inline constexpr std::uint64_t kButtonA = 1ull << 0;
inline constexpr std::uint64_t kButtonB = 1ull << 1;
inline constexpr std::uint64_t kButtonX = 1ull << 2;
inline constexpr std::uint64_t kButtonY = 1ull << 3;
inline constexpr std::uint64_t kButtonPlus = 1ull << 10;

enum class Intent : unsigned char { Move, Clone };

// What a pressed key does while a Pokemon (or a clone) is carried. D-pad, stick, L / R and ZL / ZR
// are None: they keep moving the cursor through the accepted box navigation path.
// Consume: Y / X do nothing during a carry and never fall through (R12-002: no hint toast either,
// the footer already names A / L/R / B).
enum class Key : unsigned char { None, Cancel, Place, Leave, Consume };

constexpr Key carryKey(std::uint64_t down) noexcept {
    if (down & kButtonB) return Key::Cancel;          // B wins: cancelling never stages anything
    if (down & kButtonA) return Key::Place;
    if (down & kButtonPlus) return Key::Leave;
    if (down & (kButtonY | kButtonX)) return Key::Consume;
    return Key::None;
}

struct FooterHint {
    const char* glyph;
    const char* label;
};
struct Footer {
    FooterHint hints[3];
};

// The carry footer: A places, L/R changes box, B cancels. Tapping the A glyph sends A (navTouchButton),
// so touch placement uses the same path.
constexpr Footer carryFooter(Intent intent, bool group) noexcept {
    const char* place = intent == Intent::Clone ? "Place Clone" : group ? "Place Group" : "Place";
    return {{{"A", place}, {"L/R", "Box"}, {"B", "Cancel"}}};
}

// ---- Touch (Round 13 owner decision 4: touchscreen drag multi-select with controller parity) -------
// A finger drag across box cells selects the rectangle a Y hold + D-pad selects, and lifting the finger
// picks the group up as releasing Y does (ClassicPackedMoveOverlay touchSelectionInput). While carrying,
// a release-confirmed tap (TouchInput::justTapped) is one of the carry keys (owner, Nov 2: the same
// rule in Gen I-V):
//   * on a grid cell: the controller's "move the cursor there + A" - the cursor moves to that cell and
//     the same place() transaction runs, with the same capacity / occupied / protected refusals (the
//     carry stays) and the same silent success;
//   * outside every published touch target, above the footer: B - the same silent cancel;
//   * anything else (box arrows, header, summary panel, footer badges, any button): None, the target
//     keeps its own meaning. A drag is never a tap, and a tap whose touch-down the carry grid did not
//     own (it began under Help, a menu or a dialog, or closed one) is None too.
enum class Tap : unsigned char { None, Place, Cancel };

constexpr Tap carryTap(int tappedSlot, int capacity, bool outsideTargets) noexcept {
    if (tappedSlot >= 0 && tappedSlot < capacity) return Tap::Place;
    return outsideTargets ? Tap::Cancel : Tap::None;
}

// ---- Clone destination (H31 / H32) ----------------------------------------------------------------

enum class Drop : unsigned char { Accept, Occupied, Protected, BoxFull, NoSlot };

struct CloneTarget {
    bool packed = false;          // Gen I / II: a box is a packed list of `count` Pokemon
    int capacity = 0;             // slots in the box
    int count = 0;                // packed only: Pokemon in the box (= its next free slot)
    int slot = -1;                // the hovered slot
    bool occupied = false;        // the hovered slot holds a Pokemon
    bool protectedSlot = false;   // unreadable, already staged, or not a native blank: never written
};

struct CloneDecision {
    Drop outcome = Drop::NoSlot;
    int slot = -1;                // where the clone lands when outcome == Accept
};

constexpr CloneDecision decideCloneDrop(const CloneTarget& t) noexcept {
    if (t.capacity <= 0 || t.slot < 0 || t.slot >= t.capacity) return {Drop::NoSlot, -1};
    if (t.packed) {
        if (t.count >= t.capacity) return {Drop::BoxFull, -1};
        if (t.occupied || t.slot < t.count) return {Drop::Occupied, -1};
        if (t.protectedSlot) return {Drop::Protected, -1};
        return {Drop::Accept, t.count};   // packed list: the next free slot of this box
    }
    if (t.occupied) return {Drop::Occupied, -1};
    if (t.protectedSlot) return {Drop::Protected, -1};
    return {Drop::Accept, t.slot};
}

// Where the ghost copy is drawn: the slot it would land in, or the hovered cell when refused.
constexpr int ghostSlot(const CloneTarget& t) noexcept {
    const auto decision = decideCloneDrop(t);
    return decision.outcome == Drop::Accept ? decision.slot : t.slot;
}

constexpr const char* dropStatus(Drop outcome, bool packed = false) noexcept {
    if (packed && outcome == Drop::Occupied)
        return "Gen I/II boxes have no gaps - the clone is added at the end of the box. Move to an empty slot, or B cancels";
    switch (outcome) {
        case Drop::Occupied: return "That slot is occupied - nothing was overwritten. Choose an empty slot, or B cancels";
        case Drop::Protected: return "That slot is protected and cannot take a clone - choose another empty slot, or B cancels";
        case Drop::BoxFull: return "This box is full - choose another box with L/R, or B cancels";
        case Drop::NoSlot: return "Choose a box slot for the clone, or B cancels";
        case Drop::Accept: break;
    }
    return "";
}

inline constexpr const char* kCloneSourceChangedStatus =
    "The staged save changed while the clone was carried - nothing was staged";

// ---- Packed boxes: one focus (R12-003) ------------------------------------------------------------
// Gen I / II boxes are packed lists of `count` Pokemon. While a carry is over such a box the cursor IS
// the landing position, so no second indicator (a ghost at the box end while the cursor roams the empty
// cells, or a sprite on a cell the Pokemon would never land in) can appear:
//   * Clone: the end of the box (its next free slot); on a full box the last cell, where A is refused;
//   * Move:  an insertion point 0..count (the backend inserts at min(slot, count)).
constexpr int packedFocusSlot(Intent intent, int count, int capacity, int slot) noexcept {
    if (capacity <= 0) return -1;
    if (count < 0) count = 0;
    const int end = count < capacity ? count : capacity - 1;
    if (intent == Intent::Clone) return end;
    return slot < 0 ? 0 : slot > end ? end : slot;
}

// ---- Sparse groups (Gen III-V) ----------------------------------------------------------------------
// A carried group keeps its exact shape; the cursor is the group's top-left cell. Fills `out` with
// (source slot, destination slot) pairs in source order; false when the shape runs off the
// columns x rows box (the caller refuses: the group does not fit there).
inline bool sparseGroupLanding(const std::vector<std::size_t>& sources, int destination,
                               std::vector<std::pair<std::size_t, std::size_t>>& out,
                               int columns = 6, int rows = 5) {
    out.clear();
    if (sources.empty() || columns <= 0 || rows <= 0 || destination < 0 || destination >= columns * rows)
        return false;
    int minRow = rows, minCol = columns, maxRow = 0, maxCol = 0;
    for (const auto source : sources) {
        const int row = static_cast<int>(source) / columns, col = static_cast<int>(source) % columns;
        minRow = row < minRow ? row : minRow;
        minCol = col < minCol ? col : minCol;
        maxRow = row > maxRow ? row : maxRow;
        maxCol = col > maxCol ? col : maxCol;
    }
    const int destRow = destination / columns, destCol = destination % columns;
    if (destRow + maxRow - minRow >= rows || destCol + maxCol - minCol >= columns) return false;
    for (const auto source : sources) {
        const int row = destRow + static_cast<int>(source) / columns - minRow;
        const int col = destCol + static_cast<int>(source) % columns - minCol;
        out.emplace_back(source, static_cast<std::size_t>(row * columns + col));
    }
    return true;
}

static_assert(carryKey(kButtonA) == Key::Place && carryKey(kButtonB) == Key::Cancel);
static_assert(carryKey(kButtonA | kButtonB) == Key::Cancel, "B wins over A");
static_assert(carryKey(kButtonY) == Key::Consume && carryKey(kButtonX) == Key::Consume);
static_assert(carryKey(kButtonPlus) == Key::Leave && carryKey(0) == Key::None);
static_assert(decideCloneDrop({true, 20, 3, 7, false, false}).slot == 3, "packed: next free slot");
static_assert(decideCloneDrop({true, 20, 3, 1, true, false}).outcome == Drop::Occupied);
static_assert(decideCloneDrop({true, 20, 20, 5, true, false}).outcome == Drop::BoxFull);
static_assert(decideCloneDrop({false, 30, 0, 12, false, false}).slot == 12, "sparse: the hovered slot");
// Packed boxes: any empty cell the cursor is on shows (and places) the copy at the box's end only.
static_assert(ghostSlot({true, 20, 3, 15, false, false}) == 3 && ghostSlot({true, 20, 3, 4, false, false}) == 3,
              "packed: the ghost never advertises the hovered empty position");
static_assert(decideCloneDrop({false, 30, 0, 12, false, true}).outcome == Drop::Protected);
static_assert(packedFocusSlot(Intent::Clone, 3, 20, 15) == 3 && packedFocusSlot(Intent::Clone, 20, 20, 2) == 19,
              "packed Clone: the cursor is the box end (the last cell when full)");
static_assert(packedFocusSlot(Intent::Move, 3, 20, 1) == 1 && packedFocusSlot(Intent::Move, 3, 20, 9) == 3,
              "packed Move: the cursor stays on an insertion point 0..count");

} // namespace PokeBank::UIModel::ClassicCarry
