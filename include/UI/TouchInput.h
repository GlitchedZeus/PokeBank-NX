#ifndef UI_TOUCH_INPUT_H
#define UI_TOUCH_INPUT_H

#include <switch.h>

namespace UI {
    /**
     * Reads the Switch touchscreen once per frame and exposes a small gesture state machine.
     * Coordinates are framebuffer pixels: PokeBank renders at 1280x720, matching the Switch touch
     * panel, so touch points map 1:1 with no scaling.
     *
     * UI activation uses a release-confirmed tap rather than raw touch-down. This is intentional:
     * landing a finger on a row/card must not activate it before the user has had a chance to turn
     * the gesture into a swipe. Code that genuinely needs the physical touch-down edge can use
     * justTouchedDown(). Existing justPressed() call sites therefore become proper tap actions
     * without duplicating screen-specific input paths.
     */
    class TouchInput {
    public:
        /// Poll the touchscreen; call once per frame after hidInitializeTouchScreen().
        void update();

        bool isDown() const { return curDown; }                         // a finger is currently on the screen
        bool justTouchedDown() const { return curDown && !prevDown; }   // raw physical contact edge
        bool justReleased() const { return !curDown && prevDown; }      // physical release edge

        /**
         * Confirmed tap edge used by normal UI controls.
         *
         * Historically this method returned the raw touch-down edge, which made the UI feel janky:
         * a list row or button could fire before a swipe had even started. Keep the existing method
         * name so every established hitbox automatically gets the safer behavior, but only report a
         * press once the finger has been released without crossing the drag threshold.
         */
        bool justPressed() const { return justReleased() && !dragged(); }
        bool justTapped() const { return justPressed(); }

        int x() const { return curX; }                                  // current (or last) touch position
        int y() const { return curY; }
        int startX() const { return begX; }                             // where the current/last touch began
        int startY() const { return begY; }
        int deltaX() const { return curX - begX; }
        int deltaY() const { return curY - begY; }

        bool dragged() const;                                           // moved past the tap-cancel threshold

    private:
        bool curDown = false, prevDown = false;
        int curX = 0, curY = 0;   // latest touch position
        int begX = 0, begY = 0;   // position where the current touch started (recorded on contact)
    };
}

#endif
