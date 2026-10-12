#include "Trainer/Inventory.h"
#include "Trainer/Inventory9SV.h"
#include "Trainer/Inventory9LZA.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
std::array<uint8_t, 16> record(uint32_t pouch, uint32_t count, uint32_t flags) {
    std::array<uint8_t, 16> out{};
    const auto put32 = [&](std::size_t offset, uint32_t value) {
        out[offset + 0] = static_cast<uint8_t>(value);
        out[offset + 1] = static_cast<uint8_t>(value >> 8);
        out[offset + 2] = static_cast<uint8_t>(value >> 16);
        out[offset + 3] = static_cast<uint8_t>(value >> 24);
    };
    put32(0, pouch);
    put32(4, count);
    put32(8, flags);
    return out;
}

template <typename Item>
void checkFlags(uint32_t flags, bool expectedNew, bool expectedFavorite) {
    const auto raw = record(0x89ABCDEFu, 0x80000002u, flags);
    const Item decoded = Item::fromBytes(777, raw.data());
    assert(decoded.itemId == 777);
    assert(decoded.pouchId == 0x89ABCDEFu);
    assert(decoded.count == 2);
    assert(decoded.flags == flags);
    assert(decoded.isNew == expectedNew);
    assert(decoded.isFavorite == expectedFavorite);

    // Slicing into Trainer::items must retain deterministic persisted flag state.
    const Trainer::InventoryItem base = decoded;
    assert(base.itemId == 777 && base.count == 2);
    assert(base.isNew == expectedNew);
    assert(base.isFavorite == expectedFavorite);
}
}

int main() {
    Trainer::InventoryItem empty{};
    assert(empty.itemId == 0 && empty.count == 0);
    assert(!empty.isNew && !empty.isFavorite);

    for (uint32_t flags = 0; flags < 4; ++flags) {
        const bool isNew = (flags & 1u) != 0;
        const bool isFavorite = (flags & 2u) != 0;
        checkFlags<Trainer::InventoryItem9SV>(flags, isNew, isFavorite);
        checkFlags<Trainer::InventoryItem9LZA>(flags, isNew, isFavorite);
    }

    // High flag bits are decoded with defined unsigned shifts and do not alter bit 0/1 semantics.
    checkFlags<Trainer::InventoryItem9SV>(0x80000003u, true, true);
    checkFlags<Trainer::InventoryItem9LZA>(0x80000000u, false, false);

    std::cout << "Gen IX inventory decode flags: PASS\n";
    return 0;
}
