#include <cassert>
#include <cstdint>

#include "UI/NavigationRepeat.h"

int main() {
    using PokeBank::UIModel::NavigationRepeat;
    constexpr std::uint64_t up = 1ULL << 0;
    constexpr std::uint64_t down = 1ULL << 1;
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
}
