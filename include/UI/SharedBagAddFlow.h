#ifndef POKEBANK_UI_SHARED_BAG_ADD_FLOW_H
#define POKEBANK_UI_SHARED_BAG_ADD_FLOW_H

// The ONE Bag "Add Item" state machine for every Gen I-V surface (Red -> White 2):
//
//   1 Bag / Items        2 Add (X)          3 shared item picker
//   4 highlight + A      5 the item is staged at quantity 1 at once: NO numpad
//   6 A on that row opens the quantity editor only if ItemQuantityPolicy says editable
//   7 unique items (key items, bikes, rods, HMs, Gen V TMs) never get an editor: fixed at 1
//   8 an item already in the pocket reuses that stack (focus it; never a duplicate entry)
//   9 B in the picker cancels with no mutation
//  10 everything stays staged in app memory until Review / Export
//
// Pure decision logic: no drawing, no input polling, no save I/O. Each surface keeps its own
// picker drawing and its own staged writer, and asks this header what a key press means, so
// the same press means the same thing in every game. Per-game limits come only from
// Inventory/ItemQuantityPolicy.h (pinned PKHeX 6501f0ab PlayerBag* caps), clamped to the
// surface's native writer bound so the UI can never offer a value the writer refuses.
//
// Callers:
//   ClassicInventoryOverlay.cpp selectPickerItem / EditAmount     (Gen I-III)
//   Gen4SharedPokemonSurface.inc handleItemQuantity               (Gen IV)
//   Gen5SharedReadOnlySurface.inc handleBag                       (Gen V)

#include "Inventory/ItemQuantityPolicy.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace PokeBank::UIModel::BagAdd {

namespace QP = PokeVault::Inventory::QuantityPolicy;

// What the surface knows about the pocket the player is adding to.
struct PocketState {
    uint16_t heldInPocket = 0;  // staged quantity of this item id already in the pocket (0 = none)
    std::size_t usedSlots = 0;  // occupied native slots in the pocket
    std::size_t capacity = 0;   // native slot count; 0 = unknown here, the staged writer decides
};

enum class PickOutcome : uint8_t {
    StageNew,       // stage `quantity` (always 1) as a new stack, then focus that row
    FocusExisting,  // already held: focus the existing stack, stage nothing
    PocketFull,     // refused with a precise message, stage nothing
    NotAllowed,     // not a native item for this game + pocket, stage nothing
};

struct PickDecision {
    PickOutcome outcome = PickOutcome::NotAllowed;
    uint16_t quantity = 0;  // only meaningful for StageNew
    std::string message;
};

enum class RowOutcome : uint8_t {
    EditQuantity,   // open the quantity editor on [minimum, maximum], starting at `current`
    FixedQuantity,  // unique item: no editor, nothing staged
    NotAllowed,     // invalid stack (unknown id, count beyond the native cap): nothing staged
};

struct RowDecision {
    RowOutcome outcome = RowOutcome::NotAllowed;
    uint16_t minimum = 0;
    uint16_t maximum = 0;
    uint16_t current = 0;
    std::string message;
};

// Effective maximum: the policy cap, never above the surface's native writer bound.
// writerCap == 0 means the surface has no separate bound.
[[nodiscard]] inline uint16_t effectiveMaximum(const QP::ItemQuantityRule& rule,
                                               uint16_t writerCap) noexcept {
    if (!rule.valid) return 0;
    return writerCap == 0 ? rule.maximum : std::min(rule.maximum, writerCap);
}

[[nodiscard]] inline PickDecision decidePick(const QP::ItemQuantityRule& rule,
                                             const PocketState& pocket,
                                             std::string_view pocketLabel = "This pocket") {
    PickDecision d{};
    if (!rule.valid) {
        d.message = "This item cannot be added to this pocket";
        return d;
    }
    if (pocket.heldInPocket != 0) {
        d.outcome = PickOutcome::FocusExisting;
        d.message = "Already in this pocket; highlighted the existing stack";
        return d;
    }
    if (pocket.capacity != 0 && pocket.usedSlots >= pocket.capacity) {
        d.outcome = PickOutcome::PocketFull;
        d.message = std::string(pocketLabel) + " is full (" + std::to_string(pocket.usedSlots) +
                    " of " + std::to_string(pocket.capacity) +
                    " slots); remove an item before adding";
        return d;
    }
    d.outcome = PickOutcome::StageNew;
    d.quantity = rule.minimum;  // 1 for every valid rule: never a numpad before the item exists
    d.message = rule.editable ? "Added x1 (staged only); press A on it to set the quantity"
                              : "Added (staged only); this item is unique, quantity fixed at 1";
    return d;
}

