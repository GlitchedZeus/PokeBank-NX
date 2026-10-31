#pragma once
// Top Up / Add Missing (owner Oct 28, Feature B): the Gen I, II and III staged Bag editors as the
// planner sees them (BulkItems::PocketState) and as the all-or-nothing applier drives them
// (BulkItems::apply adapter). Pure: no UI, no file I/O; every write goes through the generation's
// own staged editor, so Review / Keep / Discard / Export Copy treat the result like any Bag edit.
//
// Candidate new items are the SAME per-game lists the Add picker offers
// (Inventory::addableItems, ClassicInventoryUIModel::classicInventoryAddableItems), and the native
// slot counts come from the editors themselves:
//   * Gen I  (pret pokered wNumBagItems / wNumBoxItems): Items, Medicine, Balls, Key Items and TMs
//     are views of ONE 20-slot Bag list (slotGroup 0); PC Items is its own 50-slot list. The writer
//     sets an item by id, so an item held in several stacks is reported, not topped up.
//   * Gen II (pret pokecrystal items.asm): Items / Balls / PC Items are pair lists in which one item
//     may sit in two slots; each stack is topped up on its own (stageInventorySlotQuantity). TM/HM
//     has one fixed slot per machine; Key Items its 25-entry list.
//   * Gen III (pret pokeemerald / pokefirered item.c): every pouch is a slot array; a new item goes
//     into an ITEM_NONE slot (freeSlots). The writer sets an item by id and clears further stacks,
//     so an item held in several stacks is reported, not topped up.

#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Inventory/BulkItemApply.h"
#include "Inventory/BulkItemPlan.h"
#include "Inventory/ClassicInventoryCatalog.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace PokeVault::Inventory::BulkItems {

// The classic pocket behind a policy pocket for this game (the inverse of quantityPolicyPocket).
[[nodiscard]] inline std::optional<ClassicPocket> classicPocketFor(ClassicGame game, Pocket pocket) noexcept {
    for (const auto p : supportedPockets(game))
        if (quantityPolicyPocket(p) == pocket) return p;
    return std::nullopt;
}

// ------------------------------------------------------------------------------------- Gen I --
namespace Gen1Bag {
using Editor = Integration::Gen1::StagedInventoryEditor;

[[nodiscard]] inline Game game(const Editor& editor) noexcept { return quantityPolicyGame(editor.game()); }

[[nodiscard]] inline std::vector<PocketState> pockets(const Editor& editor, std::string& error) {
    std::vector<PocketState> out;
    std::size_t bagHeld = 0;
    std::size_t bagFree = 0;
    for (const auto classic : supportedPockets(editor.game())) {
        PocketState state;
        state.pocket = quantityPolicyPocket(classic);
        state.perSlotTopUp = false;
        std::size_t slot = 0;
        for (const auto& entry : editor.entries(classic, error)) state.held.push_back({slot++, entry.itemId, entry.quantity});
        if (!error.empty()) return {};
        const auto picks = addableItems(editor.game(), classic);
        state.addableItems.assign(picks.begin(), picks.end());
        const std::size_t free = editor.freeSlots(classic, error);
        if (!error.empty()) return {};
        if (classic == ClassicPocket::PCItems) {
            state.slotCapacity = state.held.size() + free;
        } else {
            state.slotGroup = 0;
            bagHeld += state.held.size();
            bagFree = free;
        }
        out.push_back(std::move(state));
    }
    // One 20-slot list: the planner subtracts every view's held stacks from this once.
    for (auto& state : out)
        if (state.slotGroup == 0) state.slotCapacity = bagHeld + bagFree;
    return out;
}

struct Adapter {
    ClassicGame classic;
    explicit Adapter(const Editor& editor) : classic(editor.game()) {}
    [[nodiscard]] std::optional<ClassicPocket> map(Pocket p) const noexcept { return classicPocketFor(classic, p); }
    bool setQuantity(Editor& e, const TopUp& t, std::string& error) const {
        const auto p = map(t.pocket);
        if (!p) { error = "Generation I pocket is not supported"; return false; }
        return e.stageSetQuantity(*p, t.itemId, t.to, error);
    }
    bool add(Editor& e, const Add& a, std::string& error) const {
        const auto p = map(a.pocket);
        if (!p) { error = "Generation I pocket is not supported"; return false; }
        if (quantity(e, a.pocket, a.itemId, SIZE_MAX) != 0) { error = "Generation I item is already held"; return false; }
        return e.stageSetQuantity(*p, a.itemId, a.quantity, error);
    }
    [[nodiscard]] uint16_t quantity(const Editor& e, Pocket pocket, uint16_t itemId, std::size_t) const {
        const auto p = map(pocket);
        if (!p) return 0;
        std::string error;
        for (const auto& entry : e.entries(*p, error)) if (entry.itemId == itemId) return entry.quantity;
        return 0;
    }
    [[nodiscard]] bool singleStack(const Editor& e, Pocket pocket, uint16_t itemId) const {
        const auto p = map(pocket);
        if (!p) return false;
        std::string error;
        int n = 0;
        for (const auto& entry : e.entries(*p, error)) n += entry.itemId == itemId;
        return n <= 1;
    }
};
} // namespace Gen1Bag

