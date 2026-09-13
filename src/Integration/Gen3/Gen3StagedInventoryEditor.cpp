#include "Integration/Gen3/Gen3StagedInventoryEditor.h"

#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Inventory/ClassicInventoryCatalog.h"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen3 {
namespace {

using Inventory::ClassicGame;
using Inventory::ClassicPocket;

struct PouchDefinition {
    InventoryPouch pouch;
    ClassicPocket catalogPocket;
    std::size_t offset;
    uint16_t capacity;
    bool keyed;
};

constexpr std::array<PouchDefinition, 6> kRSPouches{{
    {InventoryPouch::Items,      ClassicPocket::Items,    0x0560, 20, false},
    {InventoryPouch::KeyItems,   ClassicPocket::KeyItems, 0x05B0, 20, false},
    {InventoryPouch::PokeBalls,  ClassicPocket::Balls,    0x0600, 16, false},
    {InventoryPouch::TMCase,     ClassicPocket::TMHM,     0x0640, 64, false},
    {InventoryPouch::BerryPouch, ClassicPocket::Berries,  0x0740, 46, false},
    {InventoryPouch::PCItems,    ClassicPocket::PCItems,  0x0498, 50, false},
}};

constexpr std::array<PouchDefinition, 6> kEmeraldPouches{{
    {InventoryPouch::Items,      ClassicPocket::Items,    0x0560, 30, true},
    {InventoryPouch::KeyItems,   ClassicPocket::KeyItems, 0x05D8, 30, true},
    {InventoryPouch::PokeBalls,  ClassicPocket::Balls,    0x0650, 16, true},
    {InventoryPouch::TMCase,     ClassicPocket::TMHM,     0x0690, 64, true},
    {InventoryPouch::BerryPouch, ClassicPocket::Berries,  0x0790, 46, true},
    {InventoryPouch::PCItems,    ClassicPocket::PCItems,  0x0498, 50, false},
}};

constexpr std::array<PouchDefinition, 6> kFRLGPouches{{
    {InventoryPouch::Items,      ClassicPocket::Items,    0x0310, 42, true},
    {InventoryPouch::KeyItems,   ClassicPocket::KeyItems, 0x03B8, 30, true},
    {InventoryPouch::PokeBalls,  ClassicPocket::Balls,    0x0430, 13, true},
    {InventoryPouch::TMCase,     ClassicPocket::TMHM,     0x0464, 58, true},
    {InventoryPouch::BerryPouch, ClassicPocket::Berries,  0x054C, 43, true},
    {InventoryPouch::PCItems,    ClassicPocket::PCItems,  0x0298, 30, false},
}};

ClassicGame classicGame(SourceGame game) noexcept {
    switch (game) {
        case SourceGame::RubyGBA: return ClassicGame::Ruby;
        case SourceGame::SapphireGBA: return ClassicGame::Sapphire;
        case SourceGame::EmeraldGBA: return ClassicGame::Emerald;
        case SourceGame::FireRedGBA: return ClassicGame::FireRed;
        case SourceGame::LeafGreenGBA: return ClassicGame::LeafGreen;
    }
    return ClassicGame::Ruby;
}

std::span<const PouchDefinition> definitions(SourceGame game) noexcept {
    if (game == SourceGame::EmeraldGBA) return kEmeraldPouches;
    if (Detail::isFRLG(game)) return kFRLGPouches;
    return kRSPouches;
}

const PouchDefinition* definitionFor(SourceGame game, InventoryPouch pouch) noexcept {
    const auto defs = definitions(game);
    const auto it = std::find_if(defs.begin(), defs.end(),
        [pouch](const PouchDefinition& definition) { return definition.pouch == pouch; });
    return it == defs.end() ? nullptr : &*it;
}

ClassicPocket semanticPocket(ClassicGame game, InventoryPouch physical, uint16_t itemId) noexcept {
    if (physical != InventoryPouch::PCItems) {
        switch (physical) {
            case InventoryPouch::Items: return ClassicPocket::Items;
            case InventoryPouch::KeyItems: return ClassicPocket::KeyItems;
            case InventoryPouch::PokeBalls: return ClassicPocket::Balls;
            case InventoryPouch::TMCase: return ClassicPocket::TMHM;
            case InventoryPouch::BerryPouch: return ClassicPocket::Berries;
            case InventoryPouch::PCItems: break;
        }
    }
    if (Inventory::isAddableItem(game, ClassicPocket::KeyItems, itemId)) return ClassicPocket::KeyItems;
    if (Inventory::isAddableItem(game, ClassicPocket::TMHM, itemId)) return ClassicPocket::TMHM;
    if (Inventory::isAddableItem(game, ClassicPocket::Balls, itemId)) return ClassicPocket::Balls;
    if (Inventory::isAddableItem(game, ClassicPocket::Berries, itemId)) return ClassicPocket::Berries;
    return ClassicPocket::Items;
}

uint16_t read16(std::span<const uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<uint16_t>(bytes[offset]) |
           static_cast<uint16_t>(bytes[offset + 1] << 8);
}

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) noexcept {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}

