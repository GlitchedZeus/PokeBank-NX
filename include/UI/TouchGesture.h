#pragma once

namespace UI {

// Lightweight renderer-facing touch snapshot. Keep this header platform-neutral so draw-only helpers
// and host contracts can observe direct-manipulation state without pulling libnx/switch.h into
// otherwise portable UI/model tests.
struct TouchGestureSnapshot {
    bool down = false;
    bool dragged = false;
    int x = 0;
    int y = 0;
    int startX = 0;
    int startY = 0;
    int deltaX = 0;
    int deltaY = 0;
};

// Latest touch state for draw-time direct-manipulation effects. Input still owns activation;
// renderers may only use this snapshot to move visible content with the finger.
const TouchGestureSnapshot& latestTouchGesture() noexcept;

} // namespace UI
