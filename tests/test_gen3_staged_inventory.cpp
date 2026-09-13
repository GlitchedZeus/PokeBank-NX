#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Inventory/ClassicInventoryCatalog.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace PokeVault;
using namespace PokeVault::Integration::Gen3;

namespace {

enum class Family { RS, Emerald, FRLG };
using LogicalSectors = std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}

void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}

uint16_t read16(std::span<const uint8_t> bytes, std::size_t offset) {
    return static_cast<uint16_t>(bytes[offset]) |
           static_cast<uint16_t>(bytes[offset + 1] << 8);
}

void putEntry(LogicalSectors& logical, std::size_t offset, uint16_t item, uint16_t quantity,
              uint16_t key16) {
    write16(logical[1], offset, item);
    write16(logical[1], offset + 2, static_cast<uint16_t>(quantity ^ key16));
}

LogicalSectors makeLogical(Family family) {
    LogicalSectors logical{};
    uint32_t key = 0;
    if (family == Family::FRLG) {
        write32(logical[0], 0xAC, 1);
        key = 0xA1B2C3D4;
        write32(logical[0], 0xF20, key);
        putEntry(logical, 0x0310, 13, 3, static_cast<uint16_t>(key));
        putEntry(logical, 0x03B8, 260, 1, static_cast<uint16_t>(key));
        putEntry(logical, 0x0430, 4, 7, static_cast<uint16_t>(key));
        putEntry(logical, 0x0464, 289, 2, static_cast<uint16_t>(key));
        putEntry(logical, 0x054C, 133, 5, static_cast<uint16_t>(key));
        putEntry(logical, 0x0298, 13, 9, 0);
    } else if (family == Family::Emerald) {
        key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, key);
        logical[0][0xEE0] = 0x42;
        putEntry(logical, 0x0560, 13, 3, static_cast<uint16_t>(key));
        putEntry(logical, 0x05D8, 259, 1, static_cast<uint16_t>(key));
        putEntry(logical, 0x0650, 4, 7, static_cast<uint16_t>(key));
        putEntry(logical, 0x0690, 289, 2, static_cast<uint16_t>(key));
        putEntry(logical, 0x0790, 133, 5, static_cast<uint16_t>(key));
        putEntry(logical, 0x0498, 13, 9, 0);
    } else {
        write32(logical[0], 0xAC, 0);
        putEntry(logical, 0x0560, 13, 3, 0);
        putEntry(logical, 0x05B0, 259, 1, 0);
        putEntry(logical, 0x0600, 4, 7, 0);
        putEntry(logical, 0x0640, 289, 2, 0);
        putEntry(logical, 0x0740, 133, 5, 0);
        putEntry(logical, 0x0498, 13, 9, 0);
    }
    return logical;
}

void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter,
               uint8_t rotation, Family family) {
    auto logical = makeLogical(family);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
        const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6, Detail::sectorChecksum(
            std::span<const uint8_t>(save).subspan(offset, Detail::kChunkLengths[id])));
        write32(save, offset + 0xFF8, Detail::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}

std::vector<uint8_t> fixture(Family family) {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family);
    writeSlot(save, 1, 9, 5, family);
    return save;
}

std::size_t sectorFor(std::span<const uint8_t> bytes, uint8_t slot, uint16_t wanted) {
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        if (read16(bytes, offset + 0xFF4) == wanted) return offset;
    }
    assert(false && "logical sector missing");
    return 0;
}

std::size_t pouchOffset(Family family, InventoryPouch pouch) {
    if (family == Family::FRLG) {
        switch (pouch) {
            case InventoryPouch::Items: return 0x0310;
            case InventoryPouch::KeyItems: return 0x03B8;
            case InventoryPouch::PokeBalls: return 0x0430;
            case InventoryPouch::TMCase: return 0x0464;
            case InventoryPouch::BerryPouch: return 0x054C;
            case InventoryPouch::PCItems: return 0x0298;
        }
    }
    if (family == Family::Emerald) {
        switch (pouch) {
            case InventoryPouch::Items: return 0x0560;
            case InventoryPouch::KeyItems: return 0x05D8;
            case InventoryPouch::PokeBalls: return 0x0650;
            case InventoryPouch::TMCase: return 0x0690;
            case InventoryPouch::BerryPouch: return 0x0790;
            case InventoryPouch::PCItems: return 0x0498;
        }
    }
    switch (pouch) {
        case InventoryPouch::Items: return 0x0560;
        case InventoryPouch::KeyItems: return 0x05B0;
        case InventoryPouch::PokeBalls: return 0x0600;
        case InventoryPouch::TMCase: return 0x0640;
        case InventoryPouch::BerryPouch: return 0x0740;
        case InventoryPouch::PCItems: return 0x0498;
    }
    return 0;
}

uint16_t rawStoredQuantity(const std::vector<uint8_t>& bytes, uint8_t activeSlot,
                           Family family, InventoryPouch pouch, uint16_t itemId) {
    const std::size_t section = sectorFor(bytes, activeSlot, 1);
    const std::size_t start = pouchOffset(family, pouch);
    const uint16_t capacity = pouch == InventoryPouch::PokeBalls ? (family == Family::FRLG ? 13 : 16) :
        pouch == InventoryPouch::TMCase ? (family == Family::FRLG ? 58 : 64) :
        pouch == InventoryPouch::BerryPouch ? (family == Family::FRLG ? 43 : 46) :
        pouch == InventoryPouch::PCItems ? (family == Family::FRLG ? 30 : 50) :
        pouch == InventoryPouch::Items ? (family == Family::FRLG ? 42 : family == Family::Emerald ? 30 : 20) :
        (family == Family::FRLG || family == Family::Emerald ? 30 : 20);
    for (uint16_t slot = 0; slot < capacity; ++slot) {
        const std::size_t offset = section + start + static_cast<std::size_t>(slot) * 4;
        if (read16(bytes, offset) == itemId) return read16(bytes, offset + 2);
    }
    return 0;
}

