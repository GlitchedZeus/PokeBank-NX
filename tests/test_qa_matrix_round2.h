// PokeBank NX application QA: shared plumbing for the round 2 acceptance-matrix cells
// (bag-add, reroll) that every generation runs against its own production staged writer.
//
// Nothing here decides an outcome: the decisions come from UI/SharedBagAddFlow.h (decidePick /
// decideRow, the functions the Gen I-V Bag surfaces call) and the staging from the generation's
// real staged inventory editor, passed in as callbacks exactly as each surface wires them
// (ClassicInventoryOverlay.cpp stageQuantity for Gen I-III, Gen4SharedPokemonSurface.inc
// handleItemQuantity for Gen IV). Gen V goes through Gen5BagScreen::pickAdd in its own file.
#pragma once

#include "UI/SharedBagAddFlow.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace qamatrix::round2 {

namespace BagAdd = PokeBank::UIModel::BagAdd;
namespace QP = PokeVault::Inventory::QuantityPolicy;

// One item the cell picks, with the rule the surface would ask for it.
struct BagItem {
    std::string name;
    uint16_t id = 0;
    QP::ItemQuantityRule rule{};
    uint16_t writerCap = 0;  // the surface's native writer bound passed to decideRow (0 = none)
    std::size_t pocket = 0;  // the generation's own pocket code (ClassicPocket / Gen IV pocket index)
};

// What the surface reads from / writes to its staged writer.
struct BagBackend {
    std::function<BagAdd::PocketState(const BagItem&)> state;            // pocket state for the pick
    std::function<uint16_t(const BagItem&)> held;                        // staged quantity (0 = absent)
    std::function<bool(const BagItem&, uint16_t, std::string&)> stage;   // the surface's staging call
    std::function<std::vector<uint8_t>()> bytes;                         // staged image
};

// newItem: a stackable item absent from the bag; heldItem: a stackable item the save holds;
// uniqueItem: a Key Item absent from the bag. Returns every failed expectation.
inline std::vector<std::string> bagAddChecks(const BagBackend& b, const BagItem& newItem,
                                             const BagItem& heldItem, const BagItem& uniqueItem) {
    std::vector<std::string> bad;
    const auto expect = [&](bool ok, const std::string& what) {
        if (!ok) bad.push_back(what);
    };
    std::string error;

    // A new stackable item: StageNew at quantity 1 (= rule.minimum), no numpad; then staged x1.
    expect(newItem.rule.valid && newItem.rule.editable && newItem.rule.minimum == 1,
           newItem.name + " policy rule not a valid stackable rule");
    expect(b.held(newItem) == 0, newItem.name + " already in the synthetic bag");
    const auto pick = BagAdd::decidePick(newItem.rule, b.state(newItem));
    expect(pick.outcome == BagAdd::PickOutcome::StageNew, newItem.name + " pick is not StageNew");
    expect(pick.quantity == 1 && pick.quantity == newItem.rule.minimum,
           newItem.name + " staged quantity " + std::to_string(pick.quantity) + " is not the x1 minimum");
    if (pick.outcome == BagAdd::PickOutcome::StageNew) {
        expect(b.stage(newItem, pick.quantity, error), newItem.name + " staging refused: " + error);
        expect(b.held(newItem) == 1, newItem.name + " staged bag holds x" + std::to_string(b.held(newItem)));
    }
    // A on that row: the quantity editor opens on [1, cap].
    const auto row = BagAdd::decideRow(newItem.rule, b.held(newItem), newItem.writerCap);
    expect(row.outcome == BagAdd::RowOutcome::EditQuantity && row.minimum == 1 && row.maximum > 1 &&
               row.current == 1,
           newItem.name + " row is not EditQuantity [1, max] at 1");

    // Re-picking an item the bag already holds focuses it and stages nothing.
    const auto heldBefore = b.held(heldItem);
    const auto before = b.bytes();
    expect(heldBefore != 0, heldItem.name + " not held by the synthetic bag");
    const auto again = BagAdd::decidePick(heldItem.rule, b.state(heldItem));
    expect(again.outcome == BagAdd::PickOutcome::FocusExisting, heldItem.name + " re-pick is not FocusExisting");
    expect(b.bytes() == before && b.held(heldItem) == heldBefore, heldItem.name + " re-pick changed the stage");

    // A unique Key Item: staged at x1, then a fixed quantity (no editor; the writer refuses x2).
    expect(uniqueItem.rule.valid && !uniqueItem.rule.editable && uniqueItem.rule.maximum == 1,
           uniqueItem.name + " policy rule is not unique / max 1");
    expect(b.held(uniqueItem) == 0, uniqueItem.name + " already in the synthetic bag");
    const auto unique = BagAdd::decidePick(uniqueItem.rule, b.state(uniqueItem));
    expect(unique.outcome == BagAdd::PickOutcome::StageNew && unique.quantity == 1,
           uniqueItem.name + " pick is not StageNew x1");
    if (unique.outcome == BagAdd::PickOutcome::StageNew) {
        expect(b.stage(uniqueItem, unique.quantity, error), uniqueItem.name + " staging refused: " + error);
        expect(b.held(uniqueItem) == 1, uniqueItem.name + " staged bag does not hold x1");
    }
    const auto fixed = BagAdd::decideRow(uniqueItem.rule, 1, uniqueItem.writerCap);
    expect(fixed.outcome == BagAdd::RowOutcome::FixedQuantity && fixed.maximum == 1,
           uniqueItem.name + " row is not FixedQuantity");
    const auto fixedBytes = b.bytes();
    expect(!b.stage(uniqueItem, 2, error) && b.bytes() == fixedBytes, uniqueItem.name + " writer accepted x2");
    return bad;
}

inline std::string joined(const std::vector<std::string>& list) {
    std::string out;
    for (const auto& s : list) out += (out.empty() ? "" : "; ") + s;
    return out;
}

constexpr int kRerolls = 20;  // rerolls per shiny / non-shiny case (lane R runs 100)

} // namespace qamatrix::round2
