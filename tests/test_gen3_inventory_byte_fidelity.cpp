// Q07 (independent QA 2026-10-27): a Gen III Bag edit changes only the bytes the game itself would
// change (pret pokefirered ce6406d8 / pokeemerald 731ad5bf src/item.c), plus that sector's
// checksum. A same-value edit, or a revert, leaves the save byte-identical.
//
// Every vacant slot in these fixtures is all-zero, which on a keyed (Emerald/FRLG) bag decodes as
// "item 0" but is not the canonical {0, 0 ^ key} the old writer forced into every vacant slot. One
// vacant slot also holds distinctive 0x55AA bytes, as an unrelated record would.
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "Inventory/ItemQuantityPolicy.h"
#include "UI/SharedBagAddFlow.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>
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
    return static_cast<uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
}

constexpr uint32_t kKey = 0xA1B2C3D4;
constexpr uint16_t kPotion = 13, kAntidote = 14, kSuperPotion = 22;

std::size_t itemsOffset(Family family) { return family == Family::FRLG ? 0x0310 : 0x0560; }
uint16_t key16(Family family) { return family == Family::RS ? 0 : static_cast<uint16_t>(kKey); }

LogicalSectors makeLogical(Family family) {
    LogicalSectors logical{};
    if (family == Family::FRLG) {
        write32(logical[0], 0xAC, 1);
        write32(logical[0], 0xF20, kKey);
    } else if (family == Family::Emerald) {
        write32(logical[0], 0xAC, kKey);
        logical[0][0xEE0] = 0x42;
    }
    const std::size_t items = itemsOffset(family);
    // Slot 0: Potion x3. Slot 2: Super Potion x4 (slot 1 is a hole the game would fill first).
    // Slot 5: vacant (item 0) with distinctive quantity bytes. Every other slot all-zero.
    write16(logical[1], items + 0, kPotion);
    write16(logical[1], items + 2, static_cast<uint16_t>(3 ^ key16(family)));
    write16(logical[1], items + 8, kSuperPotion);
    write16(logical[1], items + 10, static_cast<uint16_t>(4 ^ key16(family)));
    write16(logical[1], items + 5 * 4 + 2, 0x55AA);
    if (family == Family::FRLG) {
        // Key Items (0x03B8): Meteorite x1 in slot 0, as a save that already holds one.
        write16(logical[1], 0x03B8, 280);
        write16(logical[1], 0x03BA, static_cast<uint16_t>(1 ^ key16(family)));
    }
    return logical;
}

void writeSaveSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation,
                   Family family) {
    const auto logical = makeLogical(family);
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
    writeSaveSlot(save, 0, 7, 0, family);
    writeSaveSlot(save, 1, 9, 5, family);  // slot 1 is active (newer counter)
    return save;
}

// Absolute offset of logical sector 1 in the active slot.
std::size_t activeSector1(std::span<const uint8_t> bytes) {
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const std::size_t offset = Detail::kSlotBases[1] + physical * Detail::kSectorSize;
        if (read16(bytes, offset + 0xFF4) == 1) return offset;
    }
    assert(false);
    return 0;
}

std::vector<std::size_t> diff(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    assert(a.size() == b.size());
    std::vector<std::size_t> out;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) out.push_back(i);
    return out;
}

// Every changed byte must be inside one of `slots` of the Items pouch or the sector-1 checksum.
void assertOnlySlotsChanged(std::span<const uint8_t> before, std::span<const uint8_t> after, Family family,
                            std::set<uint16_t> slots) {
    const std::size_t sector = activeSector1(before);
    const std::size_t pouch = sector + itemsOffset(family);
    for (const std::size_t at : diff(before, after)) {
        const bool checksum = at == sector + 0xFF6 || at == sector + 0xFF7;
        const bool inSlot = at >= pouch && slots.count(static_cast<uint16_t>((at - pouch) / 4)) != 0 &&
                            (at - pouch) / 4 < 64;
        if (!checksum && !inSlot) {
            std::cerr << "unexpected byte change at 0x" << std::hex << at << std::dec << "\n";
            assert(false);
        }
    }
}