bool readLogical(std::span<const uint8_t> bytes,
                 const std::array<std::size_t, 14>& sectors,
                 uint8_t firstSector, std::size_t logical,
                 std::span<uint8_t> output) noexcept {
    for (std::size_t index = 0; index < output.size(); ++index) {
        const std::size_t position = logical + index;
        const std::size_t sectorId = static_cast<std::size_t>(firstSector) +
            position / Detail::kSectorDataSize;
        if (sectorId >= sectors.size()) return false;
        const std::size_t absolute = sectors[sectorId] + position % Detail::kSectorDataSize;
        if (absolute >= bytes.size()) return false;
        output[index] = bytes[absolute];
    }
    return true;
}

bool writeLogical(std::span<uint8_t> bytes,
                  const std::array<std::size_t, 14>& sectors,
                  uint8_t firstSector, std::size_t logical,
                  std::span<const uint8_t> input,
                  std::set<uint8_t>& touched) noexcept {
    for (std::size_t index = 0; index < input.size(); ++index) {
        const std::size_t position = logical + index;
        const std::size_t sectorId = static_cast<std::size_t>(firstSector) +
            position / Detail::kSectorDataSize;
        if (sectorId >= sectors.size()) return false;
        const std::size_t absolute = sectors[sectorId] + position % Detail::kSectorDataSize;
        if (absolute >= bytes.size()) return false;
        bytes[absolute] = input[index];
        touched.insert(static_cast<uint8_t>(sectorId));
    }
    return true;
}

uint16_t securityKey16(std::span<const uint8_t> bytes, SourceGame game,
                       const std::array<std::size_t, 14>& sectors) noexcept {
    if (game == SourceGame::EmeraldGBA)
        return read16(bytes, sectors[0] + 0xAC);
    if (Detail::isFRLG(game))
        return read16(bytes, sectors[0] + 0xF20);
    return 0;
}

bool decodePouch(std::span<const uint8_t> bytes, SourceGame game,
                 const std::array<std::size_t, 14>& sectors,
                 const PouchDefinition& definition,
                 std::vector<InventoryItemRecord>& out,
                 std::string& error) {
    error.clear();
    out.clear();
    out.reserve(definition.capacity);
    const uint16_t key16 = definition.keyed ? securityKey16(bytes, game, sectors) : 0;
    for (uint16_t slot = 0; slot < definition.capacity; ++slot) {
        std::array<uint8_t, 4> entry{};
        if (!readLogical(bytes, sectors, 1,
                         definition.offset + static_cast<std::size_t>(slot) * 4, entry)) {
            error = "Generation III inventory slot crosses an invalid logical sector";
            return false;
        }
        const uint16_t itemId = read16(entry, 0);
        const uint16_t quantity = static_cast<uint16_t>(read16(entry, 2) ^ key16);
        if (itemId == 0 || quantity == 0) continue;
        if (itemId > 376 || quantity > 999) {
            error = "Generation III staged inventory contains an invalid item id or quantity";
            return false;
        }
        out.push_back({itemId, quantity});
    }
    return true;
}

void repairChecksums(std::vector<uint8_t>& bytes,
                     const std::array<std::size_t, 14>& sectors,
                     const std::set<uint8_t>& touched) {
    for (const uint8_t id : touched) {
        const std::size_t sector = sectors[id];
        const uint16_t checksum = Detail::sectorChecksum(
            std::span<const uint8_t>(bytes).subspan(sector, Detail::kChunkLengths[id]));
        write16(bytes, sector + 0xFF6, checksum);
    }
}