// ------------------------------------------------------------------------------------ Gen II --
namespace Gen2Bag {
using Editor = Integration::Gen2::StagedEditor;
using Native = Integration::Gen2::InventoryPocket;

[[nodiscard]] inline ClassicGame classicGame(const Editor& editor) noexcept {
    switch (editor.metadata().sourceGame) {
        case Integration::Gen2::SourceGame::Gold: return ClassicGame::Gold;
        case Integration::Gen2::SourceGame::Silver: return ClassicGame::Silver;
        case Integration::Gen2::SourceGame::Crystal: return ClassicGame::Crystal;
    }
    return ClassicGame::Gold;
}
[[nodiscard]] inline Game game(const Editor& editor) noexcept { return quantityPolicyGame(classicGame(editor)); }

[[nodiscard]] inline std::optional<Native> nativePocket(Pocket pocket) noexcept {
    switch (pocket) {
        case Pocket::Items: return Native::Items;
        case Pocket::Balls: return Native::Balls;
        case Pocket::KeyItems: return Native::KeyItems;
        case Pocket::TMHM: return Native::TMHM;
        case Pocket::PCItems: return Native::PCItems;
        default: return std::nullopt;
    }
}
[[nodiscard]] inline bool pairList(Native pocket) noexcept {
    return pocket == Native::Items || pocket == Native::Balls || pocket == Native::PCItems;
}

[[nodiscard]] inline std::vector<PocketState> pockets(const Editor& editor, std::string& error) {
    std::vector<PocketState> out;
    const auto classic = classicGame(editor);
    for (const auto policy : {Pocket::Items, Pocket::Balls, Pocket::KeyItems, Pocket::TMHM, Pocket::PCItems}) {
        const auto native = *nativePocket(policy);
        const auto classicPocket = classicPocketFor(classic, policy);
        if (!classicPocket) continue;
        PocketState state;
        state.pocket = policy;
        std::size_t slot = 0;
        for (const auto& entry : editor.inventoryEntries(native, error)) state.held.push_back({slot++, entry.itemId, entry.quantity});
        if (!error.empty()) return {};
        state.slotCapacity = editor.inventoryCapacity(native);
        const auto picks = addableItems(classic, *classicPocket);
        state.addableItems.assign(picks.begin(), picks.end());
        out.push_back(std::move(state));
    }
    return out;
}

struct Adapter {
    bool setQuantity(Editor& e, const TopUp& t, std::string& error) const {
        const auto p = nativePocket(t.pocket);
        if (!p || t.itemId > 0xFF || t.to > 0xFF) { error = "Generation II item or quantity is outside the stored range"; return false; }
        if (pairList(*p))
            return e.stageInventorySlotQuantity(*p, t.slot, static_cast<uint8_t>(t.itemId), static_cast<uint8_t>(t.to), error);
        return e.stageInventoryQuantity(*p, static_cast<uint8_t>(t.itemId), static_cast<uint8_t>(t.to), error);
    }
    bool add(Editor& e, const Add& a, std::string& error) const {
        const auto p = nativePocket(a.pocket);
        if (!p || a.itemId > 0xFF || a.quantity > 0xFF) { error = "Generation II item or quantity is outside the stored range"; return false; }
        if (e.stagedInventoryQuantity(*p, static_cast<uint8_t>(a.itemId)) != 0) { error = "Generation II item is already held"; return false; }
        return e.stageInventoryQuantity(*p, static_cast<uint8_t>(a.itemId), static_cast<uint8_t>(a.quantity), error);
    }
    // `slot` is the native list position for a top-up (each Gen II stack reads back on its own).
    [[nodiscard]] uint16_t quantity(const Editor& e, Pocket pocket, uint16_t itemId, std::size_t slot) const {
        const auto p = nativePocket(pocket);
        if (!p) return 0;
        std::string error;
        const auto entries = e.inventoryEntries(*p, error);
        if (!error.empty()) return 0;
        if (slot != SIZE_MAX && pairList(*p))
            return slot < entries.size() && entries[slot].itemId == itemId ? entries[slot].quantity : 0;
        for (const auto& entry : entries) if (entry.itemId == itemId) return entry.quantity;
        return 0;
    }
    // Every Gen II stack is addressed by its own slot.
    [[nodiscard]] bool singleStack(const Editor&, Pocket, uint16_t) const { return true; }
};
} // namespace Gen2Bag

