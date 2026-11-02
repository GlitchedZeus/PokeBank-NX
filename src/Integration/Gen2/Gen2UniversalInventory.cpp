#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen2/Gen2NativeItemList.h"

#include "Inventory/ClassicInventoryCatalog.h"

#include <algorithm>
#include <array>
#include <vector>

namespace PokeVault::Integration::Gen2 {
namespace {

struct InventoryLayout {
    std::size_t tmhm;
    std::size_t items;
    std::size_t keyItems;
    std::size_t balls;
    std::size_t pcItems;
};

// PKHeX 8e3eb3d4 SAV2Offsets PouchTMHM / PouchItem / PouchKey / PouchBall / PouchPC, the offsets
// Gen2ReadOnlyInventory.cpp decodes for each region.
constexpr InventoryLayout inventoryLayout(RegionLayout region, VersionFamily family) noexcept {
    if (region == RegionLayout::Japanese)
        return family == VersionFamily::Crystal
            ? InventoryLayout{0x23C9, 0x2402, 0x242C, 0x2447, 0x2461}
            : InventoryLayout{0x23C7, 0x2400, 0x242A, 0x2445, 0x245F};
    return family == VersionFamily::Crystal
        ? InventoryLayout{0x23E7, 0x2420, 0x244A, 0x2465, 0x247F}
        : InventoryLayout{0x23E6, 0x241F, 0x2449, 0x2464, 0x247E};
}

Inventory::ClassicGame classicGame(SourceGame game) noexcept {
    switch (game) {
        case SourceGame::Gold: return Inventory::ClassicGame::Gold;
        case SourceGame::Silver: return Inventory::ClassicGame::Silver;
        case SourceGame::Crystal: return Inventory::ClassicGame::Crystal;
    }
    return Inventory::ClassicGame::Gold;
}

Inventory::ClassicPocket classicPocket(InventoryPocket pocket) noexcept {
    switch (pocket) {
        case InventoryPocket::TMHM: return Inventory::ClassicPocket::TMHM;
        case InventoryPocket::Items: return Inventory::ClassicPocket::Items;
        case InventoryPocket::KeyItems: return Inventory::ClassicPocket::KeyItems;
        case InventoryPocket::Balls: return Inventory::ClassicPocket::Balls;
        case InventoryPocket::PCItems: return Inventory::ClassicPocket::PCItems;
    }
    return Inventory::ClassicPocket::Items;
}

const std::vector<InventoryItem>* listFor(const InventoryRecord& record, InventoryPocket pocket) noexcept {
    switch (pocket) {
        case InventoryPocket::TMHM: return &record.tmhm;
        case InventoryPocket::Items: return &record.items;
        case InventoryPocket::KeyItems: return &record.keyItems;
        case InventoryPocket::Balls: return &record.balls;
        case InventoryPocket::PCItems: return &record.pcItems;
    }
    return nullptr;
}

std::size_t pairOffset(const InventoryLayout& layout, InventoryPocket pocket) noexcept {
    switch (pocket) {
        case InventoryPocket::Items: return layout.items;
        case InventoryPocket::Balls: return layout.balls;
        case InventoryPocket::PCItems: return layout.pcItems;
        default: return static_cast<std::size_t>(-1);
    }
}

std::size_t pairCapacity(InventoryPocket pocket) noexcept {
    switch (pocket) {
        case InventoryPocket::Items: return 20;
        case InventoryPocket::Balls: return 12;
        case InventoryPocket::PCItems: return 50;
        default: return 0;
    }
}

struct PairEntry { uint8_t id; uint8_t quantity; };

bool readPairs(std::span<const uint8_t> bytes, std::size_t offset, std::size_t capacity,
               std::vector<PairEntry>& out) noexcept {
    if (offset >= bytes.size() || 1 + capacity * 2 + 1 > bytes.size() - offset) return false;
    const uint8_t count = bytes[offset];
    if (count > capacity || bytes[offset + 1 + count * 2] != 0xFF) return false;
    out.clear();
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const uint8_t id = bytes[offset + 1 + i * 2];
        const uint8_t qty = bytes[offset + 2 + i * 2];
        if (id == 0 || id == 0xFF || qty == 0 || qty > 99) return false;
        out.push_back({id, qty});
    }
    return true;
}

constexpr std::size_t kKeyItemCapacity = 25;

bool readKeys(std::span<const uint8_t> bytes, std::size_t offset, std::vector<uint8_t>& out) noexcept {
    constexpr std::size_t capacity = kKeyItemCapacity;
    if (offset >= bytes.size() || 1 + capacity + 1 > bytes.size() - offset) return false;
    const uint8_t count = bytes[offset];
    if (count > capacity || bytes[offset + 1 + count] != 0xFF) return false;
    out.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1),
               bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1 + count));
    return std::none_of(out.begin(), out.end(), [](uint8_t id) { return id == 0 || id == 0xFF; });
}

