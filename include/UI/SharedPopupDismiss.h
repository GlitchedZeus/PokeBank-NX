#pragma once
// Round 6 7D: a tap outside a non-destructive transient editor picker (Held Item, Species, Ball,
// Ability, Met Location, Language, Nature, Gender, Form, Move) closes it exactly like B.
//
// * Only a picker opts in: its draw publishes its frame right after beginModal. beginModal clears
//   the frame first (SharedEditorChrome.h), so a confirmation or any other modal (Discard All,
//   Delete Backup, Export, Keep / Discard) never has one and is never dismissed by a stray tap.
// * The dismissal is release-confirmed (TouchInput::justTapped) and both the touch-down and the
//   release lie outside the frame and above the footer band (the footer's own badges keep their
//   meaning). The picker stays open, and its handler keeps consuming the gesture, until that
//   release; the handler returns on the dismissing frame, so the touch never reaches the screen
//   underneath (no tap-through).
// Platform-neutral: host tests drive it with tests/touch_fakes.

namespace UI::SharedPopupDismiss {

struct Frame {
    int x = 0, y = 0, w = 0, h = 0;
    bool valid = false;
    constexpr bool contains(int px, int py) const noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

inline constexpr int kNavBarHeight = 56;                // ScreenChrome kNavBarH
inline constexpr int kFooterTop = 720 - kNavBarHeight;  // the footer band

inline Frame& published() noexcept {
    static Frame frame;
    return frame;
}
inline void clear() noexcept { published() = {}; }
inline void publish(int x, int y, int w, int h) noexcept { published() = {x, y, w, h, true}; }

template <class Touch>
bool tappedOutside(const Touch& touch, const Frame& frame) noexcept {
    if (!frame.valid || !touch.justTapped()) return false;
    return !frame.contains(touch.startX(), touch.startY()) && !frame.contains(touch.x(), touch.y()) &&
           touch.startY() < kFooterTop && touch.y() < kFooterTop;
}
template <class Touch>
bool tappedOutside(const Touch& touch) noexcept { return tappedOutside(touch, published()); }

} // namespace UI::SharedPopupDismiss
