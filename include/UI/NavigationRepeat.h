#ifndef UI_NAVIGATION_REPEAT_H
#define UI_NAVIGATION_REPEAT_H

#include <chrono>
#include <cstdint>

namespace PokeBank::UIModel {
    // Shared hold-to-repeat timing for every controller-first screen (P0 #320 phase 8). It is
    // measured in wall-clock milliseconds, not frames: a frame-counted delay stretched with the frame
    // time (18 frames is 300 ms at 60 fps but 900 ms at 20 fps), which felt like "moves once, waits,
    // then moves again" whenever a screen did heavy work. First move is immediate on the press edge,
    // repeat begins after INITIAL_DELAY_MS, then one step every REPEAT_INTERVAL_MS. At most one step
    // is emitted per frame and a late frame never bursts several steps, so nothing is skipped.
    struct RepeatTiming {
        static constexpr std::uint64_t INITIAL_DELAY_MS = 250;
        static constexpr std::uint64_t REPEAT_INTERVAL_MS = 67;
    };

    inline std::uint64_t monotonicMilliseconds() {
        using namespace std::chrono;
        return static_cast<std::uint64_t>(
            duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }

    // The timing gate itself, shared by direction repeat and the keyboard's typing repeat.
    class HoldRepeatClock {
    public:
        void start(std::uint64_t nowMs) { nextMs = nowMs + RepeatTiming::INITIAL_DELAY_MS; }
        // True at most once per call; a stalled frame resumes the cadence instead of bursting.
        bool due(std::uint64_t nowMs) {
            if (nowMs < nextMs) return false;
            nextMs += RepeatTiming::REPEAT_INTERVAL_MS;
            if (nextMs <= nowMs) nextMs = nowMs + RepeatTiming::REPEAT_INTERVAL_MS;
            return true;
        }
    private:
        std::uint64_t nextMs = 0;
    };

    // Navigation repeat for controller UIs. D-pad directions arrive as button bits; libnx exposes
    // the analog sticks separately, so AnalogNavigation below converts the left stick into the same
    // logical directions before it reaches this repeater.
    class NavigationRepeat {
    public:
        std::uint64_t apply(std::uint64_t pressed, std::uint64_t held,
                            std::uint64_t repeatableMask) {
            return apply(pressed, held, repeatableMask, monotonicMilliseconds());
        }

        std::uint64_t apply(std::uint64_t pressed, std::uint64_t held,
                            std::uint64_t repeatableMask, std::uint64_t nowMs) {
            const std::uint64_t repeatableHeld = held & repeatableMask;
            if (repeatableHeld == 0 || repeatableHeld != heldDirections) {
                heldDirections = repeatableHeld;
                if (repeatableHeld != 0) clock.start(nowMs);
                return pressed;
            }
            return clock.due(nowMs) ? (pressed | repeatableHeld) : pressed;
        }

        void reset() { heldDirections = 0; }

    private:
        std::uint64_t heldDirections = 0;
        HoldRepeatClock clock;
    };

    struct AnalogNavigationSample {
        std::uint64_t down = 0;
        std::uint64_t held = 0;
    };

    // Converts a physical left-stick sample into one stable logical direction. Two thresholds give
    // the stick hysteresis: it must move clearly outside the deadzone to press, but may drift closer
    // to centre before release. Dominant-axis locking prevents diagonal noise from alternating rows
    // and columns every frame; a deliberate turn still switches once the new axis clearly wins.
    class AnalogNavigation {
    public:
        static constexpr int PRESS_THRESHOLD = 16000;
        static constexpr int RELEASE_THRESHOLD = 9000;
        static constexpr int AXIS_SWITCH_MARGIN = 4000;

        AnalogNavigationSample sample(int x, int y,
                                      std::uint64_t up, std::uint64_t down,
                                      std::uint64_t left, std::uint64_t right) {
            const std::uint64_t previous = heldDirection;
            const Candidate candidate = dominantCandidate(x, y, up, down, left, right);

            if (heldDirection != 0) {
                const int currentStrength = directionStrength(heldDirection, x, y,
                                                               up, down, left, right);
                if (currentStrength < RELEASE_THRESHOLD) {
                    heldDirection = candidate.strength >= PRESS_THRESHOLD ? candidate.direction : 0;
                } else if (candidate.direction != 0 && candidate.direction != heldDirection &&
                           candidate.strength >= PRESS_THRESHOLD &&
                           candidate.strength >= currentStrength + AXIS_SWITCH_MARGIN) {
                    heldDirection = candidate.direction;
                }
            } else if (candidate.strength >= PRESS_THRESHOLD) {
                heldDirection = candidate.direction;
            }

            return {heldDirection & ~previous, heldDirection};
        }

        void reset() { heldDirection = 0; }

    private:
        struct Candidate {
            std::uint64_t direction = 0;
            int strength = 0;
        };

        static int magnitude(int value) { return value < 0 ? -value : value; }

        static Candidate dominantCandidate(int x, int y,
                                           std::uint64_t up, std::uint64_t down,
                                           std::uint64_t left, std::uint64_t right) {
            const int horizontal = magnitude(x);
            const int vertical = magnitude(y);
            if (horizontal > vertical)
                return {x < 0 ? left : right, horizontal};
            if (vertical > 0)
                return {y < 0 ? down : up, vertical};
            return {};
        }

        static int directionStrength(std::uint64_t direction, int x, int y,
                                     std::uint64_t up, std::uint64_t down,
                                     std::uint64_t left, std::uint64_t right) {
            if (direction == left)  return -x;
            if (direction == right) return x;
            if (direction == up)    return y;
            if (direction == down)  return -y;
            return 0;
        }

        std::uint64_t heldDirection = 0;
    };

    // Shared physical-navigation path for every controller-first screen. This keeps Party, Boxes,
    // Storage and the source pickers on identical D-pad/Left-Stick semantics.
    class ControllerNavigation {
    public:
        std::uint64_t apply(std::uint64_t digitalPressed, std::uint64_t digitalHeld,
                            int stickX, int stickY,
                            std::uint64_t up, std::uint64_t down,
                            std::uint64_t left, std::uint64_t right) {
            return apply(digitalPressed, digitalHeld, stickX, stickY, up, down, left, right,
                         monotonicMilliseconds());
        }

        std::uint64_t apply(std::uint64_t digitalPressed, std::uint64_t digitalHeld,
                            int stickX, int stickY,
                            std::uint64_t up, std::uint64_t down,
                            std::uint64_t left, std::uint64_t right, std::uint64_t nowMs) {
            const AnalogNavigationSample analog =
                analogNavigation.sample(stickX, stickY, up, down, left, right);
            const std::uint64_t mask = up | down | left | right;
            return navigationRepeat.apply(digitalPressed | analog.down,
                                          digitalHeld | analog.held, mask, nowMs);
        }

        void reset() {
            analogNavigation.reset();
            navigationRepeat.reset();
        }

    private:
        AnalogNavigation analogNavigation;
        NavigationRepeat navigationRepeat;
    };
}

#endif
