#pragma once

// ClassicCarryRules.h - the ONE rule set of the Gen I-V box carry (Round 11 H23 / H31 / H32).
//
// The shared carry layer (src/UI/ClassicPackedMoveOverlay.inc) runs Move and Clone for every
// generation: Gen I-IV through handleInput, Gen V through handleGen5CarryInput, and one draw().
// Both key dispatch sites, the carry footer, the status lines and the host tests read this pure
// model, so the generations cannot drift apart again.
//
//  * Keys while carrying (H23, owner): Y picks up (a Y tap / hold outside this model), A places,
//    B cancels, + cancels and lets the key reach the grid (Review). Y and X are refused with a hint
//    and CONSUMED: a Y that fell through would reach Gen V Review or a Gen I-IV editor handler during
//    a live carry.
//  * Clone (H31): Actions -> Clone picks up a ghost copy of the Pokemon and returns to the box. Nothing
//    is staged until A confirms an EMPTY destination; an occupied or protected slot is refused with a
//    clear status and the carry stays (never overwritten); B cancels with no change. Gen I / II boxes
//    are packed lists, so any empty cell of the chosen box lands at that box's next free slot.

#include <cstdint>

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
enum class Key : unsigned char { None, Cancel, Place, Leave, RefuseHint };

constexpr Key carryKey(std::uint64_t down) noexcept {
    if (down & kButtonB) return Key::Cancel;          // B wins: cancelling never stages anything
    if (down & kButtonA) return Key::Place;
    if (down & kButtonPlus) return Key::Leave;
    if (down & (kButtonY | kButtonX)) return Key::RefuseHint;
    return Key::None;
}

// The status a refused Y / X shows (the key is consumed, nothing else happens).
constexpr const char* refuseHint(Intent intent) noexcept {
    return intent == Intent::Clone ? "A places the clone in an empty slot, B cancels"
                                   : "A places it here, B cancels";
}

// The status shown when Actions -> Clone picks the copy up.
inline constexpr const char* kClonePickupStatus =
    "Clone: choose an empty slot, A places the copy, B cancels (nothing staged yet)";

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

constexpr const char* dropStatus(Drop outcome) noexcept {
    switch (outcome) {
        case Drop::Occupied: return "That slot is occupied - nothing was overwritten. Choose an empty slot, or B cancels";
        case Drop::Protected: return "That slot is protected and cannot take a clone - choose another empty slot, or B cancels";
        case Drop::BoxFull: return "This box is full - choose another box with L/R, or B cancels";
        case Drop::NoSlot: return "Choose a box slot for the clone, or B cancels";
        case Drop::Accept: break;
    }
    return "";
}

inline constexpr const char* kCloneCancelledStatus = "Clone cancelled - nothing was staged";
inline constexpr const char* kCloneSourceChangedStatus =
    "The staged save changed while the clone was carried - nothing was staged";

static_assert(carryKey(kButtonA) == Key::Place && carryKey(kButtonB) == Key::Cancel);
static_assert(carryKey(kButtonA | kButtonB) == Key::Cancel, "B wins over A");
static_assert(carryKey(kButtonY) == Key::RefuseHint && carryKey(kButtonX) == Key::RefuseHint);
static_assert(carryKey(kButtonPlus) == Key::Leave && carryKey(0) == Key::None);
static_assert(decideCloneDrop({true, 20, 3, 7, false, false}).slot == 3, "packed: next free slot");
static_assert(decideCloneDrop({true, 20, 3, 1, true, false}).outcome == Drop::Occupied);
static_assert(decideCloneDrop({true, 20, 20, 5, true, false}).outcome == Drop::BoxFull);
static_assert(decideCloneDrop({false, 30, 0, 12, false, false}).slot == 12, "sparse: the hovered slot");
static_assert(decideCloneDrop({false, 30, 0, 12, false, true}).outcome == Drop::Protected);

} // namespace PokeBank::UIModel::ClassicCarry
