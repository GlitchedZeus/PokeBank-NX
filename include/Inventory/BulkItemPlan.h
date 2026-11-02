#pragma once
// Top Up / Add Missing items (owner Oct 28, Feature B; adapted from PKSE d2ad323 "Add All Items").
//
// A pure planner: it reads the Bag as the staged editor holds it and returns an exact preview of what a
// bulk operation would change. Nothing is written here; each generation's applier stages the planned
// operations through its existing Bag editor (so Review / Keep / Discard work unchanged), and the user
// confirms the preview first.
//
// Rules, and where they differ from upstream d2ad323 on purpose:
//   * Key Items, HMs and items the game allows only once are skipped by default. They can gate story
//     events (gap matrix row d2ad323 / B-10). There is no key-item override.
//   * Mutually exclusive items (bikes, fossils: QuantityPolicy::exclusiveWith) are never added.
//   * Per-item caps come from QuantityPolicy::quantityRule (RSE 99, not upstream's 999; Gen I Exp. All 99).
//   * A count is never lowered; a stack already at or above the target is left as it is.
//   * Each native stack is topped up on its own (Gen II can legally hold one item in two slots;
//     upstream topped up only the first).
//   * New stacks go only into free slots of the pocket (slotCapacity), in ascending item id; the rest are
//     reported as "pocket full", never forced.
//   * Candidate new items come from the caller: the same per-game list its Add picker offers, so no
//     invalid, unreleased or event-only id can be proposed.
//   * Pockets that are views of ONE native list (Gen I: Items / Medicine / Balls / Key Items / TMs
//     share the 20-slot Bag) carry the same slotGroup; their free slots are counted once, for the
//     whole list, whichever pockets are planned.
//   * A generation whose writer sets an item's quantity by id (it merges or clears further stacks of
//     that item: Gen I, Gen III) sets perSlotTopUp = false; an item held in several stacks there is
//     reported and left alone, so a top-up can never lower the item's total.

#include "Inventory/ItemQuantityPolicy.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace PokeVault::Inventory::BulkItems {

using QuantityPolicy::Game;
using QuantityPolicy::ItemKind;
using QuantityPolicy::Pocket;

enum class Mode : uint8_t { TopUpExisting, AddMissing, Both };

struct HeldStack {
    std::size_t slot = 0;    // native slot index within the pocket
    uint16_t itemId = 0;
    uint16_t quantity = 0;
};

struct PocketState {
    Pocket pocket = Pocket::Items;
    std::vector<HeldStack> held;            // occupied native slots, in native order
    std::size_t slotCapacity = 0;           // native slots in this pocket
    std::vector<uint16_t> addableItems;     // what this game's Add picker offers for the pocket
    bool perSlotTopUp = true;               // the writer can set one native stack on its own
    int slotGroup = -1;                     // >= 0: pockets sharing one native list (and its slots)
};

struct Request {
    Mode mode = Mode::Both;
    uint16_t quantity = 1;                  // target stack size (clamped per item)
    std::vector<Pocket> pockets;            // empty = every pocket in `state`
};

enum class SkipReason : uint8_t {
    KeyOrStoryItem,      // Key Item or HM
    OnlyOnce,            // native maximum 1 (e.g. Gen V TMs): topping up cannot apply
    AlreadyAtTarget,     // existing stack >= target (never lowered)
    Exclusive,           // a mutually exclusive item is already held
    PocketFull,          // no free native slot left for a new stack
    NotInPocket,         // not a valid native member of this pocket
    SeveralStacks,       // held in several stacks where the writer sets an item by id
};

struct TopUp { Pocket pocket; std::size_t slot; uint16_t itemId; uint16_t from; uint16_t to; };
struct Add { Pocket pocket; uint16_t itemId; uint16_t quantity; };
struct Skip { Pocket pocket; uint16_t itemId; SkipReason reason; };

struct Plan {
    std::vector<TopUp> topUps;
    std::vector<Add> adds;
    std::vector<Skip> skipped;
    [[nodiscard]] bool empty() const noexcept { return topUps.empty() && adds.empty(); }
};

[[nodiscard]] inline std::string_view skipReasonText(SkipReason r) noexcept {
    switch (r) {
        case SkipReason::KeyOrStoryItem: return "Key Item or HM (excluded)";
        case SkipReason::OnlyOnce: return "the game allows only one";
        case SkipReason::AlreadyAtTarget: return "already at or above the amount";
        case SkipReason::Exclusive: return "you already have its alternative";
        case SkipReason::PocketFull: return "pocket full";
        case SkipReason::NotInPocket: return "not a native item of this pocket";
        case SkipReason::SeveralStacks: return "held in several stacks; edit it on its own";
    }
    return "skipped";
}

[[nodiscard]] inline bool storySensitive(const QuantityPolicy::ItemQuantityRule& rule) noexcept {
    return rule.kind == ItemKind::KeyItem || rule.kind == ItemKind::HM;
}

