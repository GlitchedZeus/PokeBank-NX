#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace PokeBank::UIModel::ClassicPackedMove {

enum class Direction { Up, Down, Left, Right };

struct SelectionSlots {
    std::array<int, 30> values{};
    std::size_t count = 0;

    constexpr int operator[](std::size_t index) const noexcept { return values[index]; }
};

constexpr int gridColumns(int slotCount) noexcept {
    return slotCount == 25 ? 5 : 6;
}

constexpr SelectionSlots rectangleSlots(int anchor, int cursor, int slotCount) noexcept {
    SelectionSlots out{};
    if (slotCount <= 0 || anchor < 0 || cursor < 0 ||
        anchor >= slotCount || cursor >= slotCount) return out;
    const int columns = gridColumns(slotCount);
    const int ar = anchor / columns, ac = anchor % columns;
    const int cr = cursor / columns, cc = cursor % columns;
    const int top = std::min(ar, cr), bottom = std::max(ar, cr);
    const int left = std::min(ac, cc), right = std::max(ac, cc);
    for (int row = top; row <= bottom; ++row) {
        for (int col = left; col <= right; ++col) {
            const int slot = row * columns + col;
            if (slot >= 0 && slot < slotCount)
                out.values[out.count++] = slot;
        }
    }
    return out;
}

constexpr int moveCursor(int cursor, Direction direction, int slotCount) noexcept {
    if (slotCount <= 0) return 0;
    cursor = std::clamp(cursor, 0, slotCount - 1);
    const int columns = gridColumns(slotCount);
    const int row = cursor / columns, col = cursor % columns;
    int next = cursor;
    switch (direction) {
        case Direction::Up:    next = cursor - columns; break;
        case Direction::Down:  next = cursor + columns; break;
        case Direction::Left:  next = col > 0 ? cursor - 1 : cursor; break;
        case Direction::Right: next = col + 1 < columns ? cursor + 1 : cursor; break;
    }
    if (next < 0 || next >= slotCount) return cursor;
    (void)row;
    return next;
}

} // namespace PokeBank::UIModel::ClassicPackedMove
