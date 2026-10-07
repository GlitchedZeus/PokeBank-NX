#ifndef POKEBANK_INTEGRATION_GEN3_STAGED_INVENTORY_EDITOR_H
#define POKEBANK_INTEGRATION_GEN3_STAGED_INVENTORY_EDITOR_H

#include "Integration/Gen3/PKSMGen3Adapter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen3 {

struct StagedInventoryChange {
    InventoryPouch pouch = InventoryPouch::Items;
    uint16_t itemId = 0;
    uint16_t before = 0;
    uint16_t after = 0;
    std::string label;
};

class StagedInventoryEditor {
public:
    static std::unique_ptr<StagedInventoryEditor> create(
        std::span<const uint8_t> source, SourceGame game, std::string& error);

    [[nodiscard]] SourceGame sourceGame() const noexcept { return sourceGame_; }
    [[nodiscard]] uint8_t activeSlot() const noexcept { return activeSlot_; }
    [[nodiscard]] uint32_t saveCounter() const noexcept { return saveCounter_; }
    [[nodiscard]] const std::vector<uint8_t>& originalBytes() const noexcept { return original_; }
    [[nodiscard]] const std::vector<uint8_t>& stagedBytes() const noexcept { return staged_; }
    [[nodiscard]] const std::vector<StagedInventoryChange>& pendingChanges() const noexcept {
        return changes_;
    }
    [[nodiscard]] bool hasPendingChanges() const noexcept { return !changes_.empty(); }

    [[nodiscard]] std::vector<InventoryItemRecord> inventoryEntries(
        InventoryPouch pouch, std::string& error) const;
    [[nodiscard]] uint16_t stagedInventoryQuantity(InventoryPouch pouch, uint16_t itemId) const;

    bool stageInventoryQuantity(InventoryPouch pouch, uint16_t itemId, uint16_t quantity,
                                std::string& error);
    void discard() noexcept;
    [[nodiscard]] std::vector<uint8_t> finalizedBytes(std::string& error) const;

private:
    StagedInventoryEditor(std::span<const uint8_t> source, SourceGame game, uint8_t activeSlot,
                          uint32_t saveCounter, const std::array<std::size_t, 14>& offsets);

    bool validateStaged(std::string& error) const;

    SourceGame sourceGame_;
    uint8_t activeSlot_;
    uint32_t saveCounter_;
    std::array<std::size_t, 14> sectorOffsets_{};
    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
    std::vector<StagedInventoryChange> changes_;
};

} // namespace PokeVault::Integration::Gen3

#endif
