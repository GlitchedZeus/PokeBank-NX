#include "UI/TouchInput.h"

#include <algorithm>

namespace UI {
    namespace {
        TouchGestureSnapshot gLatestTouch{};
    }

    const TouchGestureSnapshot& latestTouchGesture() noexcept {
        return gLatestTouch;
    }

    void TouchInput::update() {
        prevDown = curDown;

        HidTouchScreenState st{};
        if (hidGetTouchScreenStates(&st, 1) > 0 && st.count > 0) {
            curX = static_cast<int>(st.touches[0].x);
            curY = static_cast<int>(st.touches[0].y);

            // Record a fresh gesture on the physical contact edge. While the finger remains down,
            // remember the furthest point it ever reached from that origin. Using the maximum rather
            // than only the release coordinate prevents a swipe-out-and-back gesture from becoming an
            // accidental tap when the finger returns close to where it started.
            if (!curDown) {
                begX = curX;
                begY = curY;
                maxDistanceSquared = 0;
            } else {
                const int dx = curX - begX;
                const int dy = curY - begY;
                maxDistanceSquared = std::max(maxDistanceSquared, dx * dx + dy * dy);
            }
            curDown = true;
        } else {
            // Keep the final coordinates and maximum displacement after release so the release frame
            // can classify the completed gesture consistently.
            curDown = false;
        }

        gLatestTouch.down = curDown;
        gLatestTouch.dragged = dragged();
        gLatestTouch.x = curX;
        gLatestTouch.y = curY;
        gLatestTouch.startX = begX;
        gLatestTouch.startY = begY;
        gLatestTouch.deltaX = curX - begX;
        gLatestTouch.deltaY = curY - begY;
    }

    bool TouchInput::dragged() const {
        // A 22 px dead-zone absorbs normal fingertip wobble on the 1280x720 Switch panel while still
        // handing deliberate swipes to the browser/list gesture paths well before their 52-72 px step
        // thresholds. Once the gesture ever crosses this boundary it stays a drag until release.
        constexpr int kTapSlop = 22;
        return maxDistanceSquared > (kTapSlop * kTapSlop);
    }
}