Inventory::ClassicPocket semanticPocket(Inventory::ClassicGame game, InventoryPocket physical,
                                        uint16_t itemId) noexcept {
    if (physical != InventoryPocket::PCItems) return classicPocket(physical);
    if (Inventory::isAddableItem(game, Inventory::ClassicPocket::KeyItems, itemId))
        return Inventory::ClassicPocket::KeyItems;
    if (Inventory::isAddableItem(game, Inventory::ClassicPocket::TMHM, itemId))
        return Inventory::ClassicPocket::TMHM;
    if (Inventory::isAddableItem(game, Inventory::ClassicPocket::Balls, itemId))
        return Inventory::ClassicPocket::Balls;
    return Inventory::ClassicPocket::Items;
}

uint8_t quantityIn(std::span<const uint8_t> bytes, std::size_t payloadSize, RegionLayout region,
                   VersionFamily family, InventoryPocket pocket, uint8_t itemId) {
    if (payloadSize > bytes.size()) return 0;
    const auto record = decodeInventory(bytes.first(payloadSize), region, family);
    if (!record.available) return 0;
    const auto* list = listFor(record, pocket);
    if (!list) return 0;
    const auto it = std::find_if(list->begin(), list->end(),
        [itemId](const InventoryItem& item) { return item.itemId == itemId; });
    return it == list->end() ? 0 : it->quantity;
}

} // namespace

std::vector<InventoryItem> StagedEditor::inventoryEntries(InventoryPocket pocket, std::string& error) const {
    error.clear();
    if (metadata_.payloadSize > staged_.size()) {
        error = "Generation II staged inventory is unavailable for this save layout";
        return {};
    }
    const auto game = classicGame(sourceGame_);
    const auto catalogPocket = classicPocket(pocket);
    if (!Inventory::supportsPocket(game, catalogPocket)) {
        error = "Inventory category is not supported by this Generation II game";
        return {};
    }
    const auto record = decodeInventory(std::span<const uint8_t>(staged_).first(metadata_.payloadSize),
                                        metadata_.region, metadata_.family);
    if (!record.available) {
        error = record.detail.empty() ? "Generation II staged inventory is malformed" : record.detail;
        return {};
    }
    const auto* list = listFor(record, pocket);
    return list ? *list : std::vector<InventoryItem>{};
}

uint8_t StagedEditor::stagedInventoryQuantity(InventoryPocket pocket, uint8_t itemId) const {
    return quantityIn(staged_, metadata_.payloadSize, metadata_.region, metadata_.family, pocket, itemId);
}

