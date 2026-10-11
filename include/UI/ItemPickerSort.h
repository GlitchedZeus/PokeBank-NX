#ifndef POKEBANK_UI_ITEM_PICKER_SORT_H
#define POKEBANK_UI_ITEM_PICKER_SORT_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace UI::ItemPickerSort {

// Sort only the PRESENTATION COPY of native item IDs. Never reorder a game
// inventory pouch, mutate a staged item or reinterpret a stored item identity.
enum class Mode : std::uint8_t { Native, NameAZ, NameZA };

inline constexpr Mode next(Mode mode) noexcept {
    return mode == Mode::Native ? Mode::NameAZ :
           mode == Mode::NameAZ ? Mode::NameZA : Mode::Native;
}

inline constexpr const char* label(Mode mode) noexcept {
    return mode == Mode::Native ? "Native order" :
           mode == Mode::NameAZ ? "Name A-Z" : "Name Z-A";
}

inline std::string asciiFold(const std::string& value) {
    std::string result = value;
    for (char& c : result)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

template <typename T, typename NameFor>
int reorder(std::vector<T>& displayed, const std::vector<T>& original,
            Mode mode, int selectedIndex, NameFor nameFor) {
    T selected{};
    const bool valid = selectedIndex >= 0 &&
        selectedIndex < static_cast<int>(displayed.size());
    if (valid) selected = displayed[static_cast<std::size_t>(selectedIndex)];

    displayed = original;
    if (mode != Mode::Native) {
        std::stable_sort(displayed.begin(), displayed.end(),
            [&](const T& a, const T& b) {
                const std::string left = asciiFold(nameFor(a));
                const std::string right = asciiFold(nameFor(b));
                return mode == Mode::NameAZ ? left < right : left > right;
            });
    }
    if (!valid) return 0;
    const auto it = std::find(displayed.begin(), displayed.end(), selected);
    return it == displayed.end() ? 0 : static_cast<int>(it - displayed.begin());
}
} // namespace UI::ItemPickerSort

#endif
