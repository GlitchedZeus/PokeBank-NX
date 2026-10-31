#ifndef POKEBANK_UI_GEN4_ITEM_LIST_WINDOW_H
#define POKEBANK_UI_GEN4_ITEM_LIST_WINDOW_H

#include <algorithm>

namespace PokeBank::UIModel::Gen4ItemListWindow {

// A sliding, full-height window instead of fixed pages. Navigating upward
// from item zero to the last item must not render a mostly-empty final page.
// Entirely presentation-only; item identities and staged writes are unchanged.
[[nodiscard]] constexpr int first(int selection, int total, int visible = 9) noexcept {
    if (total <= 0 || visible <= 0 || total <= visible) return 0;
    const int index = std::clamp(selection, 0, total - 1);
    return std::clamp(index - visible / 2, 0, total - visible);
}

} // namespace PokeBank::UIModel::Gen4ItemListWindow
#endif
