#ifndef TRAINER_INVENTORY9_SV_H
#define TRAINER_INVENTORY9_SV_H

#include <cstdint>
#include <vector>

#include "Trainer/Inventory.h"

namespace Trainer {
    /**
     * Inventory9.h - Generation 9 Item/Inventory Management
     *
     * Key Facts:
     * - Items are stored by ITEM ID as index
     * - Item at ID X is at offset (X * 0x10)
     * - Each item slot is 16 bytes
     * - Block size: 0xBB80 bytes
     */

    // Block size for Gen 9 item storage
    constexpr size_t ITEM_BLOCK_SIZE9_SV = 0xBB80; // 47,872 bytes

    // Size of each item entry
    constexpr size_t ITEM_SIZE9_SV = 0x10;  // 16 bytes per item

    // Maximum item ID (block size / item size)
    constexpr size_t MAX_ITEM_ID9_SV = ITEM_BLOCK_SIZE9_SV / ITEM_SIZE9_SV;  // 2992

    constexpr size_t POUCH_COUNT9_SV = 10; // Number of pouches (in-game bag order)

    /**
     * Gen 9 Item Structure (16 bytes per item)
     * Offset 0-3: Pouch (uint32) - which pouch this item belongs to
     * Offset 4-7: Count (int32) - quantity of this item
     * Offset 8-11: Flags (uint32) - isNew, isFavorite, etc.
     * Offset 12-15: Padding (uint32) - reserved
     */
    struct InventoryItem9SV: InventoryItem {
        uint32_t pouchId;     // Which pouch this item belongs to
        uint32_t flags;       // Flags (isNew, isFavorite, etc.)

        // Decode from 16-byte block at given item ID
        static InventoryItem9SV fromBytes(uint16_t itemId, const uint8_t* data) {
            InventoryItem9SV item{};
            const auto read32 = [](const uint8_t* p) noexcept -> uint32_t {
                return static_cast<uint32_t>(p[0])
                     | (static_cast<uint32_t>(p[1]) << 8)
                     | (static_cast<uint32_t>(p[2]) << 16)
                     | (static_cast<uint32_t>(p[3]) << 24);
            };

            // Bytes 0-3: Pouch ID
            item.pouchId = read32(data);
            // Bytes 4-7: Count (native int32; UI stores the non-negative low 16-bit quantity)
            const uint32_t nativeCount = read32(data + 4);
            item.count = static_cast<uint16_t>(nativeCount);
            // Bytes 8-11: Flags. Preserve the base fields before the derived record is sliced into
            // Trainer::items; bit 0 = NEW, bit 1 = FAVORITE.
            item.flags = read32(data + 8);
            item.isNew = (item.flags & 0x01u) != 0;
            item.isFavorite = (item.flags & 0x02u) != 0;

            item.itemId = itemId;
            return item;
        }
    };

    /**
     * Scarlet/Violet's bag pockets, per PKHeX ItemStorage9SV.GetLegal.
     *
     * This was previously a verbatim copy of Legends: Z-A's pouch model, which gave S/V a
     * Mega Stones pocket it does not have while omitting the "Other Items" and picnic
     * Ingredients pockets it does. Composition is display-only -- the item write
     * is keyed on item id, not on pouch -- so correcting it cannot affect a save.
     *
     * The legal id list per pouch is generated: Names::getPouchItems(GameVersion::SV, idx).
     */
    // Order == the in-game bag tab order. The array below is indexed by the enum value, so the enum
    // order, the display order, and the generated getPouchItems(SV, idx) order must all agree.
    enum class PouchType9SV : uint32_t {
        Medicine = 0,
        Balls = 1,
        BattleItems = 2,
        Berries = 3,
        Other = 4,         // "Other Items" pocket -- general items
        TMs = 5,
        Material = 6,      // "TM Materials" (PKHeX Candy/Material span) -- was missing entirely
        Treasure = 7,
        Ingredients = 8,   // picnic ingredients + furniture; shown as "Picnic Items"
        KeyItems = 9,      // PKHeX keeps S/V key items in its `Event` span
    };

    struct PouchInfo9SV {
        PouchType9SV type;
        const char* name;
    };

    inline const PouchInfo9SV& getPouchInfo9SV(PouchType9SV type) {
        static const PouchInfo9SV pouches[] = {
            {PouchType9SV::Medicine, "Medicines"},
            {PouchType9SV::Balls, "Poké Balls"},
            {PouchType9SV::BattleItems, "Battle Items"},
            {PouchType9SV::Berries, "Berries"},
            {PouchType9SV::Other, "Other Items"},
            {PouchType9SV::TMs, "TMs"},
            {PouchType9SV::Material, "TM Materials"},
            {PouchType9SV::Treasure, "Treasures"},
            {PouchType9SV::Ingredients, "Picnic Items"},
            {PouchType9SV::KeyItems, "Key Items"}
        };
        return pouches[static_cast<int>(type)];
    }
}

#endif