void runExactGame(SourceGame game, Family family) {
    auto source = fixture(family);
    const auto sourceCopy = source;
    std::string error;
    auto editor = StagedInventoryEditor::create(source, game, error);
    assert(editor && error.empty());
    assert(editor->activeSlot() == 1 && editor->saveCounter() == 9);
    assert(editor->stagedInventoryQuantity(InventoryPouch::Items, 13) == 3);

    assert(editor->stageInventoryQuantity(InventoryPouch::Items, 13, 50, error));
    assert(editor->stageInventoryQuantity(InventoryPouch::PokeBalls, 2, 10, error));
    assert(editor->stageInventoryQuantity(InventoryPouch::KeyItems, 260, 1, error));
    assert(editor->stageInventoryQuantity(InventoryPouch::TMCase, 290, 6, error));
    assert(editor->stageInventoryQuantity(InventoryPouch::BerryPouch, 134, 8, error));
    assert(editor->stageInventoryQuantity(InventoryPouch::PCItems, 14, 9, error));

    assert(!editor->stageInventoryQuantity(InventoryPouch::PokeBalls, 13, 1, error));
    assert(!editor->stageInventoryQuantity(InventoryPouch::Items, 500, 1, error));
    assert(!editor->stageInventoryQuantity(InventoryPouch::TMCase, 339, 2, error));
    assert(editor->stageInventoryQuantity(InventoryPouch::TMCase, 339, 1, error));

    assert(editor->stageInventoryQuantity(InventoryPouch::Items, 13, 25, error));
    auto items = editor->inventoryEntries(InventoryPouch::Items, error);
    assert(error.empty());
    assert(std::count_if(items.begin(), items.end(), [](const InventoryItemRecord& item) {
        return item.itemId == 13;
    }) == 1);
    assert(editor->stageInventoryQuantity(InventoryPouch::Items, 13, 0, error));
    assert(editor->stagedInventoryQuantity(InventoryPouch::Items, 13) == 0);

    auto finalized = editor->finalizedBytes(error);
    assert(!finalized.empty() && error.empty());
    assert(source == sourceCopy); // caller/source bytes are immutable
    const auto selected = Detail::validateSlot(finalized, 1);
    assert(selected.valid && selected.counter == 9);
    assert(Detail::familyMatches(game, Detail::detectFamily(finalized, selected)));

    // Inactive slot and every active logical sector except inventory section 1 are byte-identical.
    assert(std::equal(source.begin(), source.begin() + 0xE000, finalized.begin()));
    for (uint16_t id = 0; id < Detail::kSectorCount; ++id) {
        if (id == 1) continue;
        const std::size_t before = sectorFor(source, 1, id);
        const std::size_t after = sectorFor(finalized, 1, id);
        assert(std::equal(source.begin() + static_cast<std::ptrdiff_t>(before),
                          source.begin() + static_cast<std::ptrdiff_t>(before + Detail::kSectorSize),
                          finalized.begin() + static_cast<std::ptrdiff_t>(after)));
    }

    const bool keyed = family != Family::RS;
    const uint16_t key16 = keyed ? 0xC3D4 : 0;
    assert(rawStoredQuantity(finalized, 1, family, InventoryPouch::PokeBalls, 2) ==
           static_cast<uint16_t>(10 ^ key16));
    assert(rawStoredQuantity(finalized, 1, family, InventoryPouch::PCItems, 14) == 9);

    editor->discard();
    assert(!editor->hasPendingChanges());
    assert(editor->stagedBytes() == sourceCopy);
}

void testCapacity() {
    auto source = fixture(Family::RS);
    std::string error;
    auto editor = StagedInventoryEditor::create(source, SourceGame::RubyGBA, error);
    assert(editor);
    const auto catalog = Inventory::addableItems(Inventory::ClassicGame::Ruby, Inventory::ClassicPocket::Balls);
    for (uint16_t item : catalog) assert(editor->stageInventoryQuantity(InventoryPouch::PokeBalls, item, 1, error));
    // RS has 16 ball slots but only 11 legal balls; exercise capacity with Items instead.
    const auto items = Inventory::addableItems(Inventory::ClassicGame::Ruby, Inventory::ClassicPocket::Items);
    for (std::size_t i = 0; i < 20; ++i)
        assert(editor->stageInventoryQuantity(InventoryPouch::Items, items[i], 1, error));
    assert(!editor->stageInventoryQuantity(InventoryPouch::Items, items[20], 1, error));
    assert(error.find("No free slots") != std::string::npos);
}

} // namespace

int main() {
    runExactGame(SourceGame::RubyGBA, Family::RS);
    runExactGame(SourceGame::SapphireGBA, Family::RS);
    runExactGame(SourceGame::EmeraldGBA, Family::Emerald);
    runExactGame(SourceGame::FireRedGBA, Family::FRLG);
    runExactGame(SourceGame::LeafGreenGBA, Family::FRLG);
    testCapacity();
    std::cout << "Gen III staged inventory editor: PASS\n";
}
