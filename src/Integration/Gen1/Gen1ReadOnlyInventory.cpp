#include "Integration/Gen1/Gen1ReadOnlyInventory.h"

#include <array>

namespace PokeVault::Integration::Gen1 {
namespace {

// Oracle basis:
// - PKHeX PlayerBag1 / SAV1Offsets: two count-prefixed inventories, 20 carried
//   slots and 50 PC slots, with international offsets 0x25C9/0x27E6 and
//   Japanese offsets 0x25C4/0x27DC.
// - PKHeX InventoryPouchGB: [count][item,quantity]... followed by 0xFF.
// - pinned PKSM-Core Sav1::validItems1: accepted raw Gen I item IDs.
// The decoder intentionally preserves raw Gen I IDs. Display conversion is handled
// by Names::getItemNameG1 so Gen I-only items are never mislabeled as modern ID 128.
constexpr size_t kBagCapacity = 20;
constexpr size_t kPcCapacity = 50;
constexpr uint8_t kTerminator = 0xFF;
constexpr uint8_t kMaxQuantity = 99;

constexpr size_t bagOffset(RegionLayout region) noexcept {
    return region == RegionLayout::Japanese ? 0x25C4 : 0x25C9;
}

constexpr size_t pcOffset(RegionLayout region) noexcept {
    return region == RegionLayout::Japanese ? 0x27DC : 0x27E6;
}

bool decodeBlock(std::span<const uint8_t> source, size_t offset, size_t capacity,
                 std::vector<InventoryEntry>& out, const char* label,
                 std::string& error) noexcept {
    const size_t blockSize = 1 + capacity * 2 + 1;
    if (offset > source.size() || blockSize > source.size() - offset) {
        error = std::string(label) + " inventory block is truncated";
        return false;
    }

    const uint8_t count = source[offset];
    if (count > capacity) {
        error = std::string(label) + " inventory count exceeds capacity";
        return false;
    }

    const size_t terminatorOffset = offset + 1 + static_cast<size_t>(count) * 2;
    if (source[terminatorOffset] != kTerminator) {
        error = std::string(label) + " inventory terminator is invalid";
        return false;
    }

    out.clear();
    out.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const size_t entryOffset = offset + 1 + i * 2;
        const uint8_t itemId = source[entryOffset];
        const uint8_t quantity = source[entryOffset + 1];
        if (!isValidGen1InventoryItem(itemId)) {
            error = std::string(label) + " inventory contains an invalid item id";
            out.clear();
            return false;
        }
        if (quantity == 0 || quantity > kMaxQuantity) {
            error = std::string(label) + " inventory contains an invalid quantity";
            out.clear();
            return false;
        }
        out.push_back({itemId, quantity});
    }
    return true;
}

} // namespace

const char* inventoryCategoryName(size_t category) noexcept {
    switch (category) {
        case 0: return "Bag";
        case 1: return "PC Items";
        default: return "?";
    }
}

bool isValidGen1InventoryItem(uint8_t itemId) noexcept {
    if (itemId >= 0xC4 && itemId <= 0xFA) return true; // HM01-05, TM01-50.
    constexpr std::array<uint8_t, 69> valid{{
        1,2,3,4,5,6,10,11,12,13,14,15,16,17,18,19,20,
        29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,
        45,46,47,48,49,51,52,53,54,55,56,57,58,60,61,
        62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,
        77,78,79,80,81,82,83
    }};
    for (const uint8_t validId : valid) if (validId == itemId) return true;
    return false;
}

ReadOnlyInventory decodeInventory(
    std::span<const uint8_t> source, RegionLayout region) noexcept {
    ReadOnlyInventory result;
    if (!decodeBlock(source, bagOffset(region), kBagCapacity,
                     result.bag, "Bag", result.error)) {
        result.bag.clear();
        result.pcItems.clear();
        return result;
    }
    if (!decodeBlock(source, pcOffset(region), kPcCapacity,
                     result.pcItems, "PC", result.error)) {
        result.bag.clear();
        result.pcItems.clear();
        return result;
    }
    result.available = true;
    return result;
}

} // namespace PokeVault::Integration::Gen1
