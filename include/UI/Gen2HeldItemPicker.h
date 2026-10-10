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
constexpr int move(int index, int count, int dx, int dy, int pages = 0) noexcept {
    return SharedHeldItemPicker::move(index, count, dx, dy, pages);
}
inline std::string itemName(uint8_t item) {
    if (item == 0) return "None";
    // Gold, Silver and Crystal share this exact machine map and catalog formatter.
    using namespace PokeVault::Inventory;
    return displayItemName(ClassicGame::Gold, ClassicPocket::PCItems, item);
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
        [](uint8_t item) { return Model::itemName(item); });
}
}
