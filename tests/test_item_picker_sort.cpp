#include "UI/ItemPickerSort.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

int main() {
    using namespace UI::ItemPickerSort;
    static_assert(next(Mode::Native) == Mode::NameAZ);
    static_assert(next(Mode::NameAZ) == Mode::NameZA);
    static_assert(next(Mode::NameZA) == Mode::Native);
    static_assert(std::string_view(label(Mode::Native)) == "Native order");

    const std::vector<std::uint16_t> original{3, 1, 4, 2};
    std::vector<std::uint16_t> displayed = original;
    const auto name = [](std::uint16_t id) {
        switch (id) {
            case 1: return std::string("apple");
            case 2: return std::string("Zinc");
            case 3: return std::string("Berry");
            default: return std::string("Pecha");
        }
    };
    // Start focused on native item 4, and never change that item's identity.
    int selected = 2;
    selected = reorder(displayed, original, Mode::NameAZ, selected, name);
    assert((displayed == std::vector<std::uint16_t>{1, 3, 4, 2}));
    assert(displayed.at(static_cast<std::size_t>(selected)) == 4);
    selected = reorder(displayed, original, Mode::NameZA, selected, name);
    assert((displayed == std::vector<std::uint16_t>{2, 4, 3, 1}));
    assert(displayed.at(static_cast<std::size_t>(selected)) == 4);
    selected = reorder(displayed, original, Mode::Native, selected, name);
    assert(displayed == original);
    assert(selected == 2);
    // Item IDs, not translated labels, determine which item gets selected.
    const auto snapshot = original;
    assert(reorder(displayed, original, Mode::NameAZ, -1, name) == 0);
    assert(original == snapshot);
    assert(reorder(displayed, original, Mode::Native, 99, name) == 0);
    assert(displayed == original);
    std::cout << "Gen I-IV item picker sorting and native-ID preservation: PASS\n";
}
