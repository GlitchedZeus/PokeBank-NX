#ifndef TRAINER_INVENTORY9_H
#define TRAINER_INVENTORY9_H

#include <cstdint>
#include <vector>
#include "Trainer/Inventory.h"

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
constexpr size_t GEN9_ITEM_BLOCK_SIZE = 0xBB80; // 47,872 bytes

// Size of each item entry
constexpr size_t GEN9_ITEM_SIZE = 0x10;  // 16 bytes per item

// Maximum item ID (block size / item size)
constexpr size_t GEN9_MAX_ITEM_ID = GEN9_ITEM_BLOCK_SIZE / GEN9_ITEM_SIZE;  // 2992

constexpr size_t POUCH_COUNT_GEN9 = 8; // Number of pouches

/**
 * Gen 9 Item Structure (16 bytes per item)
 * Offset 0-3: Pouch (uint32) - which pouch this item belongs to
 * Offset 4-7: Count (int32) - quantity of this item
 * Offset 8-11: Flags (uint32) - isNew, isFavorite, etc.
 * Offset 12-15: Padding (uint32) - reserved
 */
struct InventoryItem9 {
    uint32_t pouchId;     // Which pouch this item belongs to
    uint16_t itemId;      // Item ID (not stored, derived from index)
    int32_t count;        // Quantity
    uint32_t flags;       // Flags (isNew, isFavorite, etc.)

    // Decode from 16-byte block at given item ID
    static InventoryItem9 fromBytes(uint16_t itemId, const uint8_t* data) {
        InventoryItem9 item;
        item.itemId = itemId;

        // Bytes 0-3: Pouch ID
        item.pouchId = (data[0]) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

        // Bytes 4-7: Count (signed int32)
        item.count = (data[4]) | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);

        // Bytes 8-11: Flags
        item.flags = (data[8]) | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);

        return item;
    }

    // Encode to 16-byte block
    void toBytes(uint8_t* data) const {
        // Bytes 0-3: Pouch ID
        data[0] = pouchId & 0xFF;
        data[1] = (pouchId >> 8) & 0xFF;
        data[2] = (pouchId >> 16) & 0xFF;
        data[3] = (pouchId >> 24) & 0xFF;

        // Bytes 4-7: Count
        uint32_t countBits = static_cast<uint32_t>(count);
        data[4] = countBits & 0xFF;
        data[5] = (countBits >> 8) & 0xFF;
        data[6] = (countBits >> 16) & 0xFF;
        data[7] = (countBits >> 24) & 0xFF;

        // Bytes 8-11: Flags
        data[8] = flags & 0xFF;
        data[9] = (flags >> 8) & 0xFF;
        data[10] = (flags >> 16) & 0xFF;
        data[11] = (flags >> 24) & 0xFF;

        // Bytes 12-15: Padding
        data[12] = 0;
        data[13] = 0;
        data[14] = 0;
        data[15] = 0;
    }

    // Flag bits
    bool isNew() const { return (flags & 0x1) != 0; }
    bool isFavorite() const { return (flags & 0x2) != 0; }

    // Convert to old InventoryItem format
    InventoryItem toInventoryItem() const {
        InventoryItem item;
        item.itemId = itemId;
        item.count = static_cast<uint16_t>(count);
        item.isNew = isNew();
        item.isFavorite = isFavorite();
        return item;
    }
};

enum class PouchType9 : uint32_t {
    Medicine = 0,
    Balls = 1,
    BattleItems = 2,
    Treasure = 3,
    KeyItems = 4,
    Berries = 5,
    TMs = 6,
    MegaStones = 7,
    // None = 0xFFFFFFFF, // Is this really needed?
};

/**
 * Valid item IDs for each pouch
 */
inline const std::vector<uint16_t>& getValidItemIds(PouchType9 pouch) {
    static const std::vector<uint16_t> medicine = {
        17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,708
    };
    static const std::vector<uint16_t> balls = {
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,492,493,494,495,496,497,498,499,576,851
    };
    static const std::vector<uint16_t> berries = {
        149,150,151,152,153,155,156,157,158,169,170,171,172,173,174,175,176,177,178,179,180,181,182,
        183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,686
    };
    static const std::vector<uint16_t> battleItems = {
        45,46,47,48,49,50,52,80,81,82,83,84,85,103,107,108,109,214,217,218,221,222,230,231,232,233,234,
        236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,253,266,267,268,270,275,289,290,291,
        292,293,294,296,538,540,564,565,566,567,568,569,570,639,640,646,647,710,711,795,796,849,
        1124,1125,1126,1127,1128,1231,1232,1233,1234,1235,1236,1237,1238,1239,1240,1241,1242,1243,1244,
        1245,1246,1247,1248,1249,1250,1582,1592,2401,2558,2618,2619
    };
    static const std::vector<uint16_t> treasure = {86,88,89,92,571,581,582};
    static const std::vector<uint16_t> keyItems = {
        632,700,765,847,2588,2589,2590,2591,2592,2595,2596,2597,2598,2599,2600,2601,2602,2603,2604,2605,
        2606,2607,2608,2609,2610,2611,2612,2613,2614,2615,2616,2617,2620,2621,2622,2623,2624,2625,2626,
        2627,2628,2629,2630,2631,2632,2633,2634
    };
    static const std::vector<uint16_t> tms = {
        328,329,330,331,332,333,334,335,336,337,338,339,340,341,342,343,344,345,346,347,348,349,350,351,
        352,353,354,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,
        376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,393,394,395,396,397,398,399,
        400,401,402,403,404,405,406,407,408,409,410,411,412,413,414,415,416,417,418,419,420,421,422,423,
        424,425,426,427,618,619,620,690,691,692,693,2160,2162,2163,2164,2165,2166,2167,2168
    };
    static const std::vector<uint16_t> megaStones = {
        656,657,658,659,660,661,662,663,664,665,666,667,668,669,670,671,672,673,674,675,676,677,678,679,
        680,681,682,683,754,755,756,757,758,759,760,761,762,763,764,765,766,767,768,769,770,2559,2560,
        2561,2562,2563,2564,2565,2566,2567,2568,2569,2570,2571,2572,2573,2574,2575,2576,2577,2578,2579,
        2580,2581,2582,2583,2584,2585,2586,2587,2635,2636,2637,2638,2639,2640,2641,2642,2643,2644,2645,
        2646,2647,2648,2649,2650
    };

    switch (pouch) {
        case PouchType9::Medicine: return medicine;
        case PouchType9::Balls: return balls;
        case PouchType9::Berries: return berries;
        case PouchType9::BattleItems: return battleItems;
        case PouchType9::Treasure: return treasure;
        case PouchType9::KeyItems: return keyItems;
        case PouchType9::TMs: return tms;
        case PouchType9::MegaStones: return megaStones;
        default: {
            static const std::vector<uint16_t> empty;
            return empty;
        }
    }
}

struct PouchInfo9 {
    PouchType9 type;
    const char* name;
};

inline const PouchInfo9& getPouchInfo9(PouchType9 type) {
    static const PouchInfo9 pouches[] = {
        {PouchType9::Medicine, "Medicine"},
        {PouchType9::Balls, "Poke Balls"},
        {PouchType9::BattleItems, "Battle Items"},
        {PouchType9::Treasure, "Treasure"},
        {PouchType9::KeyItems, "Key Items"},
        {PouchType9::Berries, "Berries"},
        {PouchType9::TMs, "TMs"},
        {PouchType9::MegaStones, "Mega Stones"}
    };
    return pouches[static_cast<int>(type)];
}

#endif