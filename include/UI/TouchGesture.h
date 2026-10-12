#pragma once

namespace UI {

// Lightweight renderer-facing touch snapshot. Keep this header platform-neutral so draw-only helpers
// and host contracts can observe direct-manipulation state without pulling libnx/switch.h into
// otherwise portable UI/model tests.
struct TouchGestureSnapshot {
    bool down = false;
    int startX = 0;
    int startY = 0;
    int deltaX = 0;
    int deltaY = 0;
};

// A sideways drag is not a Species-preview scroll, even if the finger has a
// vertical component. Below tap slop, preserve natural pixel-following motion.
[[nodiscard]] inline bool permitsVerticalPreview(const TouchGestureSnapshot& gesture) noexcept {
    constexpr int kTapSlop = 22;
    const int ax = gesture.deltaX < 0 ? -gesture.deltaX : gesture.deltaX;
    const int ay = gesture.deltaY < 0 ? -gesture.deltaY : gesture.deltaY;
    return ax <= kTapSlop || ay >= ax;
}

// Latest touch state for draw-time direct-manipulation effects. Input still owns activation;
// renderers may only use this snapshot to move visible content with the finger.
const TouchGestureSnapshot& latestTouchGesture() noexcept;

} // namespace UI
