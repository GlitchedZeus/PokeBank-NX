#ifndef POKEBANK_GEN2_READ_ONLY_INVENTORY_H
#define POKEBANK_GEN2_READ_ONLY_INVENTORY_H
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace PokeVault::Integration::Gen2 {
struct InventoryEntry { uint8_t itemId=0; uint8_t quantity=0; };
struct ReadOnlyInventory { bool available=false; std::string error; std::vector<InventoryEntry> tmhm,items,keyItems,balls,pcItems; };
constexpr std::size_t kInventoryCategoryCount=5;
const char* inventoryCategoryName(std::size_t category) noexcept;
ReadOnlyInventory decodeInventory(std::span<const uint8_t> payload, RegionLayout region, VersionFamily family);
bool isValidGen2Item(uint8_t itemId, VersionFamily family, std::size_t category) noexcept;
} // namespace PokeVault::Integration::Gen2
#endif
