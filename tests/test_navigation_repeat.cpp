#include <cassert>
#include <cstdint>

#include "UI/NavigationRepeat.h"

// Shared navigation timing contract (P0 #320 phase 8): the first move is immediate, held repeat
// starts after RepeatTiming::INITIAL_DELAY_MS and then steps every REPEAT_INTERVAL_MS of wall
// time, independent of the frame rate, one step per frame at most and never a burst.
int main() {
    using PokeBank::UIModel::AnalogNavigation;
    using PokeBank::UIModel::ControllerNavigation;
    using PokeBank::UIModel::HoldRepeatClock;
    using PokeBank::UIModel::NavigationRepeat;
    using PokeBank::UIModel::RepeatTiming;
    constexpr std::uint64_t up = 1ULL << 0;
    constexpr std::uint64_t down = 1ULL << 1;
    constexpr std::uint64_t left = 1ULL << 2;
    constexpr std::uint64_t right = 1ULL << 3;
    constexpr std::uint64_t a = 1ULL << 8;
    constexpr std::uint64_t mask = up | down;
    constexpr std::uint64_t delay = RepeatTiming::INITIAL_DELAY_MS;
    constexpr std::uint64_t interval = RepeatTiming::REPEAT_INTERVAL_MS;
    static_assert(delay >= 200 && delay <= 300, "short, predictable hold delay");
    static_assert(interval >= 50 && interval <= 80, "fast sustained repeat");

    // Same wall-clock cadence at 60, 30 and 20 fps: the first held step lands on the first frame
    // at or after the delay, and the step count over one second of holding stays the same within
    // one step, so a slow screen no longer stretches the delay.
    for (const std::uint64_t frameMs : {16ULL, 33ULL, 50ULL}) {
        NavigationRepeat repeat;
        const std::uint64_t t0 = 1000;
        assert(repeat.apply(up, up, mask, t0) == up); // immediate first move
        std::uint64_t firstRepeat = 0;
        int steps = 0;
        for (std::uint64_t t = t0 + frameMs; t <= t0 + 1000; t += frameMs) {
            if (repeat.apply(0, up, mask, t) == up) {
                if (!firstRepeat) firstRepeat = t;
                ++steps;
            }
        }
        assert(firstRepeat >= t0 + delay && firstRepeat < t0 + delay + frameMs);
        const int expected = static_cast<int>((1000 - delay) / interval) + 1;
        assert(steps >= expected - 1 && steps <= expected + 1);
    }

    // A stalled frame (e.g. a save re-parse) emits ONE step, never a burst that skips rows.
    {
        HoldRepeatClock clock;
        clock.start(0);
        assert(!clock.due(delay - 1));
        assert(clock.due(delay));
        assert(clock.due(delay + 5 * interval)); // 5 intervals late: still a single step
        assert(!clock.due(delay + 5 * interval + 1));
        assert(clock.due(delay + 6 * interval));
    }

    NavigationRepeat repeat;
    // Changing direction restarts the delay but preserves the real edge press.
    assert(repeat.apply(up, up, mask, 0) == up);
    assert(repeat.apply(down, down, mask, delay + 1) == down);
    assert(repeat.apply(0, down, mask, delay + 2) == 0);
    assert(repeat.apply(0, down, mask, 2 * delay + 1) == down);

    // Non-navigation buttons are never synthesized or delayed.
    assert(repeat.apply(a, down | a, mask, 2 * delay + 10) == a); // between repeats
    repeat.reset();
    assert(repeat.apply(a, 0, mask, 4 * delay) == a);

    // Releasing re-arms: a fresh press is immediate and waits the full delay again.
    repeat.reset();
    assert(repeat.apply(up, up, mask, 10000) == up);
    assert(repeat.apply(0, 0, mask, 10010) == 0);
    assert(repeat.apply(up, up, mask, 10020) == up);
    assert(repeat.apply(0, up, mask, 10020 + delay - 1) == 0);
    assert(repeat.apply(0, up, mask, 10020 + delay) == up);

    // Physical analog samples have a deadzone and a lower release threshold.
    AnalogNavigation analog;
    auto sample = analog.sample(4000, 3000, up, down, left, right);
    assert(sample.down == 0 && sample.held == 0);
    sample = analog.sample(0, 18000, up, down, left, right);
    assert(sample.down == up && sample.held == up);
    sample = analog.sample(0, 12000, up, down, left, right);
    assert(sample.down == 0 && sample.held == up);
    sample = analog.sample(0, 8000, up, down, left, right);
    assert(sample.down == 0 && sample.held == 0);

    // Diagonal jitter stays on the selected axis until the other axis clearly wins.
    sample = analog.sample(17000, 18000, up, down, left, right);
    assert(sample.down == up && sample.held == up);
    sample = analog.sample(19000, 18000, up, down, left, right);
    assert(sample.down == 0 && sample.held == up);
    sample = analog.sample(24000, 17000, up, down, left, right);
    assert(sample.down == right && sample.held == right);

    // D-pad / stick parity: the combined path emits the analog edge once, then repeats on the
    // same wall-clock contract while the stick stays held; release and the opposite direction
    // re-arm it predictably. Horizontal uses the identical contract.
    for (const int axis : {0, 1}) {
        ControllerNavigation controller;
        const int x = axis ? 20000 : 0, y = axis ? 0 : 20000;
        const std::uint64_t dir = axis ? right : up;
        assert(controller.apply(0, 0, x, y, up, down, left, right, 0) == dir);
        for (std::uint64_t t = 16; t < delay; t += 16)
            assert(controller.apply(0, 0, x, y, up, down, left, right, t) == 0);
        assert(controller.apply(0, 0, x, y, up, down, left, right, delay) == dir);
        assert(controller.apply(0, 0, 0, 0, up, down, left, right, delay + 16) == 0);
        assert(controller.apply(0, 0, -x, -y, up, down, left, right, delay + 32) == (axis ? left : down));
        // D-pad held gives the same cadence as the stick.
        ControllerNavigation dpad;
        assert(dpad.apply(dir, dir, 0, 0, up, down, left, right, 0) == dir);
        assert(dpad.apply(0, dir, 0, 0, up, down, left, right, delay - 1) == 0);
        assert(dpad.apply(0, dir, 0, 0, up, down, left, right, delay) == dir);
    }

    // The default overloads read the shared monotonic clock.
    const std::uint64_t before = PokeBank::UIModel::monotonicMilliseconds();
    assert(PokeBank::UIModel::monotonicMilliseconds() >= before);
}
