#ifndef POKEBANK_INVENTORY_ITEM_QUANTITY_POLICY_H
#define POKEBANK_INVENTORY_ITEM_QUANTITY_POLICY_H

// Generation- and game-aware item quantity policy for every Gen I-V game.
//
// Pure query API: no UI, no save I/O, no mutation. It answers "which quantities
// may the UI offer for this exact item in this exact game and pocket?" so no
// surface ever offers 2 / 99 / 999 of something that can only exist once
// (bikes, rods, key items, HMs, Gen V TMs).
//
// Sources (cited per fact in ItemQuantityPolicy.cpp):
//  * Per-pocket native caps: PKHeX (pinned 6501f0ab) PKHeX.Core/Items/Bags/
//    PlayerBag1.cs, PlayerBag2.cs, PlayerBag3RS.cs, PlayerBag3E.cs,
//    PlayerBag3FRLG.cs, PlayerBag4DP.cs, PlayerBag4Pt.cs, PlayerBag4HGSS.cs,
//    PlayerBag5BW.cs, PlayerBag5B2W2.cs (MaxCount column + GetMaxCount HM rule).
//    These match the repo's own readers: Gen1ReadOnlyInventory.cpp (99),
//    Gen2ReadOnlyInventory.cpp (99, HM 1, ID-only key list),
//    Gen3StagedInventoryEditor.cpp (<= 999 on decode), Gen4BagCatalog.h
//    (gen4BagMaxQuantity), Gen5Bag.h (bagPocketLayout5 maxCount).
//  * Pocket membership / key-item lists: the repo's own catalogs
//    (Inventory/ClassicInventoryCatalog for Gen I-III, Gen4BagCatalog.h,
//    Gen5Bag.h BagCatalog5), which are themselves PKHeX ItemStorage* tables.
//  * Machine reuse semantics: Bulbapedia "TM" (TMs single-use in Gen I-IV,
//    reusable from Gen V) and "HM" (always reusable).
//
// ---------------------------------------------------------------------------
// INTEGRATION NOTE (for Thread C - Gen1*/Gen2*/Classic*, Thread S - Gen3/4/5
// surfaces and Shared*, and 2R - SaveSelectScreen/UI.cpp)
//
//  * A on an existing stack:
//        auto r = quantityRule(game, pocket, id);
//        if (r.editable) open the quantity editor clamped to [r.minimum, r.maximum];
//        else            show the item read-only (r.reason explains why), offer
//                        Remove only if the surface already supports it.
//  * Add:
//        1. item picker (offer only ids where quantityRule(...).valid and
//           mayAddItem(game, pocket, id, heldAlready) is true; heldAlready is the
//           total already held for that id in bag + PC);
//        2. then open the quantity editor ONLY if r.editable (i.e. r.maximum > 1);
//           otherwise stage quantity r.minimum (== 1) directly.
//        For a stackable item already held, the native games merge into the
//        existing stack: raise that stack (up to r.maximum) rather than
//        creating a duplicate slot.
//  * exclusiveWith(game, id) lists items the story only lets the player hold
//    one of (e.g. RSE Mach Bike / Acro Bike). It is advisory: show a warning,
//    do not hard-block - quantityRule/mayAddItem deliberately ignore it.
//  * Never widen r.maximum in the UI. Writers keep their own structural
//    checks; this policy is the UI-facing ceiling, not a replacement for them.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <span>
#include <string_view>

namespace PokeVault::Inventory::QuantityPolicy {

enum class Game : uint8_t {
    Red, Blue, Yellow,
    Gold, Silver, Crystal,
    Ruby, Sapphire, Emerald, FireRed, LeafGreen,
    Diamond, Pearl, Platinum, HeartGold, SoulSilver,
    Black, White, Black2, White2,
};
inline constexpr std::size_t kGameCount = 20;

// Logical pockets as the app's surfaces present them. Gen I has a single
// native bag list; Medicine/Balls/KeyItems/TMHM there are the logical split
// used by ClassicInventoryCatalog. Gen V stores Poké Balls in Items.
enum class Pocket : uint8_t {
    Items, Medicine, Balls, KeyItems, TMHM, Berries, Mail, BattleItems, PCItems,
};

enum class ItemKind : uint8_t { Invalid, Regular, KeyItem, TM, HM };

struct ItemQuantityRule {
    bool valid = false;      // native member of this game + pocket (id 0 / foreign ids: false)
    bool editable = false;   // offer a quantity editor (only when maximum > 1)
    uint16_t minimum = 0;    // 0 when !valid, else 1
    uint16_t maximum = 0;    // native per-slot cap for this item in this pocket
    bool unique = false;     // at most one may exist in the save
    ItemKind kind = ItemKind::Invalid;
    bool verified = true;    // false: fact not sourced, fell back to the native cap
    std::string_view reason; // short human-readable justification
};

[[nodiscard]] int generation(Game game) noexcept;
[[nodiscard]] std::string_view gameId(Game game) noexcept;            // e.g. "emerald_gba"
[[nodiscard]] bool gameFromId(std::string_view id, Game& out) noexcept;
[[nodiscard]] std::span<const Pocket> pockets(Game game) noexcept;
[[nodiscard]] bool supportsPocket(Game game, Pocket pocket) noexcept;

// Native per-slot cap of a pocket for ordinary (non-unique) items; 0 if the
// pocket does not exist in that game.
[[nodiscard]] uint16_t nativePocketCap(Game game, Pocket pocket) noexcept;

// Item classification independent of pocket (a key item in the PC is still a key item).
[[nodiscard]] ItemKind itemKind(Game game, uint16_t itemId) noexcept;

[[nodiscard]] ItemQuantityRule quantityRule(Game game, Pocket pocket, uint16_t itemId) noexcept;

// May the UI offer `itemId` in Add when the save already holds `heldAlready`
// copies of it (bag + PC total)? Unique items: only when none is held.
// Stackable items: only while the existing stack can still grow.
[[nodiscard]] bool mayAddItem(Game game, Pocket pocket, uint16_t itemId,
                              uint32_t heldAlready) noexcept;

// The story-exclusive group containing `itemId` (itemId itself included;
// advisory only, see integration note). Empty when none is documented.
// Documented groups: R/B/Y and FR/LG Dome/Helix Fossil, R/S/E Mach/Acro Bike,
// R/S/E Root/Claw Fossil.
[[nodiscard]] std::span<const uint16_t> exclusiveWith(Game game, uint16_t itemId) noexcept;

} // namespace PokeVault::Inventory::QuantityPolicy

#endif