std::string pouchLabel(InventoryPouch pouch) {
    switch (pouch) {
        case InventoryPouch::Items: return "Items";
        case InventoryPouch::KeyItems: return "Key Items";
        case InventoryPouch::PokeBalls: return "Poké Balls";
        case InventoryPouch::TMCase: return "TM/HM";
        case InventoryPouch::BerryPouch: return "Berries";
        case InventoryPouch::PCItems: return "PC Items";
    }
    return "Inventory";
}

} // namespace

StagedInventoryEditor::StagedInventoryEditor(
    std::span<const uint8_t> source, SourceGame game, uint8_t activeSlot,
    uint32_t saveCounter, const std::array<std::size_t, 14>& offsets)
    : sourceGame_(game), activeSlot_(activeSlot), saveCounter_(saveCounter), sectorOffsets_(offsets),
      original_(source.begin(), source.end()), staged_(source.begin(), source.end()) {}

std::unique_ptr<StagedInventoryEditor> StagedInventoryEditor::create(
    std::span<const uint8_t> source, SourceGame game, std::string& error) {
    error.clear();
    if (source.size() != Detail::kSaveSize) {
        error = "Generation III staged save must be exactly 128 KiB";
        return nullptr;
    }
    if (!Detail::sourceGameSupported(game)) {
        error = "Unsupported Generation III source identity";
        return nullptr;
    }
    const Detail::SlotValidation slots[2] = {
        Detail::validateSlot(source, 0), Detail::validateSlot(source, 1)};
    if (!slots[0].valid && !slots[1].valid) {
        error = "No checksum-valid Generation III save slot is available for staged editing";
        return nullptr;
    }
    const uint8_t active = Detail::selectActiveSlot(slots);
    const auto& selected = slots[active];
    if (!Detail::familyMatches(game, Detail::detectFamily(source, selected))) {
        error = "Generation III save family does not match the selected exact game";
        return nullptr;
    }

    auto editor = std::unique_ptr<StagedInventoryEditor>(
        new StagedInventoryEditor(source, game, active, selected.counter,
                                  selected.logicalSectorOffsets));
    for (const auto& definition : definitions(game)) {
        std::vector<InventoryItemRecord> entries;
        if (!decodePouch(editor->staged_, game, editor->sectorOffsets_, definition, entries, error))
            return nullptr;
    }
    return editor;
}

std::vector<InventoryItemRecord> StagedInventoryEditor::inventoryEntries(
    InventoryPouch pouch, std::string& error) const {
    error.clear();
    const auto* definition = definitionFor(sourceGame_, pouch);
    if (!definition) {
        error = "Generation III inventory category is not supported";
        return {};
    }
    std::vector<InventoryItemRecord> out;
    if (!decodePouch(staged_, sourceGame_, sectorOffsets_, *definition, out, error)) return {};
    return out;
}

uint16_t StagedInventoryEditor::stagedInventoryQuantity(InventoryPouch pouch, uint16_t itemId) const {
    std::string error;
    const auto entries = inventoryEntries(pouch, error);
    if (!error.empty()) return 0;
    const auto it = std::find_if(entries.begin(), entries.end(),
        [itemId](const InventoryItemRecord& item) { return item.itemId == itemId; });
    return it == entries.end() ? 0 : it->count;
}

