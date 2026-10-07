#ifndef POKEBANK_INTEGRATION_GEN1_STAGED_INVENTORY_EDITOR_H
#define POKEBANK_INTEGRATION_GEN1_STAGED_INVENTORY_EDITOR_H

#include "Integration/Gen1/Gen1ReadOnlyInventory.h"
#include "Inventory/ClassicInventoryCatalog.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen1 {

struct InventoryChange {
    Inventory::ClassicPocket pocket = Inventory::ClassicPocket::Items;
    uint16_t itemId = 0;
    uint16_t before = 0;
    uint16_t after = 0;
};

// Mutable in-memory sidecar for a strictly parsed Gen I battery save. The original ReadOnlySave
// and its source bytes are never modified; callers receive a separate finalized byte buffer only
// after checksum repair plus strict parser/inventory round-trip verification.
class StagedInventoryEditor final {
public:
    static std::unique_ptr<StagedInventoryEditor> create(const ReadOnlySave& source, std::string& error);

    Inventory::ClassicGame game() const noexcept { return game_; }
    RegionLayout region() const noexcept { return region_; }
    SourceGame sourceGame() const noexcept { return sourceGame_; }
    std::span<const uint8_t> originalBytes() const noexcept { return originalBytes_; }
    std::span<const uint8_t> stagedBytes() const noexcept { return stagedBytes_; }

    std::vector<InventoryEntry> entries(Inventory::ClassicPocket pocket, std::string& error) const;
    bool stageSetQuantity(Inventory::ClassicPocket pocket, uint16_t itemId, uint16_t quantity,
                          std::string& error);
    bool stageRemove(Inventory::ClassicPocket pocket, uint16_t itemId, std::string& error) {
        return stageSetQuantity(pocket, itemId, 0, error);
    }

    bool hasPendingChanges() const noexcept { return !changes_.empty(); }
    const std::vector<InventoryChange>& pendingChanges() const noexcept { return changes_; }
    void discard() noexcept;
    std::vector<uint8_t> finalize(std::string& error) const;

private:
    StagedInventoryEditor(const ReadOnlySave& source, Inventory::ClassicGame game);

    bool decode(ReadOnlyInventory& out, std::string& error) const;
    bool rewrite(bool pc, const std::vector<InventoryEntry>& values, std::string& error);
    void recordChange(Inventory::ClassicPocket pocket, uint16_t itemId, uint16_t before, uint16_t after);

    Inventory::ClassicGame game_;
    RegionLayout region_;
    SourceGame sourceGame_;
    std::vector<uint8_t> originalBytes_;
    std::vector<uint8_t> stagedBytes_;
    std::vector<InventoryChange> changes_;
};

} // namespace PokeVault::Integration::Gen1

#endif