[[nodiscard]] inline PickDecision decidePick(QP::Game game, QP::Pocket pocket, uint16_t itemId,
                                             const PocketState& state,
                                             std::string_view pocketLabel = "This pocket") {
    return decidePick(QP::quantityRule(game, pocket, itemId), state, pocketLabel);
}

[[nodiscard]] inline RowDecision decideRow(const QP::ItemQuantityRule& rule, uint16_t current,
                                           uint16_t writerCap = 0) {
    RowDecision d{};
    d.current = current;
    const uint16_t maximum = effectiveMaximum(rule, writerCap);
    if (!rule.valid || current == 0 || maximum == 0 || current > maximum) {
        d.message = "This item's quantity is outside the native limits";
        return d;
    }
    if (!rule.editable || maximum <= 1) {
        d.outcome = RowOutcome::FixedQuantity;
        d.minimum = d.maximum = 1;
        d.message = "This item is unique; its quantity is fixed at 1";
        return d;
    }
    d.outcome = RowOutcome::EditQuantity;
    d.minimum = rule.minimum;
    d.maximum = maximum;
    return d;
}

[[nodiscard]] inline RowDecision decideRow(QP::Game game, QP::Pocket pocket, uint16_t itemId,
                                           uint16_t current, uint16_t writerCap = 0) {
    return decideRow(QP::quantityRule(game, pocket, itemId), current, writerCap);
}

// Key Items (bikes, rods, story items) are tied to story/event progress: every surface asks for
// an A-confirmed warning before one is added to or removed from the staged bag. Decided by the
// policy's item kind, never by pocket alone (a Key Item stored in the PC is still a Key Item).
[[nodiscard]] constexpr bool needsKeyItemWarning(const QP::ItemQuantityRule& rule) noexcept {
    return rule.valid && rule.kind == QP::ItemKind::KeyItem;
}

// The warning names the exact destination pocket (R2-CHECK-010: a Key Item added to the R/S PC
// must say "Add Super Rod to PC Items?", not leave the player guessing where it goes).
[[nodiscard]] inline std::string keyItemWarningQuestion(const std::string& itemName,
                                                        std::string_view pocketName, bool removing) {
    return removing ? "Remove " + itemName + " from " + std::string(pocketName) + "?"
                    : "Add " + itemName + " to " + std::string(pocketName) + "?";
}

// The picker phase as a tiny state machine. Each event is consumed once; nothing here mutates
// the bag. The surface performs the staging the returned decision names.
class Flow {
public:
    enum class Phase : uint8_t { Inventory, Picker };

    [[nodiscard]] Phase phase() const noexcept { return phase_; }
    [[nodiscard]] bool pickerOpen() const noexcept { return phase_ == Phase::Picker; }

    // X / Add. Refuses (stays in Inventory) when the picker would be empty.
    bool openPicker(bool hasChoices) noexcept {
        if (phase_ != Phase::Inventory || !hasChoices) return false;
        phase_ = Phase::Picker;
        return true;
    }
    // B in the picker: back to the inventory, no mutation.
    bool cancel() noexcept {
        if (phase_ != Phase::Picker) return false;
        phase_ = Phase::Inventory;
        return true;
    }
    // A in the picker. The picker closes for every outcome except NotAllowed (the player can
    // still choose another row). Outside the picker the press is ignored (NotAllowed).
    PickDecision pick(const QP::ItemQuantityRule& rule, const PocketState& pocket,
                      std::string_view pocketLabel = "This pocket") {
        if (phase_ != Phase::Picker) return {};
        auto d = decidePick(rule, pocket, pocketLabel);
        if (d.outcome != PickOutcome::NotAllowed) phase_ = Phase::Inventory;
        return d;
    }
    void reset() noexcept { phase_ = Phase::Inventory; }

private:
    Phase phase_ = Phase::Inventory;
};

} // namespace PokeBank::UIModel::BagAdd

#endif
