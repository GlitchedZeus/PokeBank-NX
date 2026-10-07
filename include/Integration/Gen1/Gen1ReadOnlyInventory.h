#ifndef POKEBANK_INTEGRATION_GEN1_READ_ONLY_INVENTORY_H
#define POKEBANK_INTEGRATION_GEN1_READ_ONLY_INVENTORY_H

#include "Integration/Gen1/Gen1ReadOnlySave.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen1 {

inline constexpr size_t kInventoryCategoryCount = 2;

struct InventoryEntry {
    uint8_t itemId = 0;
    uint8_t quantity = 0;
};

struct ReadOnlyInventory {
    bool available = false;
    std::string error;
    std::vector<InventoryEntry> bag;
    std::vector<InventoryEntry> pcItems;
};

const char* inventoryCategoryName(size_t category) noexcept;
bool isValidGen1InventoryItem(uint8_t itemId) noexcept;
ReadOnlyInventory decodeInventory(
    std::span<const uint8_t> source, RegionLayout region) noexcept;

} // namespace PokeVault::Integration::Gen1

#endif
