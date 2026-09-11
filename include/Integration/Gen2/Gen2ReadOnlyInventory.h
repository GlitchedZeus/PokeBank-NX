#ifndef POKEBANK_GEN2_READ_ONLY_INVENTORY_H
#define POKEBANK_GEN2_READ_ONLY_INVENTORY_H

#include "Integration/Gen2/Gen2ReadOnlySave.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen2 {

enum class InventoryPocket : uint8_t {
    TMHM = 0,
    Items = 1,
    KeyItems = 2,
    Balls = 3,
    PCItems = 4,
};

constexpr std::size_t kInventoryPocketCount = 5;

struct InventoryItem {
    uint8_t itemId = 0;
    uint8_t quantity = 0;
    std::string name;
};

struct InventoryRecord {
    bool available = false;
    std::string detail;
    std::vector<InventoryItem> tmhm;
    std::vector<InventoryItem> items;
    std::vector<InventoryItem> keyItems;
    std::vector<InventoryItem> balls;
    std::vector<InventoryItem> pcItems;
};

[[nodiscard]] InventoryRecord decodeInventory(
    std::span<const uint8_t> payload, RegionLayout region, VersionFamily family);

[[nodiscard]] std::string_view gen2ItemName(uint8_t itemId) noexcept;

} // namespace PokeVault::Integration::Gen2
#endif
