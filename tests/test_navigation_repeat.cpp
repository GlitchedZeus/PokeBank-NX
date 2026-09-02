#include <cassert>
#include <cstdint>

#include "UI/NavigationRepeat.h"

int main() {
    using PokeBank::UIModel::AnalogNavigation;
    using PokeBank::UIModel::ControllerNavigation;
    using PokeBank::UIModel::NavigationRepeat;
    constexpr std::uint64_t up = 1ULL << 0;
    constexpr std::uint64_t down = 1ULL << 1;
    constexpr std::uint64_t left = 1ULL << 2;
    constexpr std::uint64_t right = 1ULL << 3;
    constexpr std::uint64_t a = 1ULL << 8;
    constexpr std::uint64_t mask = up | down;

    NavigationRepeat repeat;
    assert(repeat.apply(up, up, mask) == up);
    for (int frame = 1; frame < NavigationRepeat::INITIAL_DELAY_FRAMES; ++frame)
        assert(repeat.apply(0, up, mask) == 0);
    assert(repeat.apply(0, up, mask) == up);
    for (int frame = 1; frame < NavigationRepeat::REPEAT_INTERVAL_FRAMES; ++frame)
        assert(repeat.apply(0, up, mask) == 0);
    assert(repeat.apply(0, up, mask) == up);

    // Changing direction restarts the delay but preserves the real edge press.
    assert(repeat.apply(down, down, mask) == down);
    assert(repeat.apply(0, down, mask) == 0);

    // Non-navigation buttons are never synthesized or delayed.
    assert(repeat.apply(a, down | a, mask) == a);
    repeat.reset();
    assert(repeat.apply(a, 0, mask) == a);

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

    // The combined path emits the analog edge once, then repeats while the physical stick remains
    // held; release and an opposite direction re-arm it predictably.
    ControllerNavigation controller;
    assert(controller.apply(0, 0, 0, 20000, up, down, left, right) == up);
    for (int frame = 1; frame < NavigationRepeat::INITIAL_DELAY_FRAMES; ++frame)
        assert(controller.apply(0, 0, 0, 20000, up, down, left, right) == 0);
    assert(controller.apply(0, 0, 0, 20000, up, down, left, right) == up);
    assert(controller.apply(0, 0, 0, 0, up, down, left, right) == 0);
    assert(controller.apply(0, 0, 0, -20000, up, down, left, right) == down);
}
