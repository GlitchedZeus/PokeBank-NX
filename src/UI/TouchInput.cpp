#include "UI/TouchInput.h"

namespace UI {
    void TouchInput::update() {
        prevDown = curDown;

        HidTouchScreenState st{};
        if (hidGetTouchScreenStates(&st, 1) > 0 && st.count > 0) {
            curX = static_cast<int>(st.touches[0].x);
            curY = static_cast<int>(st.touches[0].y);
            // Record the start position on the physical contact edge (curDown still contains the
            // previous frame here). The final coordinates are retained after release so release-
            // confirmed taps and swipes can resolve against the same 1280x720 geometry.
            if (!curDown) { begX = curX; begY = curY; }
            curDown = true;
        } else {
            curDown = false;
        }
    }

    bool TouchInput::dragged() const {
        const int dx = curX - begX, dy = curY - begY;
        // A fingertip naturally wanders a little even during a deliberate tap. Twenty pixels was
        // twitchy on hardware and cancelled legitimate taps near row/card edges. Twenty-eight keeps
        // taps forgiving while remaining well below the larger thresholds used by real swipe actions.
        constexpr int kTapSlop = 28;
        return (dx * dx + dy * dy) > (kTapSlop * kTapSlop);
    }
}