bool StagedInventoryEditor::stageInventoryQuantity(
    InventoryPouch pouch, uint16_t itemId, uint16_t quantity, std::string& error) {
    error.clear();
    const auto* definition = definitionFor(sourceGame_, pouch);
    if (!definition) {
        error = "Generation III inventory category is not supported";
        return false;
    }
    const ClassicGame game = classicGame(sourceGame_);
    if (!Inventory::isAddableItem(game, definition->catalogPocket, itemId)) {
        error = "Item is not valid for this exact Generation III game and pocket";
        return false;
    }
    const ClassicPocket logicalPocket = semanticPocket(game, pouch, itemId);
    const auto rule = Inventory::quantityRule(game, logicalPocket, itemId);
    if (quantity != 0 && (quantity < rule.minimum || quantity > rule.maximum)) {
        error = "Item quantity is outside the valid range for this Generation III item";
        return false;
    }

    std::vector<InventoryItemRecord> entries;
    if (!decodePouch(staged_, sourceGame_, sectorOffsets_, *definition, entries, error)) return false;
    const uint16_t before = stagedInventoryQuantity(pouch, itemId);

    const auto first = std::find_if(entries.begin(), entries.end(),
        [itemId](const InventoryItemRecord& item) { return item.itemId == itemId; });
    if (quantity == 0) {
        entries.erase(std::remove_if(entries.begin(), entries.end(),
            [itemId](const InventoryItemRecord& item) { return item.itemId == itemId; }), entries.end());
    } else if (first != entries.end()) {
        first->count = quantity;
        entries.erase(std::remove_if(std::next(first), entries.end(),
            [itemId](const InventoryItemRecord& item) { return item.itemId == itemId; }), entries.end());
    } else {
        if (entries.size() >= definition->capacity) {
            error = "No free slots in this Generation III inventory pocket";
            return false;
        }
        entries.push_back({itemId, quantity});
    }

    const auto stagedBackup = staged_;
    const auto changesBackup = changes_;
    const uint16_t key16 = definition->keyed ? securityKey16(staged_, sourceGame_, sectorOffsets_) : 0;
    std::set<uint8_t> touched;
    for (uint16_t slot = 0; slot < definition->capacity; ++slot) {
        std::array<uint8_t, 4> raw{};
        if (slot < entries.size()) {
            write16(raw, 0, entries[slot].itemId);
            write16(raw, 2, static_cast<uint16_t>(entries[slot].count ^ key16));
        } else {
            write16(raw, 0, 0);
            write16(raw, 2, key16); // semantic quantity zero for keyed empty slots
        }
        if (!writeLogical(staged_, sectorOffsets_, 1,
                          definition->offset + static_cast<std::size_t>(slot) * 4,
                          raw, touched)) {
            staged_ = stagedBackup;
            error = "Generation III inventory write crossed an invalid logical sector";
            return false;
        }
    }
    repairChecksums(staged_, sectorOffsets_, touched);

    if (!validateStaged(error) || stagedInventoryQuantity(pouch, itemId) != quantity) {
        staged_ = stagedBackup;
        changes_ = changesBackup;
        if (error.empty()) error = "Generation III inventory edit failed semantic round-trip validation";
        return false;
    }

    const std::string key = std::to_string(static_cast<unsigned>(pouch)) + ":" +
                            std::to_string(itemId);
    const std::string label = pouchLabel(pouch) + ": " +
        Inventory::displayItemName(game, logicalPocket, itemId);
    changes_.erase(std::remove_if(changes_.begin(), changes_.end(),
        [&](const StagedInventoryChange& change) {
            return std::to_string(static_cast<unsigned>(change.pouch)) + ":" +
                   std::to_string(change.itemId) == key;
        }), changes_.end());
    if (before != quantity) changes_.push_back({pouch, itemId, before, quantity, label});
    return true;
}

bool StagedInventoryEditor::validateStaged(std::string& error) const {
    error.clear();
    const auto selected = Detail::validateSlot(staged_, activeSlot_);
    if (!selected.valid) {
        error = "Generation III staged save failed sector/checksum validation";
        return false;
    }
    if (selected.counter != saveCounter_ || selected.logicalSectorOffsets != sectorOffsets_) {
        error = "Generation III staged edit changed save counter or rotating-sector identity";
        return false;
    }
    if (!Detail::familyMatches(sourceGame_, Detail::detectFamily(staged_, selected))) {
        error = "Generation III staged edit changed the validated save family";
        return false;
    }
    for (const auto& definition : definitions(sourceGame_)) {
        std::vector<InventoryItemRecord> entries;
        if (!decodePouch(staged_, sourceGame_, sectorOffsets_, definition, entries, error)) return false;
    }
    const uint8_t inactive = activeSlot_ == 0 ? 1 : 0;
    const std::size_t inactiveBase = Detail::kSlotBases[inactive];
    const std::size_t inactiveLength = Detail::kSectorCount * Detail::kSectorSize;
    if (!std::equal(original_.begin() + static_cast<std::ptrdiff_t>(inactiveBase),
                    original_.begin() + static_cast<std::ptrdiff_t>(inactiveBase + inactiveLength),
                    staged_.begin() + static_cast<std::ptrdiff_t>(inactiveBase))) {
        error = "Generation III staged edit modified the inactive save slot";
        return false;
    }
    return true;
}

void StagedInventoryEditor::discard() noexcept {
    staged_ = original_;
    changes_.clear();
}

std::vector<uint8_t> StagedInventoryEditor::finalizedBytes(std::string& error) const {
    if (!validateStaged(error)) return {};
    if (staged_.size() != original_.size()) {
        error = "Generation III staged save size changed unexpectedly";
        return {};
    }
    return staged_;
}

} // namespace PokeVault::Integration::Gen3
