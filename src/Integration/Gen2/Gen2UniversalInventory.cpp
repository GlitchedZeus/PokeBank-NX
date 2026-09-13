#include "Integration/Gen2/Gen2StagedEditor.h"

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

constexpr InventoryLayout inventoryLayout(VersionFamily family) noexcept {
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

void writePairs(std::vector<uint8_t>& bytes, std::size_t offset, std::size_t capacity,
                std::span<const PairEntry> entries) {
    bytes[offset] = static_cast<uint8_t>(entries.size());
    std::fill(bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1),
              bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1 + capacity * 2 + 1), 0);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        bytes[offset + 1 + i * 2] = entries[i].id;
        bytes[offset + 2 + i * 2] = entries[i].quantity;
    }
    bytes[offset + 1 + entries.size() * 2] = 0xFF;
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

void writeKeys(std::vector<uint8_t>& bytes, std::size_t offset, std::span<const uint8_t> entries) {
    constexpr std::size_t capacity = kKeyItemCapacity;
    bytes[offset] = static_cast<uint8_t>(entries.size());
    std::fill(bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1),
              bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1 + capacity + 1), 0);
    std::copy(entries.begin(), entries.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1));
    bytes[offset + 1 + entries.size()] = 0xFF;
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

uint8_t quantityIn(std::span<const uint8_t> bytes, std::size_t payloadSize, VersionFamily family,
                   InventoryPocket pocket, uint8_t itemId) {
    if (payloadSize > bytes.size()) return 0;
    const auto record = decodeInventory(bytes.first(payloadSize), RegionLayout::International, family);
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
    if (metadata_.region != RegionLayout::International || metadata_.payloadSize > staged_.size()) {
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
    return quantityIn(staged_, metadata_.payloadSize, metadata_.family, pocket, itemId);
}

bool StagedEditor::stageInventoryQuantity(InventoryPocket pocket, uint8_t itemId, uint8_t quantity,
                                          std::string& error) {
    error.clear();
    if (metadata_.region != RegionLayout::International || metadata_.payloadSize > staged_.size()) {
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

    const uint8_t originalQuantity = quantityIn(original_, metadata_.payloadSize, metadata_.family, pocket, itemId);
    const auto stagedBackup = staged_;
    const auto changesBackup = changes_;
    const auto layout = inventoryLayout(metadata_.family);

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
        auto it = std::find(entries.begin(), entries.end(), itemId);
        if (quantity == 0) {
            if (it != entries.end()) entries.erase(it);
        } else if (it == entries.end()) {
            if (entries.size() >= kKeyItemCapacity) {
                error = "No free slots in this Key Items pocket";
                return false;
            }
            entries.push_back(itemId);
        }
        writeKeys(staged_, layout.keyItems, entries);
    } else {
        const std::size_t offset = pairOffset(layout, pocket);
        const std::size_t capacity = pairCapacity(pocket);
        std::vector<PairEntry> entries;
        if (offset == static_cast<std::size_t>(-1) || capacity == 0 || !readPairs(staged_, offset, capacity, entries)) {
            error = "Generation II staged inventory pair-list is malformed";
            return false;
        }
        auto it = std::find_if(entries.begin(), entries.end(),
            [itemId](const PairEntry& entry) { return entry.id == itemId; });
        if (quantity == 0) {
            if (it != entries.end()) entries.erase(it);
        } else if (it != entries.end()) {
            it->quantity = quantity;
        } else {
            if (entries.size() >= capacity) {
                error = "No free slots in this Generation II pocket";
                return false;
            }
            entries.push_back({itemId, quantity});
        }
        writePairs(staged_, offset, capacity, entries);
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

} // namespace PokeVault::Integration::Gen2