// ----------------------------------------------------------------------------------- Gen III --
namespace Gen3Bag {
using Editor = Integration::Gen3::StagedInventoryEditor;
using Pouch = Integration::Gen3::InventoryPouch;

[[nodiscard]] inline ClassicGame classicGame(const Editor& editor) noexcept {
    using S = Integration::Gen3::SourceGame;
    switch (editor.sourceGame()) {
        case S::RubyGBA: return ClassicGame::Ruby;
        case S::SapphireGBA: return ClassicGame::Sapphire;
        case S::EmeraldGBA: return ClassicGame::Emerald;
        case S::FireRedGBA: return ClassicGame::FireRed;
        case S::LeafGreenGBA: return ClassicGame::LeafGreen;
    }
    return ClassicGame::Ruby;
}
[[nodiscard]] inline Game game(const Editor& editor) noexcept { return quantityPolicyGame(classicGame(editor)); }

[[nodiscard]] inline std::optional<Pouch> pouchFor(Pocket pocket) noexcept {
    switch (pocket) {
        case Pocket::Items: return Pouch::Items;
        case Pocket::KeyItems: return Pouch::KeyItems;
        case Pocket::Balls: return Pouch::PokeBalls;
        case Pocket::TMHM: return Pouch::TMCase;
        case Pocket::Berries: return Pouch::BerryPouch;
        case Pocket::PCItems: return Pouch::PCItems;
        default: return std::nullopt;
    }
}

[[nodiscard]] inline std::vector<PocketState> pockets(const Editor& editor, std::string& error) {
    std::vector<PocketState> out;
    const auto classic = classicGame(editor);
    for (const auto policy : {Pocket::Items, Pocket::KeyItems, Pocket::Balls, Pocket::TMHM, Pocket::Berries, Pocket::PCItems}) {
        const auto pouch = *pouchFor(policy);
        const auto classicPocket = classicPocketFor(classic, policy);
        if (!classicPocket) continue;
        PocketState state;
        state.pocket = policy;
        state.perSlotTopUp = false;
        std::size_t slot = 0;
        for (const auto& entry : editor.inventoryEntries(pouch, error)) state.held.push_back({slot++, entry.itemId, entry.count});
        if (!error.empty()) return {};
        state.slotCapacity = state.held.size() + editor.freeSlots(pouch);
        const auto picks = addableItems(classic, *classicPocket);
        state.addableItems.assign(picks.begin(), picks.end());
        out.push_back(std::move(state));
    }
    return out;
}

struct Adapter {
    bool setQuantity(Editor& e, const TopUp& t, std::string& error) const {
        const auto p = pouchFor(t.pocket);
        if (!p) { error = "Generation III pocket is not supported"; return false; }
        return e.stageInventoryQuantity(*p, t.itemId, t.to, error);
    }
    bool add(Editor& e, const Add& a, std::string& error) const {
        const auto p = pouchFor(a.pocket);
        if (!p) { error = "Generation III pocket is not supported"; return false; }
        if (e.stagedInventoryQuantity(*p, a.itemId) != 0) { error = "Generation III item is already held"; return false; }
        return e.stageInventoryQuantity(*p, a.itemId, a.quantity, error);
    }
    [[nodiscard]] uint16_t quantity(const Editor& e, Pocket pocket, uint16_t itemId, std::size_t) const {
        const auto p = pouchFor(pocket);
        return p ? e.stagedInventoryQuantity(*p, itemId) : 0;
    }
    [[nodiscard]] bool singleStack(const Editor& e, Pocket pocket, uint16_t itemId) const {
        const auto p = pouchFor(pocket);
        if (!p) return false;
        std::string error;
        int n = 0;
        for (const auto& entry : e.inventoryEntries(*p, error)) n += entry.itemId == itemId;
        return n <= 1;
    }
};
} // namespace Gen3Bag

} // namespace PokeVault::Inventory::BulkItems
