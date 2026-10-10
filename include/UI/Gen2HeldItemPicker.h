#pragma once
#include "Inventory/ClassicInventoryCatalog.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/SharedHeldItemPickerPresentation.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ItemPickerArtwork.h"
#include "UI/Common.h"
#include <vector>

namespace PokeBank::UIModel::Gen2HeldItemPicker {
inline constexpr int columns = SharedHeldItemPicker::columns;
inline constexpr int rows = SharedHeldItemPicker::rows;
inline constexpr int pageSize = SharedHeldItemPicker::pageSize;
inline int initialIndex(const std::vector<uint8_t>& items, uint8_t current) {
    return SharedHeldItemPicker::initialIndex(items, current);
}
inline int move(int index, int count, int dx, int dy, int pages = 0) noexcept {
    return SharedHeldItemPicker::move(index, count, dx, dy, pages);
}
inline std::string itemName(uint8_t item) {
    if (item == 0) return "None";
    // Gold, Silver and Crystal share this exact machine map and catalog formatter.
    using namespace PokeVault::Inventory;
    return displayItemName(ClassicGame::Gold, ClassicPocket::PCItems, item);
}

// The display label includes the move for readability (e.g. "TM27 — Return"),
// but the pinned item sprite is named "tm27.png", not "tm27-return.png".
// Never convert a Gen II item number into a Gen III/IV item number.
inline std::string spriteName(uint8_t item) {
    std::string name = itemName(item);
    if (name.size() > 2 && ((name[0] == 'T' && name[1] == 'M') ||
                            (name[0] == 'H' && name[1] == 'M'))) {
        const std::size_t end = name.find(' ');
        if (end != std::string::npos && end > 2) name.resize(end);
    }
    return name;
}
}

namespace UI::Gen2HeldItemPickerPresentation {
inline void drawList(PKSEFramebuffer& fb, int panelX, int panelY,
                     const std::vector<uint8_t>& items, int selected) {
    namespace Model = PokeBank::UIModel::Gen2HeldItemPicker;
    // True Gen II item IDs and display names; shared Gen III grid geometry.
    SharedHeldItemPickerPresentation::drawGrid(
        fb, panelX, panelY, items, selected,
        [](uint8_t item) { return Model::itemName(item); },
        [](uint8_t item) { return Model::spriteName(item); });
}
}
