#pragma once

#include <cstdlib>

// Round 5: touch paging for the shared paged Review (SharedEditorChrome::drawReviewPanel). Rows
// are informational, so a tap never selects or acts; a completed vertical swipe that starts and
// ends on the panel turns one page, the touch counterpart of L/R. Sideways or short drags, and a
// swipe that leaves the panel, do nothing (N03: touch scrolling and joystick focus coexist).
namespace PokeBank::UIModel::SharedReviewTouch {

struct Rect {
    int x, y, w, h;
    constexpr bool contains(int px, int py) const noexcept {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

// drawReviewPanel on the 1280x720 screen above the 56px footer: w 1100, h 584, centred.
inline constexpr Rect kPanel{(1280 - 1100) / 2, (720 - 56 - 584) / 2, 1100, 584};
inline constexpr int kMinSwipe = 24;

// -1 previous page, +1 next page, 0 nothing. Dragging up (negative deltaY) shows the next page.
constexpr int swipePage(bool justReleased, bool dragged, int startX, int startY, int endX, int endY,
                        Rect panel = kPanel) noexcept {
    if (!justReleased || !dragged) return 0;
    if (!panel.contains(startX, startY) || !panel.contains(endX, endY)) return 0;
    const int dx = endX - startX, dy = endY - startY;
    const int ady = dy < 0 ? -dy : dy, adx = dx < 0 ? -dx : dx;
    if (ady < kMinSwipe || ady <= adx) return 0;
    return dy < 0 ? 1 : -1;
}

} // namespace PokeBank::UIModel::SharedReviewTouch
