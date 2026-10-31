// Feature B (owner Oct 28): applying a confirmed Top Up / Add Missing plan is all or nothing, goes through
// the generation's own staged Bag editor, and keeps every edit staged before it. Synthetic saves only.
#include "Inventory/BulkItemApply.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "fuzz_synthetic_saves.h"

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace B = PokeVault::Inventory::BulkItems;
namespace C = PokeVault::Inventory;
namespace G1 = PokeVault::Integration::Gen1;

namespace {
// A minimal editor that can be told to refuse one item: proves the batch is atomic.
struct FakeEditor {
    std::map<uint16_t, uint16_t> qty;
    uint16_t refuse = 0;
};
struct FakeAdapter {
    bool setQuantity(FakeEditor& e, const B::TopUp& t, std::string& err) {
        if (t.itemId == e.refuse) { err = "refused"; return false; }
        e.qty[t.itemId] = t.to; return true;
    }
    bool add(FakeEditor& e, const B::Add& a, std::string& err) {
        if (a.itemId == e.refuse) { err = "refused"; return false; }
        e.qty[a.itemId] = a.quantity; return true;
    }
    uint16_t quantity(const FakeEditor& e, B::Pocket, uint16_t id, std::size_t) const {
        const auto it = e.qty.find(id); return it == e.qty.end() ? 0 : it->second;
    }
    bool singleStack(const FakeEditor&, B::Pocket, uint16_t) const { return true; }
};

// Gen I: one 20-slot Bag; logical pockets are views of it.
C::ClassicPocket classic(B::Pocket p) {
    switch (p) {
        case B::Pocket::Medicine: return C::ClassicPocket::Medicines;
        case B::Pocket::Balls: return C::ClassicPocket::Balls;
        case B::Pocket::KeyItems: return C::ClassicPocket::KeyItems;
        case B::Pocket::TMHM: return C::ClassicPocket::TMHM;
        case B::Pocket::PCItems: return C::ClassicPocket::PCItems;
        default: return C::ClassicPocket::Items;
    }
}
struct Gen1Adapter {
    bool setQuantity(G1::StagedInventoryEditor& e, const B::TopUp& t, std::string& err) {
        return e.stageSetQuantity(classic(t.pocket), t.itemId, t.to, err);
    }
    bool add(G1::StagedInventoryEditor& e, const B::Add& a, std::string& err) {
        return e.stageSetQuantity(classic(a.pocket), a.itemId, a.quantity, err);
    }
    uint16_t quantity(const G1::StagedInventoryEditor& e, B::Pocket p, uint16_t id, std::size_t) const {
        std::string err;
        for (const auto& entry : e.entries(classic(p), err)) if (entry.itemId == id) return entry.quantity;
        return 0;
    }
    bool singleStack(const G1::StagedInventoryEditor& e, B::Pocket p, uint16_t id) const {
        std::string err; int n = 0;
        for (const auto& entry : e.entries(classic(p), err)) n += entry.itemId == id;
        return n <= 1;
    }
};
}  // namespace

int main() {
    // Atomic: one refused step leaves the editor exactly as it was.
    {
        FakeEditor e{{{13, 5}, {14, 2}}, 0};
        B::Plan plan;
        plan.topUps = {{B::Pocket::Items, 0, 13, 5, 99}, {B::Pocket::Items, 1, 14, 2, 99}};
        plan.adds = {{B::Pocket::Items, 20, 10}};
        e.refuse = 20;
        const auto before = e.qty;
        auto r = B::apply(e, plan, FakeAdapter{});
        assert(!r.applied && !r.error.empty() && e.qty == before);
        e.refuse = 0;
        r = B::apply(e, plan, FakeAdapter{});
        assert(r.applied && r.toppedUp == 2 && r.added == 1);
        assert(e.qty[13] == 99 && e.qty[14] == 99 && e.qty[20] == 10);
        assert(!B::apply(e, B::Plan{}, FakeAdapter{}).applied);   // empty plan: nothing staged
    }
    // Real Gen I staged editor: a manual edit made first survives; the batch is ordinary staged work;
    // the finalized save reparses strictly with the planned quantities; the source is never written.
    {
        auto raw = qasynth::gen1::save();
        raw[0x25C9] = 0; raw[0x25CA] = 0xFF;    // empty Bag list (pret wNumBagItems)
        raw[0x27E6] = 0; raw[0x27E7] = 0xFF;    // empty PC item list
        qasynth::gen1::forge(raw);
        const auto source = raw;
        auto parsed = G1::parse(raw, G1::SourceGame::Red);
        assert(parsed);
        std::string err;
        auto editor = G1::StagedInventoryEditor::create(*parsed.save, err);
        assert(editor && err.empty());
        assert(editor->stageSetQuantity(C::ClassicPocket::Medicines, 20, 3, err));   // manual: Potion x3
        const auto manual = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());

        B::PocketState items{B::Pocket::Medicine, {}, 20, {}};
        std::size_t slot = 0;
        for (const auto& e : editor->entries(C::ClassicPocket::Medicines, err)) items.held.push_back({slot++, e.itemId, e.quantity});
        items.held.erase(std::remove_if(items.held.begin(), items.held.end(), [](const B::HeldStack& h) { return h.itemId != 20; }), items.held.end());
        const auto picks = C::addableItems(C::ClassicGame::Red, C::ClassicPocket::Medicines);
        items.addableItems.assign(picks.begin(), picks.begin() + 3);   // a few medicines
        const auto plan = B::plan(B::Game::Red, {B::Mode::Both, 40, {}}, std::span(&items, 1));
        assert(plan.topUps.size() == 1 && plan.topUps[0].itemId == 20 && plan.topUps[0].to == 40);

        // A failing adapter changes nothing, including the manual edit.
        struct Refusing : Gen1Adapter {
            bool add(G1::StagedInventoryEditor&, const B::Add&, std::string& e) { e = "no"; return false; }
        };
        if (!plan.adds.empty()) {
            assert(!B::apply(*editor, plan, Refusing{}).applied);
            assert(std::equal(manual.begin(), manual.end(), editor->stagedBytes().begin()));
        }
        const auto r = B::apply(*editor, plan, Gen1Adapter{});
        assert(r.applied && r.error.empty());
        const auto out = editor->finalize(err);
        assert(!out.empty() && err.empty());
        auto reparsed = G1::parse(out, G1::SourceGame::Red);
        assert(reparsed);
        auto check = G1::StagedInventoryEditor::create(*reparsed.save, err);
        assert(check);
        for (const auto& t : plan.topUps) assert(Gen1Adapter{}.quantity(*check, t.pocket, t.itemId, t.slot) == t.to);
        for (const auto& a : plan.adds) assert(Gen1Adapter{}.quantity(*check, a.pocket, a.itemId, 0) == a.quantity);
        assert(raw == source);
        assert(std::equal(source.begin(), source.end(), parsed.save->sourceBytes().begin()));
        // Discard drops the whole batch together with the manual edit, as any staged work.
        editor->discard();
        assert(!editor->hasPendingChanges());
    }
    std::cout << "Top Up / Add Missing apply: atomic, staged, strict reparse (Gen I end to end): PASS\n";
    return 0;
}