bool StagedEditor::stageInventoryQuantity(InventoryPocket pocket, uint8_t itemId, uint8_t quantity,
                                          std::string& error) {
    error.clear();
    if (refuseWhileCarrying(error)) return false;
    if (metadata_.payloadSize > staged_.size()) {
        error = "Generation II staged inventory is unavailable for this save layout";
        return false;
    }
    const auto game = classicGame(sourceGame_);
    const auto physicalPocket = classicPocket(pocket);
    if (!Inventory::isAddableItem(game, physicalPocket, itemId)) {
        error = "Item is not valid for this exact Generation II game and pocket";
        return false;
    }
    const auto logicalPocket = semanticPocket(game, pocket, itemId);
    const auto rule = Inventory::quantityRule(game, logicalPocket, itemId);
    if (quantity != 0 && (quantity < rule.minimum || quantity > rule.maximum)) {
        error = "Item quantity is outside the valid range for this Generation II item";
        return false;
    }

    const uint8_t originalQuantity = quantityIn(original_, metadata_.payloadSize, metadata_.region, metadata_.family,
                                                pocket, itemId);
    const auto stagedBackup = staged_;
    const auto changesBackup = changes_;
    const auto layout = inventoryLayout(metadata_.region, metadata_.family);

    if (pocket == InventoryPocket::TMHM) {
        const auto machines = Inventory::addableItems(game, Inventory::ClassicPocket::TMHM);
        const auto it = std::find(machines.begin(), machines.end(), itemId);
        if (it == machines.end()) {
            error = "Machine is not valid for this Generation II game";
            return false;
        }
        const std::size_t index = static_cast<std::size_t>(it - machines.begin());
        if (layout.tmhm + index >= metadata_.payloadSize) {
            error = "Generation II TM/HM pouch exceeds the staged payload";
            return false;
        }
        staged_[layout.tmhm + index] = quantity;
    } else if (pocket == InventoryPocket::KeyItems) {
        std::vector<uint8_t> entries;
        if (!readKeys(staged_, layout.keyItems, entries)) {
            error = "Generation II staged Key Items list is malformed";
            return false;
        }
        if (quantity != 0 && entries.size() >= kKeyItemCapacity &&
            std::find(entries.begin(), entries.end(), itemId) == entries.end()) {
            error = "No free slots in this Key Items pocket";
            return false;
        }
        // Byte-surgical (Q07): a key item is present (1) or absent (0).
        if (!NativeItemList::set(staged_, layout.keyItems, kKeyItemCapacity, 1, itemId,
                                 quantity != 0 ? 1 : 0, error)) return false;
    } else {
        const std::size_t offset = pairOffset(layout, pocket);
        const std::size_t capacity = pairCapacity(pocket);
        std::vector<PairEntry> entries;
        if (offset == static_cast<std::size_t>(-1) || capacity == 0 || !readPairs(staged_, offset, capacity, entries)) {
            error = "Generation II staged inventory pair-list is malformed";
            return false;
        }
        const bool present = std::any_of(entries.begin(), entries.end(),
            [itemId](const PairEntry& entry) { return entry.id == itemId; });
        if (quantity != 0 && !present && entries.size() >= capacity) {
            error = "No free slots in this Generation II pocket";
            return false;
        }
        // Byte-surgical (Q07): only the entry, the shifted tail and the count change.
        if (!NativeItemList::set(staged_, offset, capacity, 2, itemId, quantity, error)) return false;
    }

    const auto record = decodeInventory(std::span<const uint8_t>(staged_).first(metadata_.payloadSize),
                                        metadata_.region, metadata_.family);
    if (!record.available || stagedInventoryQuantity(pocket, itemId) != quantity) {
        staged_ = stagedBackup;
        changes_ = changesBackup;
        error = "Generation II inventory edit failed semantic round-trip validation";
        return false;
    }

    const std::string key = "inventory:" + std::to_string(static_cast<unsigned>(pocket)) +
                            ":" + std::to_string(itemId);
    const std::string label = std::string(Inventory::pocketName(physicalPocket)) + ": " +
                              Inventory::displayItemName(game, logicalPocket, itemId);
    setChange(key, label, std::to_string(originalQuantity), std::to_string(quantity));
    return true;
}

