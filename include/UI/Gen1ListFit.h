#pragma once

#include <algorithm>

// How many whole rows of `rowStep` fit between `top` and `bottom` (exclusive), capped at `maxRows`
// and never fewer than one. Gen I list editors draw only these rows, so a focused row is never
// clipped by, or drawn under, the status line or footer beneath the list.
namespace PokeBank::UIModel::Gen1ListFit {

constexpr int wholeRows(int top, int bottom, int rowStep, int maxRows) noexcept {
    if (rowStep <= 0) return 1;
    return std::clamp((bottom - top) / rowStep, 1, std::max(1, maxRows));
}

} // namespace PokeBank::UIModel::Gen1ListFit
