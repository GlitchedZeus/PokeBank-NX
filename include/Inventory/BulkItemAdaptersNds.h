#pragma once
// Top Up / Add Missing (owner Oct 28, Feature B): the Gen IV and Gen V staged Bags as the planner
// sees them (BulkItems::PocketState) and as the all-or-nothing applier drives them. Pure: no UI, no
// file I/O; every write goes through the generation's own staged writer.
//
//   * Gen IV: Gen4StagedPokemonEditor (stageBagQuantity per visible stack, stageBagAdd appends after
//     the last occupied slot into a blank 0/0 or FFFF/FFFF sentinel, PKSM-Core aa22d7a4 pouch
//     offsets, Gen4ReadOnlyInventory.h). Free slots are exactly the blank sentinels after the last
//     occupied slot, the ones stageBagAdd may use. Candidates: gen4BagChoices (the Add picker's list).
//   * Gen V: Gen5StagedBag (setQuantity per visible stack, add into a 0/0 slot; PKHeX
//     PlayerBag5BW / B2W2 via Gen5Bag.h). Free slots are the 0/0 slots. Candidates: gen5BagChoices.
// Both writers address one stack by its visible row, so each stack is topped up on its own.

#include "Integration/Gen4/Gen4BagCatalog.h"
#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen5/Gen5Bag.h"
#include "Inventory/BulkItemApply.h"
#include "Inventory/BulkItemPlan.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace PokeVault::Inventory::BulkItems {