// "Every pocket" of a Bag never includes the PC: the PC is storage, not the Bag the player carries.
[[nodiscard]] inline std::vector<Pocket> bagPockets(std::span<const PocketState> state) {
    std::vector<Pocket> out;
    for (const auto& p : state)
        if (p.pocket != Pocket::PCItems) out.push_back(p.pocket);
    return out;
}

[[nodiscard]] inline Plan plan(Game game, const Request& request, std::span<const PocketState> state) {
    Plan out;
    if (request.quantity == 0) return out;
    const auto wanted = [&](Pocket p) {
        return request.pockets.empty() ||
               std::find(request.pockets.begin(), request.pockets.end(), p) != request.pockets.end();
    };
    const bool topUp = request.mode != Mode::AddMissing;
    const bool addMissing = request.mode != Mode::TopUpExisting;

    // Everything already held anywhere in the Bag counts for exclusivity.
    std::vector<uint16_t> heldAnywhere;
    for (const auto& p : state)
        for (const auto& h : p.held) heldAnywhere.push_back(h.itemId);
    // Free native slots of each shared list, counted once over every pocket that views it.
    std::vector<std::pair<int, std::size_t>> groupFree;
    for (const auto& p : state) {
        if (p.slotGroup < 0 || std::any_of(groupFree.begin(), groupFree.end(),
                                           [&p](const auto& g) { return g.first == p.slotGroup; }))
            continue;
        std::size_t used = 0;
        for (const auto& q : state) if (q.slotGroup == p.slotGroup) used += q.held.size();
        groupFree.push_back({p.slotGroup, p.slotCapacity > used ? p.slotCapacity - used : 0});
    }

    for (const auto& p : state) {
        if (!wanted(p.pocket)) continue;
        if (topUp) {
            for (const auto& h : p.held) {
                const auto stacks = std::count_if(p.held.begin(), p.held.end(),
                                                  [&h](const HeldStack& o) { return o.itemId == h.itemId; });
                if (!p.perSlotTopUp && stacks > 1) {
                    const bool reported = std::any_of(out.skipped.begin(), out.skipped.end(), [&](const Skip& s) {
                        return s.pocket == p.pocket && s.itemId == h.itemId && s.reason == SkipReason::SeveralStacks;
                    });
                    if (!reported) out.skipped.push_back({p.pocket, h.itemId, SkipReason::SeveralStacks});
                    continue;
                }
                const auto rule = QuantityPolicy::quantityRule(game, p.pocket, h.itemId);
                if (!rule.valid) { out.skipped.push_back({p.pocket, h.itemId, SkipReason::NotInPocket}); continue; }
                if (storySensitive(rule)) { out.skipped.push_back({p.pocket, h.itemId, SkipReason::KeyOrStoryItem}); continue; }
                if (rule.unique || rule.maximum <= 1) { out.skipped.push_back({p.pocket, h.itemId, SkipReason::OnlyOnce}); continue; }
                const uint16_t target = std::min<uint16_t>(request.quantity, rule.maximum);
                if (h.quantity >= target) { out.skipped.push_back({p.pocket, h.itemId, SkipReason::AlreadyAtTarget}); continue; }
                out.topUps.push_back({p.pocket, h.slot, h.itemId, h.quantity, target});
            }
        }
        if (addMissing) {
            std::size_t ownFree = p.slotCapacity > p.held.size() ? p.slotCapacity - p.held.size() : 0;
            std::size_t* free = &ownFree;
            for (auto& g : groupFree) if (g.first == p.slotGroup) free = &g.second;
            std::size_t& freeSlots = *free;
            std::vector<uint16_t> candidates(p.addableItems.begin(), p.addableItems.end());
            std::sort(candidates.begin(), candidates.end());
            candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
            for (const uint16_t id : candidates) {
                if (id == 0) continue;
                const bool held = std::any_of(p.held.begin(), p.held.end(), [id](const HeldStack& h) { return h.itemId == id; });
                if (held) continue;
                const auto rule = QuantityPolicy::quantityRule(game, p.pocket, id);
                if (!rule.valid || !QuantityPolicy::mayAddItem(game, p.pocket, id, 0)) {
                    out.skipped.push_back({p.pocket, id, SkipReason::NotInPocket});
                    continue;
                }
                if (storySensitive(rule)) { out.skipped.push_back({p.pocket, id, SkipReason::KeyOrStoryItem}); continue; }
                bool exclusive = false;
                for (const uint16_t other : QuantityPolicy::exclusiveWith(game, id))
                    if (other != id && std::find(heldAnywhere.begin(), heldAnywhere.end(), other) != heldAnywhere.end())
                        exclusive = true;
                if (exclusive) { out.skipped.push_back({p.pocket, id, SkipReason::Exclusive}); continue; }
                if (freeSlots == 0) { out.skipped.push_back({p.pocket, id, SkipReason::PocketFull}); continue; }
                const uint16_t quantity = rule.unique ? 1 : std::min<uint16_t>(request.quantity, rule.maximum);
                out.adds.push_back({p.pocket, id, quantity});
                heldAnywhere.push_back(id);
                --freeSlots;
            }
        }
    }
    return out;
}

}  // namespace PokeVault::Inventory::BulkItems