std::size_t StagedEditor::inventoryCapacity(InventoryPocket pocket) const {
    if (pocket == InventoryPocket::KeyItems) return kKeyItemCapacity;
    if (pocket == InventoryPocket::TMHM)
        return Inventory::addableItems(classicGame(sourceGame_), Inventory::ClassicPocket::TMHM).size();
    return pairCapacity(pocket);
}

bool StagedEditor::stageInventorySlotQuantity(InventoryPocket pocket, std::size_t slot, uint8_t itemId,
                                              uint8_t quantity, std::string& error) {
    error.clear();
    if (refuseWhileCarrying(error)) return false;
    if (metadata_.payloadSize > staged_.size()) {
        error = "Generation II staged inventory is unavailable for this save layout";
        return false;
    }
    const auto layout = inventoryLayout(metadata_.region, metadata_.family);
    const std::size_t offset = pairOffset(layout, pocket);
    const std::size_t capacity = pairCapacity(pocket);
    if (offset == static_cast<std::size_t>(-1) || capacity == 0) {
        error = "Only Items, Balls and PC Items hold separate Generation II stacks";
        return false;
    }
    const auto game = classicGame(sourceGame_);
    if (!Inventory::isAddableItem(game, classicPocket(pocket), itemId)) {
        error = "Item is not valid for this exact Generation II game and pocket";
        return false;
    }
    const auto logicalPocket = semanticPocket(game, pocket, itemId);
    const auto rule = Inventory::quantityRule(game, logicalPocket, itemId);
    if (quantity == 0 || quantity < rule.minimum || quantity > rule.maximum) {
        error = "Item quantity is outside the valid range for this Generation II item";
        return false;
    }
    std::vector<PairEntry> entries;
    if (!readPairs(staged_, offset, capacity, entries)) {
        error = "Generation II staged inventory pair-list is malformed";
        return false;
    }
    if (slot >= entries.size() || entries[slot].id != itemId) {
        error = "Generation II item stack changed; edit it again";
        return false;
    }
    std::size_t first = 0;
    while (first < entries.size() && entries[first].id != itemId) ++first;
    uint8_t before = 0;
    if (first == slot) {
        before = quantityIn(original_, metadata_.payloadSize, metadata_.region, metadata_.family, pocket, itemId);
    } else {
        std::vector<PairEntry> original;
        if (readPairs(original_, offset, capacity, original) && slot < original.size() &&
            original[slot].id == itemId)
            before = original[slot].quantity;
    }
    const auto stagedBackup = staged_;
    staged_[offset + 2 + slot * 2] = quantity;   // byte-surgical: this stack's quantity only
    std::vector<PairEntry> check;
    const auto record = decodeInventory(std::span<const uint8_t>(staged_).first(metadata_.payloadSize),
                                        metadata_.region, metadata_.family);
    if (!record.available || !readPairs(staged_, offset, capacity, check) || check.size() != entries.size() ||
        check[slot].id != itemId || check[slot].quantity != quantity) {
        staged_ = stagedBackup;
        error = "Generation II inventory edit failed semantic round-trip validation";
        return false;
    }
    // The first stack keeps the per-item Review row every other Bag edit uses; a further stack of
    // the same item gets its own row.
    std::string key = "inventory:" + std::to_string(static_cast<unsigned>(pocket)) + ":" + std::to_string(itemId);
    std::string label = std::string(Inventory::pocketName(classicPocket(pocket))) + ": " +
                        Inventory::displayItemName(game, logicalPocket, itemId);
    if (first != slot) {
        key += ":slot:" + std::to_string(slot);
        label += " (stack " + std::to_string(slot + 1) + ")";
    }
    setChange(key, label, std::to_string(before), std::to_string(quantity));
    return true;
}

} // namespace PokeVault::Integration::Gen2
