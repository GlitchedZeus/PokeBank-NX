#ifndef POKEBANK_UI_GEN4_BOX_MOVE_NAVIGATION_H
#define POKEBANK_UI_GEN4_BOX_MOVE_NAVIGATION_H

#include <algorithm>

namespace PokeBank::UIModel::Gen4BoxMoveNavigation {

// Exactly the six-column, five-row PC-box cursor semantics used by the
// established Gen I–III box actions. This is navigation only: no save writes.
struct Cursor {
    int box = 0;
    int slot = 0;
};
enum class Direction { Left, Right, Up, Down, PreviousBox, NextBox, JumpBack, JumpForward };
[[nodiscard]] constexpr Cursor navigate(Cursor value, Direction direction,
                                        int boxCount = 18) noexcept {
    if (boxCount < 1) return value;
    value.box = std::clamp(value.box, 0, boxCount - 1);
    value.slot = std::clamp(value.slot, 0, 29);
    constexpr int columns = 6, rows = 5;
    const int row = value.slot / columns;
    const int col = value.slot % columns;
    switch (direction) {
        case Direction::Left:
            value.slot = row * columns + (col + columns - 1) % columns;
            break;
        case Direction::Right:
            value.slot = row * columns + (col + 1) % columns;
            break;
        case Direction::Up:
            value.slot = ((row + rows - 1) % rows) * columns + col;
            break;
        case Direction::Down:
            value.slot = ((row + 1) % rows) * columns + col;
            break;
        case Direction::PreviousBox:
            value.box = (value.box + boxCount - 1) % boxCount;
            break;
        case Direction::NextBox:
            value.box = (value.box + 1) % boxCount;
            break;
        case Direction::JumpBack:
            value.box = (value.box + boxCount - (5 % boxCount)) % boxCount;
            break;
        case Direction::JumpForward:
            value.box = (value.box + 5) % boxCount;
            break;
    }
    return value;
}
} // namespace PokeBank::UIModel::Gen4BoxMoveNavigation
#endif
