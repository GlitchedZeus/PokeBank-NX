#ifndef POKEBANK_INVENTORY_CLASSIC_INVENTORY_CATALOG_H
#define POKEBANK_INVENTORY_CLASSIC_INVENTORY_CATALOG_H

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace PokeVault::Inventory {

enum class ClassicGame : uint8_t {
    Red, Blue, Yellow,
    Gold, Silver, Crystal,
    Ruby, Sapphire, Emerald,
    FireRed, LeafGreen,
};

enum class ClassicPocket : uint8_t {
    Items,
    Medicines,
    Balls,
    KeyItems,
    TMHM,
    Berries,
    PCItems,
};

struct QuantityRule {
    bool editable = true;
    uint16_t minimum = 1;
    uint16_t maximum = 99;
};

std::optional<ClassicGame> classicGameFromId(std::string_view gameId) noexcept;
std::string_view classicGameId(ClassicGame game) noexcept;
std::string_view pocketName(ClassicPocket pocket) noexcept;
std::span<const ClassicPocket> supportedPockets(ClassicGame game) noexcept;
std::span<const uint16_t> addableItems(ClassicGame game, ClassicPocket pocket) noexcept;
bool supportsPocket(ClassicGame game, ClassicPocket pocket) noexcept;
bool isAddableItem(ClassicGame game, ClassicPocket pocket, uint16_t itemId) noexcept;
QuantityRule quantityRule(ClassicGame game, ClassicPocket pocket, uint16_t itemId) noexcept;
bool isStorySensitiveKeyItem(ClassicGame game, ClassicPocket pocket, uint16_t itemId) noexcept;
uint16_t classicMachineMove(ClassicGame game, uint16_t itemId) noexcept;
std::string displayItemName(ClassicGame game, ClassicPocket pocket, uint16_t itemId);

} // namespace PokeVault::Inventory

#endif