void runFamily(SourceGame game, Family family) {
    const auto source = fixture(family);
    std::string error;
    auto editor = StagedInventoryEditor::create(source, game, error);
    assert(editor && error.empty());
    const auto stagedAt = [&] { return std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end()); };

    // 1. Same quantity: a true no-op, byte for byte, with nothing pending.
    assert(editor->stageInventoryQuantity(InventoryPouch::Items, kPotion, 3, error));
    assert(stagedAt() == source && !editor->hasPendingChanges());
    // Removing an absent item is also nothing.
    assert(editor->stageInventoryQuantity(InventoryPouch::Items, kAntidote, 0, error));
    assert(stagedAt() == source && !editor->hasPendingChanges());

    // 2. Quantity change: that slot's quantity plus the sector checksum only.
    assert(editor->stageInventoryQuantity(InventoryPouch::Items, kPotion, 4, error));
    const auto changed = stagedAt();
    assertOnlySlotsChanged(source, changed, family, {0});
    assert(!diff(source, changed).empty());
    // The inactive save slot is untouched.
    assert(std::equal(source.begin(), source.begin() + static_cast<std::ptrdiff_t>(Detail::kSlotBases[1]),
                      changed.begin()));
    // 3. Repeating the identical edit adds nothing.
    assert(editor->stageInventoryQuantity(InventoryPouch::Items, kPotion, 4, error));
    assert(stagedAt() == changed);

    // 4. Revert (3 -> 4 -> 3): byte-identical again and no false pending change.
    assert(editor->stageInventoryQuantity(InventoryPouch::Items, kPotion, 3, error));
    assert(stagedAt() == source);
    assert(!editor->hasPendingChanges());

    // 5. Add: the first ITEM_NONE slot (the hole at slot 1), not the end of the list.
    assert(editor->stageInventoryQuantity(InventoryPouch::Items, kAntidote, 2, error));
    const auto added = stagedAt();
    assertOnlySlotsChanged(source, added, family, {1});
    const std::size_t pouch = activeSector1(added) + itemsOffset(family);
    assert(read16(added, pouch + 4) == kAntidote);
    assert(static_cast<uint16_t>(read16(added, pouch + 6) ^ key16(family)) == 2);
    assert(read16(added, pouch + 5 * 4 + 2) == 0x55AA);  // distinctive vacant bytes kept

    // 6. Remove: in place (quantity 0, ITEM_NONE), no compaction; Super Potion stays in slot 2.
    assert(editor->stageInventoryQuantity(InventoryPouch::Items, kPotion, 0, error));
    const auto removed = stagedAt();
    assertOnlySlotsChanged(source, removed, family, {0, 1});
    assert(read16(removed, pouch + 0) == 0);
    assert(static_cast<uint16_t>(read16(removed, pouch + 2) ^ key16(family)) == 0);
    assert(read16(removed, pouch + 8) == kSuperPotion);
    assert(editor->stagedInventoryQuantity(InventoryPouch::Items, kPotion) == 0);
    assert(editor->stagedInventoryQuantity(InventoryPouch::Items, kSuperPotion) == 4);

    // 7. Serialize -> strict reparse -> same-value edit on the reparsed copy is a no-op.
    const auto finalized = editor->finalizedBytes(error);
    assert(!finalized.empty() && error.empty());
    auto reopened = StagedInventoryEditor::create(finalized, game, error);
    assert(reopened && error.empty());
    assert(reopened->stagedInventoryQuantity(InventoryPouch::Items, kAntidote) == 2);
    assert(reopened->stageInventoryQuantity(InventoryPouch::Items, kAntidote, 2, error));
    assert(std::equal(reopened->stagedBytes().begin(), reopened->stagedBytes().end(), finalized.begin()));

    // 8. Discard returns the exact source; the caller's source was never written.
    editor->discard();
    assert(stagedAt() == source && !editor->hasPendingChanges());
}

