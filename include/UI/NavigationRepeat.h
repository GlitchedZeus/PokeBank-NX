#ifndef UI_NAVIGATION_REPEAT_H
#define UI_NAVIGATION_REPEAT_H

#include <cstdint>

namespace PokeBank::UIModel {
    // Frame-based navigation repeat for controller UIs. libnx exposes both D-pad and left-stick
    // directions through the same digital button bits, so screens get precise taps and comfortable
    // held scrolling without maintaining two navigation paths.
    class NavigationRepeat {
    public:
        static constexpr int INITIAL_DELAY_FRAMES = 18;
        static constexpr int REPEAT_INTERVAL_FRAMES = 4;

        std::uint64_t apply(std::uint64_t pressed, std::uint64_t held,
                            std::uint64_t repeatableMask) {
            const std::uint64_t repeatableHeld = held & repeatableMask;
            if (repeatableHeld == 0 || repeatableHeld != heldDirections) {
                heldDirections = repeatableHeld;
                heldFrames = 0;
                return pressed;
            }

            ++heldFrames;
            if (heldFrames >= INITIAL_DELAY_FRAMES &&
                (heldFrames - INITIAL_DELAY_FRAMES) % REPEAT_INTERVAL_FRAMES == 0) {
                return pressed | repeatableHeld;
            }
            return pressed;
        }

        void reset() {
            heldDirections = 0;
            heldFrames = 0;
        }

    private:
        std::uint64_t heldDirections = 0;
        int heldFrames = 0;
    };
}

#endif
