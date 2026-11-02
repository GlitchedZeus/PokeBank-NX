#pragma once
// Applies a confirmed BulkItems::Plan to a staged Bag editor, all or nothing (owner Oct 28, Feature B).
//
// The plan is first run on a COPY of the editor. Only if every top-up and add stages, and the copy then
// reads back exactly the planned quantities, does the copy replace the editor. A failure leaves the
// editor -- and every edit the user staged before -- untouched. Nothing here writes a file: the result
// is ordinary staged work that goes through Review, Keep / Discard and Export Copy like any manual edit.
//
// The adapter supplies the generation's own staged calls:
//   bool setQuantity(Editor&, const TopUp&, std::string& error)   // an existing native stack
//   bool add(Editor&, const Add&, std::string& error)             // a new stack in a free slot
//   uint16_t quantity(const Editor&, Pocket, uint16_t itemId, std::size_t slot)  // read back
//   bool singleStack(const Editor&, Pocket, uint16_t itemId)      // false if held in several slots

#include "Inventory/BulkItemPlan.h"

#include <string>
#include <utility>

namespace PokeVault::Inventory::BulkItems {

struct ApplyResult {
    bool applied = false;
    std::size_t toppedUp = 0;
    std::size_t added = 0;
    std::string error;
};

template <class Editor, class Adapter>
[[nodiscard]] ApplyResult apply(Editor& editor, const Plan& plan, Adapter&& adapter) {
    ApplyResult result;
    if (plan.empty()) { result.error = "Nothing to change"; return result; }
    Editor trial = editor;
    std::string error;
    for (const auto& t : plan.topUps) {
        if (!adapter.singleStack(trial, t.pocket, t.itemId)) {
            result.error = "An item is held in more than one slot; edit it on its own";
            return result;
        }
        if (!adapter.setQuantity(trial, t, error)) { result.error = error.empty() ? "A top-up was refused" : error; return result; }
    }
    // Top-ups read back by their native stack before any add runs: a writer that may place a new
    // stack in an interior gap (Gen V) would otherwise shift the visible row a top-up names.
    for (const auto& t : plan.topUps)
        if (adapter.quantity(trial, t.pocket, t.itemId, t.slot) != t.to) { result.error = "A top-up did not read back"; return result; }
    for (const auto& a : plan.adds) {
        if (!adapter.add(trial, a, error)) { result.error = error.empty() ? "An item could not be added" : error; return result; }
    }
    for (const auto& a : plan.adds)
        if (adapter.quantity(trial, a.pocket, a.itemId, SIZE_MAX) != a.quantity) { result.error = "An added item did not read back"; return result; }
    editor = std::move(trial);
    result.applied = true;
    result.toppedUp = plan.topUps.size();
    result.added = plan.adds.size();
    return result;
}

}  // namespace PokeVault::Inventory::BulkItems