// ------------------------------------------------------------------------------------ Gen IV --
namespace Gen4Bag {
using Editor = Integration::Gen4::Gen4StagedPokemonEditor;
namespace G4 = Integration::Gen4;

// Gen4ReadOnlyInventory.h BagPocket order, as UI/Gen4BagQuantity.h maps it to the policy.
inline constexpr std::array<Pocket, G4::BagPocketCount> kPockets{{
    Pocket::Items, Pocket::KeyItems, Pocket::TMHM, Pocket::Mail,
    Pocket::Medicine, Pocket::Berries, Pocket::Balls, Pocket::BattleItems}};

[[nodiscard]] inline std::optional<std::size_t> nativePocket(Pocket pocket) noexcept {
    for (std::size_t i = 0; i < kPockets.size(); ++i) if (kPockets[i] == pocket) return i;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<Game> game(const Editor& editor) noexcept {
    Game out{};
    if (!QuantityPolicy::gameFromId(editor.exactGameId(), out) || QuantityPolicy::generation(out) != 4) return std::nullopt;
    return out;
}

// The staged bag exactly as the writer reparses it.
[[nodiscard]] inline std::optional<G4::ValidatedBag> stagedBag(const Editor& editor, std::size_t* generalOffset = nullptr) {
    const auto parsed = G4::Gen4ReadOnlySave::parse(editor.stagedBytes(), editor.layout(), editor.exactGameId());
    if (!parsed) return std::nullopt;
    if (generalOffset) *generalOffset = parsed->generalSelection().offset;
    return G4::decodeReadOnlyBag(*parsed);
}

[[nodiscard]] inline std::vector<PocketState> pockets(const Editor& editor, std::string& error) {
    std::size_t base = 0;
    const auto bag = stagedBag(editor, &base);
    if (!bag) { error = "Gen IV staged bag failed native validation"; return {}; }
    const auto& bytes = editor.stagedBytes();
    const auto layout = G4::bagLayout(editor.layout());
    const auto read16 = [&](std::size_t at) { return static_cast<uint16_t>(bytes[at] | (bytes[at + 1] << 8)); };
    std::vector<PocketState> out;
    for (std::size_t pocket = 0; pocket < G4::BagPocketCount; ++pocket) {
        PocketState state;
        state.pocket = kPockets[pocket];
        std::size_t row = 0;
        for (const auto& stack : (*bag)[pocket]) state.held.push_back({row++, stack.itemId, stack.count});
        // Blank sentinels after the last occupied slot: where stageBagAdd may write.
        const auto spec = layout[pocket];
        std::size_t afterLast = 0;
        for (std::size_t slot = 0; slot < spec.slots; ++slot) {
            const std::size_t at = base + spec.offset + slot * 4;
            if (at + 4 > bytes.size()) { error = "Gen IV bag pocket exceeds the staged save"; return {}; }
            const uint16_t id = read16(at), count = read16(at + 2);
            if (id != 0 && id != 0xFFFF && count != 0) afterLast = slot + 1;
        }
        std::size_t free = 0;
        for (std::size_t slot = afterLast; slot < spec.slots; ++slot) {
            const std::size_t at = base + spec.offset + slot * 4;
            const uint16_t id = read16(at), count = read16(at + 2);
            if ((id == 0 && count == 0) || (id == 0xFFFF && count == 0xFFFF)) ++free;
        }
        state.slotCapacity = state.held.size() + free;
        state.addableItems = G4::gen4BagChoices(editor.layout(), pocket);
        out.push_back(std::move(state));
    }
    error.clear();
    return out;
}

struct Adapter {
    bool setQuantity(Editor& e, const TopUp& t, std::string& error) const {
        const auto p = nativePocket(t.pocket);
        if (!p) { error = "Gen IV pocket is not supported"; return false; }
        return e.stageBagQuantity(*p, t.slot, t.to, &error);
    }
    bool add(Editor& e, const Add& a, std::string& error) const {
        const auto p = nativePocket(a.pocket);
        if (!p) { error = "Gen IV pocket is not supported"; return false; }
        return e.stageBagAdd(*p, a.itemId, a.quantity, &error);
    }
    [[nodiscard]] uint16_t quantity(const Editor& e, Pocket pocket, uint16_t itemId, std::size_t slot) const {
        const auto p = nativePocket(pocket);
        const auto bag = p ? stagedBag(e) : std::nullopt;
        if (!bag) return 0;
        const auto& rows = (*bag)[*p];
        if (slot != SIZE_MAX) return slot < rows.size() && rows[slot].itemId == itemId ? rows[slot].count : 0;
        for (const auto& stack : rows) if (stack.itemId == itemId) return stack.count;
        return 0;
    }
    [[nodiscard]] bool singleStack(const Editor&, Pocket, uint16_t) const { return true; }
};
} // namespace Gen4Bag

// ------------------------------------------------------------------------------------- Gen V --
namespace Gen5Bag {
using Editor = Integration::Gen5::Gen5StagedBag;
namespace G5 = Integration::Gen5;

// Gen5Bag.h BagPocket5 order (Poke Balls live in Items), as the Gen V surface maps it.
inline constexpr std::array<Pocket, G5::BagPocket5Count> kPockets{{
    Pocket::Items, Pocket::KeyItems, Pocket::TMHM, Pocket::Medicine, Pocket::Berries}};

[[nodiscard]] inline std::optional<std::size_t> nativePocket(Pocket pocket) noexcept {
    for (std::size_t i = 0; i < kPockets.size(); ++i) if (kPockets[i] == pocket) return i;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<Game> game(std::string_view gameId) noexcept {
    Game out{};
    if (!QuantityPolicy::gameFromId(gameId, out) || QuantityPolicy::generation(out) != 5) return std::nullopt;
    return out;
}

[[nodiscard]] inline std::vector<PocketState> pockets(const Editor& bag) {
    const auto current = bag.current();
    const auto block = bag.staged();
    std::vector<PocketState> out;
    for (std::size_t pocket = 0; pocket < G5::BagPocket5Count; ++pocket) {
        PocketState state;
        state.pocket = kPockets[pocket];
        std::size_t row = 0;
        for (const auto& stack : current[pocket]) state.held.push_back({row++, stack.itemId, stack.count});
        const auto spec = G5::bagPocketLayout5(bag.family(), pocket);
        std::size_t free = 0;
        for (std::size_t slot = 0; slot < spec.slots; ++slot) {
            const std::size_t at = spec.offset + slot * 4;
            if (at + 4 > block.size()) break;
            if ((block[at] | block[at + 1] | block[at + 2] | block[at + 3]) == 0) ++free;
        }
        state.slotCapacity = state.held.size() + free;
        state.addableItems = G5::gen5BagChoices(bag.family(), pocket);
        out.push_back(std::move(state));
    }
    return out;
}

struct Adapter {
    bool setQuantity(Editor& e, const TopUp& t, std::string& error) const {
        const auto p = nativePocket(t.pocket);
        if (!p) { error = "Gen V pocket is not supported"; return false; }
        return e.setQuantity(*p, t.slot, t.to, &error);
    }
    bool add(Editor& e, const Add& a, std::string& error) const {
        const auto p = nativePocket(a.pocket);
        if (!p) { error = "Gen V pocket is not supported"; return false; }
        return e.add(*p, a.itemId, a.quantity, &error);
    }
    [[nodiscard]] uint16_t quantity(const Editor& e, Pocket pocket, uint16_t itemId, std::size_t slot) const {
        const auto p = nativePocket(pocket);
        if (!p) return 0;
        const auto rows = e.current()[*p];
        if (slot != SIZE_MAX) return slot < rows.size() && rows[slot].itemId == itemId ? rows[slot].count : 0;
        for (const auto& stack : rows) if (stack.itemId == itemId) return stack.count;
        return 0;
    }
    [[nodiscard]] bool singleStack(const Editor&, Pocket, uint16_t) const { return true; }
};
} // namespace Gen5Bag

} // namespace PokeVault::Inventory::BulkItems
