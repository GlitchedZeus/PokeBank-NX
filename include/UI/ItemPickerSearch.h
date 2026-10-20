#ifndef POKEBANK_UI_ITEM_PICKER_SEARCH_H
#define POKEBANK_UI_ITEM_PICKER_SEARCH_H

#include "UI/ItemPickerSort.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace UI::ItemPickerSearch {

// NEW-06 (Round 3): Held Item search alongside sorting. Like ItemPickerSort this only rebuilds the
// PRESENTATION COPY of the game's native held-item list; no item id is converted, added or stored.
// A query matches any item whose display name contains it, ignoring case, spaces and punctuation
// ("kings" finds King's Rock, "berry" every Berry).

inline std::string key(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        const auto u = static_cast<unsigned char>(c);
        if (std::isalnum(u)) out.push_back(static_cast<char>(std::tolower(u)));
        else if (u >= 0x80) out.push_back(c);   // keep UTF-8 bytes (é in Poké Ball) as typed
    }
    return out;
}

inline bool matches(const std::string& name, const std::string& query) {
    const std::string q = key(query);
    return q.empty() || key(name).find(q) != std::string::npos;
}

// Keep only the names in `displayed` that match `query`, preserving order and the selected item.
// Returns the selected item's new index (0 when it was filtered out); when nothing matches,
// `displayed` is left exactly as it was and -1 is returned, so a picker never becomes empty.
template <typename T, typename NameFor>
int narrow(std::vector<T>& displayed, const std::string& query, int selectedIndex, NameFor nameFor) {
    const bool valid = selectedIndex >= 0 && selectedIndex < static_cast<int>(displayed.size());
    if (key(query).empty()) return valid ? selectedIndex : 0;
    std::vector<T> kept;
    for (const T& value : displayed)
        if (matches(nameFor(value), query)) kept.push_back(value);
    if (kept.empty()) return -1;
    const T selected = valid ? displayed[static_cast<std::size_t>(selectedIndex)] : T{};
    displayed = std::move(kept);
    if (!valid) return 0;
    const auto it = std::find(displayed.begin(), displayed.end(), selected);
    return it == displayed.end() ? 0 : static_cast<int>(it - displayed.begin());
}

// The whole search: `original` (native order) in `mode` order, narrowed to `query`. On no match
// `displayed` is untouched and -1 is returned (the caller keeps the previous list).
template <typename T, typename NameFor>
int apply(std::vector<T>& displayed, const std::vector<T>& original, ItemPickerSort::Mode mode,
          const std::string& query, int selectedIndex, NameFor nameFor) {
    std::vector<T> next = displayed;
    const int sorted = ItemPickerSort::reorder(next, original, mode, selectedIndex, nameFor);
    const int index = narrow(next, query, sorted, nameFor);
    if (index < 0) return -1;
    displayed = std::move(next);
    return index;
}

} // namespace UI::ItemPickerSearch

#endif