// Q06 (independent QA 2026-10-27): Meteorite (280) is an FRLG Key Item. pret pokefirered ce6406d8
// src/data/items.json: POCKET_KEY_ITEMS, importance 1; PKHeX 8e3eb3d4 ItemStorage3FRLG.Key;
// upstream PKSE d2ad323.
void meteorite(SourceGame game) {
    namespace C = PokeVault::Inventory;
    namespace QP = PokeVault::Inventory::QuantityPolicy;
    const auto classic = game == SourceGame::FireRedGBA ? C::ClassicGame::FireRed : C::ClassicGame::LeafGreen;
    const auto policy = game == SourceGame::FireRedGBA ? QP::Game::FireRed : QP::Game::LeafGreen;
    // Catalog and policy: a unique Key Item, only in Key Items, with the story-item warning.
    assert(C::isAddableItem(classic, C::ClassicPocket::KeyItems, 280));
    assert(!C::isAddableItem(classic, C::ClassicPocket::Items, 280));
    const auto rule = QP::quantityRule(policy, QP::Pocket::KeyItems, 280);
    assert(rule.valid && rule.maximum == 1 && rule.kind == QP::ItemKind::KeyItem);
    assert(PokeBank::UIModel::BagAdd::needsKeyItemWarning(rule));
    assert(!QP::quantityRule(policy, QP::Pocket::Items, 280).valid);
    // Ruby/Sapphire/Emerald keep their own Meteorite entry; nothing else moved.
    assert(C::isAddableItem(C::ClassicGame::Emerald, C::ClassicPocket::KeyItems, 280));

    // An existing Meteorite is recognised and can be kept, refused above 1, and removed in place.
    const auto source = fixture(Family::FRLG);
    std::string error;
    auto editor = StagedInventoryEditor::create(source, game, error);
    assert(editor && error.empty());
    assert(editor->stagedInventoryQuantity(InventoryPouch::KeyItems, 280) == 1);
    assert(editor->stageInventoryQuantity(InventoryPouch::KeyItems, 280, 1, error));
    assert(std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end()) == source);
    assert(!editor->stageInventoryQuantity(InventoryPouch::KeyItems, 280, 2, error));
    assert(!editor->stageInventoryQuantity(InventoryPouch::Items, 280, 1, error));
    assert(std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end()) == source);
    assert(editor->stageInventoryQuantity(InventoryPouch::KeyItems, 280, 0, error));
    const std::vector<uint8_t> removed(editor->stagedBytes().begin(), editor->stagedBytes().end());
    const std::size_t sector = activeSector1(removed);
    for (const std::size_t at : diff(source, removed))
        assert((at >= sector + 0x03B8 && at < sector + 0x03BC) || at == sector + 0xFF6 || at == sector + 0xFF7);
    // Verified round trip: strict reparse, and the item can be put back.
    const auto finalized = editor->finalizedBytes(error);
    assert(!finalized.empty());
    auto reopened = StagedInventoryEditor::create(finalized, game, error);
    assert(reopened && reopened->stagedInventoryQuantity(InventoryPouch::KeyItems, 280) == 0);
    assert(reopened->stageInventoryQuantity(InventoryPouch::KeyItems, 280, 1, error));
    assert(reopened->stagedInventoryQuantity(InventoryPouch::KeyItems, 280) == 1);
}

}  // namespace

int main() {
    meteorite(SourceGame::FireRedGBA);
    meteorite(SourceGame::LeafGreenGBA);
    runFamily(SourceGame::RubyGBA, Family::RS);
    runFamily(SourceGame::EmeraldGBA, Family::Emerald);
    runFamily(SourceGame::FireRedGBA, Family::FRLG);
    runFamily(SourceGame::LeafGreenGBA, Family::FRLG);
    std::cout << "Gen III inventory byte fidelity (no-op, in-place quantity, first-free add, in-place remove): PASS\n";
}
