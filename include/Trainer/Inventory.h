#ifndef INVENTORY_H
#define INVENTORY_H

#include <cstdint>

/// Item structure - each item is 4 bytes
struct InventoryItem {
    uint16_t itemId;      // Bits 0-10
    uint16_t count;       // Bits 15-24
    bool isNew;           // Bit 30
    bool isFavorite;      // Bit 31

    // Decode from 32-bit value
    static InventoryItem fromValue(uint32_t value) {
        InventoryItem item;
        item.itemId = value & 0x7FF;
        item.count = (value >> 15) & 0x3FF;
        item.isNew = (value & 0x40000000) != 0;
        item.isFavorite = (value & 0x80000000) != 0;
        return item;
    }

    // Encode to 32-bit value
    uint32_t toValue() const {
        uint32_t val = (itemId & 0x7FF) | ((count & 0x3FF) << 15);
        if (isNew) val |= 0x40000000;
        if (isFavorite) val |= 0x80000000;
        return val;
    }
};

/// Item pouch categories
enum class PouchType {
    Medicine = 0,   // Offset 0, 60 slots
    Balls,          // Offset 240, 30 slots
    Battle,         // Offset 360, 20 slots
    Berries,        // Offset 440, 80 slots
    Items,          // Offset 760, 550 slots
    TMs,            // Offset 2960, 210 slots
    Treasures,      // Offset 3800, 100 slots
    Ingredients,    // Offset 4200, 100 slots
    KeyItems,       // Offset 4600, 64 slots
    Count           // Total number of pouches
};

/// Pouch information
struct PouchInfo {
    PouchType type;
    const char* name;
    int offset;
    int maxCount;
};

/// Get pouch info for a given type
inline const PouchInfo& getPouchInfo(PouchType type) {
    static const PouchInfo pouches[] = {
        {PouchType::Medicine, "Medicine", 0, 60},
        {PouchType::Balls, "Poke Balls", 240, 30},
        {PouchType::Battle, "Battle Items", 360, 20},
        {PouchType::Berries, "Berries", 440, 80},
        {PouchType::Items, "Items", 760, 550},
        {PouchType::TMs, "TMs/TRs", 2960, 210},
        {PouchType::Treasures, "Treasures", 3800, 100},
        {PouchType::Ingredients, "Ingredients", 4200, 100},
        {PouchType::KeyItems, "Key Items", 4600, 64}
    };
    return pouches[static_cast<int>(type)];
}

#endif // INVENTORY_H