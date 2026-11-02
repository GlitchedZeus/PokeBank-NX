#pragma once

#include <algorithm>

// One list-picker paging scheme for the Gen I-III species, move, location and value pickers,
// identical to the shared Gen IV/V editor's list pickers (SharedPokemonEditorSurface.inc
// handleValuePicker): Up/Down step one row and wrap; Left or L page back 10, Right or R page
// forward 10, ZL/ZR jump 50 - all clamped to the list. Grid pickers (held items) keep their own
// 2-D navigation.
namespace PokeBank::UIModel::SharedPickerPaging {

struct Keys {
    bool up = false, down = false, left = false, right = false;
    bool l = false, r = false, zl = false, zr = false;
    constexpr bool any() const noexcept { return up || down || left || right || l || r || zl || zr; }
};

[[nodiscard]] constexpr int pagedRow(int row, int count, const Keys& k) noexcept {
    if (count <= 0) return 0;
    row = std::clamp(row, 0, count - 1);
    if (k.up) row = (row - 1 + count) % count;
    if (k.down) row = (row + 1) % count;
    if (k.left || k.l) row = std::max(0, row - 10);
    if (k.right || k.r) row = std::min(count - 1, row + 10);
    if (k.zl) row = std::max(0, row - 50);
    if (k.zr) row = std::min(count - 1, row + 50);
    return row;
}

} // namespace PokeBank::UIModel::SharedPickerPaging
