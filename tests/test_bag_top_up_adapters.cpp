// Top Up / Add Missing (owner Oct 28, Feature B): the Gen I-V adapters over the REAL staged Bag
// editors, end to end on synthetic saves (builders shared with tests/test_bag_add_flow_gen15.cpp and
// tests/fuzz_synthetic_saves.h; no real save, ROM or key is read).
//
// For every generation: the plan uses the Add picker's own item list; plan + apply reads back exactly
// in the staged editor; Key Items are never touched (TMs / HMs are planned, Round 12 H12); per-item
// native caps hold; a full pocket is reported, never forced; a refused step leaves the editor
// byte-identical (edits staged before included); the finalized / export candidate reparses strictly
// with the new counts; the source bytes never change; Discard restores the source. Gen II tops up
// each of an item's two stacks; Gen I / III report an item held in several stacks instead of merging it.
#include "Inventory/BulkItemAdaptersClassic.h"
#include "Inventory/BulkItemAdaptersNds.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "UI/BagTopUpModel.h"
#include "fuzz_synthetic_saves.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <span>
#include <string>
#include <vector>

namespace B = PokeVault::Inventory::BulkItems;
namespace I = PokeVault::Inventory;
namespace QP = PokeVault::Inventory::QuantityPolicy;
namespace Model = PokeBank::UIModel::BagTopUp;

static int failures = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            ++failures;                                                                   \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                                 \
    } while (0)

namespace {

std::size_t count(const B::Plan& plan, B::SkipReason reason, B::Pocket pocket) {
    return static_cast<std::size_t>(std::count_if(plan.skipped.begin(), plan.skipped.end(),
        [&](const B::Skip& s) { return s.reason == reason && s.pocket == pocket; }));
}
bool skipped(const B::Plan& plan, uint16_t id, B::SkipReason reason) {
    return std::any_of(plan.skipped.begin(), plan.skipped.end(),
                       [&](const B::Skip& s) { return s.itemId == id && s.reason == reason; });
}
const B::TopUp* topUp(const B::Plan& plan, uint16_t id, std::size_t slot = SIZE_MAX) {
    for (const auto& t : plan.topUps) if (t.itemId == id && (slot == SIZE_MAX || t.slot == slot)) return &t;
    return nullptr;
}
std::size_t addsIn(const B::Plan& plan, B::Pocket pocket) {
    return static_cast<std::size_t>(std::count_if(plan.adds.begin(), plan.adds.end(),
        [&](const B::Add& a) { return a.pocket == pocket; }));
}

// Story items are never planned; every planned count is within the item's native cap; candidate
// new items are exactly the Add picker's list for their pocket. `pc`: the scope may fill PC Items
// (Round 11 H34: PC Items / Bag + PC Items); "All Bag pockets" never does.
void planIsSafe(QP::Game game, const B::Plan& plan, std::span<const B::PocketState> state, bool pc = false) {
    for (const auto& t : plan.topUps) {
        const auto rule = QP::quantityRule(game, t.pocket, t.itemId);
        CHECK(rule.valid && !B::storySensitive(rule) && t.to <= rule.maximum && t.to > t.from);
    }
    for (const auto& a : plan.adds) {
        const auto rule = QP::quantityRule(game, a.pocket, a.itemId);
        CHECK(rule.valid && !B::storySensitive(rule) && a.quantity >= 1 && a.quantity <= rule.maximum);
        const auto p = std::find_if(state.begin(), state.end(), [&](const B::PocketState& s) { return s.pocket == a.pocket; });
        CHECK(p != state.end() && std::find(p->addableItems.begin(), p->addableItems.end(), a.itemId) != p->addableItems.end());
        CHECK(pc || a.pocket != B::Pocket::PCItems);   // "All Bag pockets" never fills the PC
    }
}

// Every planned change reads back exactly through the adapter.
template <class Editor, class Adapter>
void readsBack(const Editor& e, const B::Plan& plan, const Adapter& adapter) {
    for (const auto& t : plan.topUps) CHECK(adapter.quantity(e, t.pocket, t.itemId, t.slot) == t.to);
    for (const auto& a : plan.adds) CHECK(adapter.quantity(e, a.pocket, a.itemId, SIZE_MAX) == a.quantity);
}

// The real adapter, told to refuse the LAST add: proves the batch is all or nothing.
template <class Inner>
struct RefuseLastAdd {
    Inner inner;
    uint16_t refuse = 0;
    template <class E> bool setQuantity(E& e, const B::TopUp& t, std::string& err) { return inner.setQuantity(e, t, err); }
    template <class E> bool add(E& e, const B::Add& a, std::string& err) {
        if (a.itemId == refuse) { err = "refused"; return false; }
        return inner.add(e, a, err);
    }
    template <class E> uint16_t quantity(const E& e, B::Pocket p, uint16_t id, std::size_t s) const { return inner.quantity(e, p, id, s); }
    template <class E> bool singleStack(const E& e, B::Pocket p, uint16_t id) const { return inner.singleStack(e, p, id); }
};
template <class Inner>
RefuseLastAdd<Inner> refusing(Inner inner, const B::Plan& plan) {
    return {inner, plan.adds.empty() ? uint16_t{0} : plan.adds.back().itemId};
}

std::size_t topUpsIn(const B::Plan& plan, B::Pocket pocket) {
    return static_cast<std::size_t>(std::count_if(plan.topUps.begin(), plan.topUps.end(),
        [&](const B::TopUp& t) { return t.pocket == pocket; }));
}
bool added(const B::Plan& plan, B::Pocket pocket, uint16_t id) {
    return std::any_of(plan.adds.begin(), plan.adds.end(), [&](const B::Add& a) { return a.pocket == pocket && a.itemId == id; });
}
// No story item (Key Item / HM) is ever planned as a change in any list, PC included.
void noStoryChange(QP::Game game, const B::Plan& plan) {
    for (const auto& a : plan.adds) CHECK(!B::storySensitive(QP::quantityRule(game, a.pocket, a.itemId)));
    for (const auto& t : plan.topUps) CHECK(!B::storySensitive(QP::quantityRule(game, t.pocket, t.itemId)));
}
Model::State scoped(Model::Scope scope, uint16_t quantity, B::Mode mode = B::Mode::Both) {
    Model::State s;
    s.scope = scope;
    s.quantity = quantity;
    s.mode = mode;
    return s;
}

std::vector<uint16_t> addable(I::ClassicGame g, I::ClassicPocket p) {
    const auto s = I::addableItems(g, p);
    return {s.begin(), s.end()};
}

// ------------------------------------------------------------------------------------- Gen I --
namespace gen1 {
namespace G1 = PokeVault::Integration::Gen1;
std::vector<uint8_t> fixture(const std::vector<std::pair<uint8_t, uint8_t>>& bag,
                             const std::vector<std::pair<uint8_t, uint8_t>>& pc = {}) {
    auto raw = qasynth::gen1::save();
    raw[0x25C9] = static_cast<uint8_t>(bag.size());          // pret wNumBagItems
    for (std::size_t i = 0; i < bag.size(); ++i) { raw[0x25CA + 2 * i] = bag[i].first; raw[0x25CB + 2 * i] = bag[i].second; }
    raw[0x25CA + 2 * bag.size()] = 0xFF;
    raw[0x27E6] = static_cast<uint8_t>(pc.size());           // pret wNumBoxItems (empty by default)
    for (std::size_t i = 0; i < pc.size(); ++i) { raw[0x27E7 + 2 * i] = pc[i].first; raw[0x27E8 + 2 * i] = pc[i].second; }
    raw[0x27E7 + 2 * pc.size()] = 0xFF;
    qasynth::gen1::forge(raw);
    return raw;
}
void run() {
    // 18 of 20 Bag slots: Potion twice (two stacks), Poke Ball x2, Bicycle, HM01, TM01 x1 and filler.
    std::vector<std::pair<uint8_t, uint8_t>> bag{{20, 3}, {20, 5}, {4, 2}, {6, 1}, {196, 1}, {201, 1}};
    for (const auto id : addable(I::ClassicGame::Red, I::ClassicPocket::Items)) {
        if (bag.size() >= 18) break;
        if (std::none_of(bag.begin(), bag.end(), [&](const auto& e) { return e.first == id; }))
            bag.push_back({static_cast<uint8_t>(id), 1});
    }
    const auto raw = fixture(bag);
    const auto source = raw;
    auto parsed = G1::parse(raw, G1::SourceGame::Red);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    std::string err;
    auto editor = G1::StagedInventoryEditor::create(*parsed.save, err);
    CHECK(editor != nullptr);
    if (!editor) return;
    // An edit the user staged first survives every outcome below.
    CHECK(editor->stageSetQuantity(I::ClassicPocket::Balls, 4, 7, err));

    const auto state = B::Gen1Bag::pockets(*editor, err);
    CHECK(err.empty() && !state.empty());
    for (const auto& p : state) {
        const auto classic = B::classicPocketFor(I::ClassicGame::Red, p.pocket);
        CHECK(classic.has_value());
        if (classic) CHECK(p.addableItems == addable(I::ClassicGame::Red, *classic));   // the Add picker's list
    }
    const auto request = Model::request(
        [] { Model::State s; s.scope = Model::Scope::AllBag; s.quantity = 999; return s; }(), B::Pocket::Medicine, state);
    const auto plan = B::plan(B::Gen1Bag::game(*editor), request, state);
    planIsSafe(QP::Game::Red, plan, state);
    CHECK(skipped(plan, 20, B::SkipReason::SeveralStacks) && !topUp(plan, 20));   // never merges two stacks
    CHECK(topUp(plan, 4) && topUp(plan, 4)->from == 7 && topUp(plan, 4)->to == 99);  // 999 asked, native 99
    CHECK(topUp(plan, 201) && topUp(plan, 201)->to == 99);
    // Round 12 owner H12 ("can we add all of the tm/hm's instead of excluding them"): an HM is an
    // ordinary unique TM/HM item now, so a held one is "only one" (never topped up), not excluded.
    CHECK(skipped(plan, 6, B::SkipReason::KeyOrStoryItem) && skipped(plan, 196, B::SkipReason::OnlyOnce));
    CHECK(plan.adds.size() == 2);   // one 20-slot Bag shared by every category: 2 free slots in all
    std::size_t full = 0;
    for (const auto& s : plan.skipped) full += s.reason == B::SkipReason::PocketFull;
    CHECK(full > 0);
    // Round 11 (H34 "gen 1 doesnt seems to work"): the preview says WHY: one shared 20-slot list,
    // reported once (not once per category), and every item that did not fit names it.
    CHECK(plan.full.size() == 1 && plan.full[0].sharedList && plan.full[0].slots == 20 &&
          plan.full[0].freeBefore == 2 && plan.full[0].notFitted == full);
    for (const auto& s : plan.skipped)
        if (s.reason == B::SkipReason::PocketFull)
            CHECK(s.sharedList && s.slots == 20 && B::skipText(s) == "Bag full: its 20 slots are shared by every category");
    {
        const auto lines = Model::previewLines(plan, request.pockets, [](B::Pocket, uint16_t id) { return std::to_string(id); },
                                               [](B::Pocket) { return std::string("Pocket"); });
        CHECK(!lines.empty() && lines[0].kind == Model::LineKind::Note &&
              lines[0].text == "Bag (every category shares 20 slots): 2 free, " + std::to_string(full) + " more did not fit");
    }

    const auto stagedBefore = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
    const auto changesBefore = editor->pendingChanges().size();
    CHECK(!B::apply(*editor, plan, refusing(B::Gen1Bag::Adapter(*editor), plan)).applied);
    CHECK(std::equal(stagedBefore.begin(), stagedBefore.end(), editor->stagedBytes().begin()));
    CHECK(editor->pendingChanges().size() == changesBefore);

    const auto result = B::apply(*editor, plan, B::Gen1Bag::Adapter(*editor));
    CHECK(result.applied && result.toppedUp == plan.topUps.size() && result.added == 2);
    readsBack(*editor, plan, B::Gen1Bag::Adapter(*editor));
    CHECK(B::Gen1Bag::Adapter(*editor).quantity(*editor, B::Pocket::KeyItems, 6, SIZE_MAX) == 1);
    CHECK(B::Gen1Bag::Adapter(*editor).quantity(*editor, B::Pocket::TMHM, 196, SIZE_MAX) == 1);
    CHECK(editor->freeSlots(I::ClassicPocket::Items, err) == 0);
    CHECK(editor->pendingChanges().size() == changesBefore + plan.topUps.size() + 2 - 1);  // Poke Ball row reused

    const auto out = editor->finalize(err);
    CHECK(!out.empty());
    auto reparsed = G1::parse(out, G1::SourceGame::Red);
    CHECK(static_cast<bool>(reparsed));
    if (reparsed) {
        auto check = G1::StagedInventoryEditor::create(*reparsed.save, err);
        CHECK(check != nullptr);
        if (check) readsBack(*check, plan, B::Gen1Bag::Adapter(*check));
    }
    CHECK(raw == source);
    CHECK(std::equal(source.begin(), source.end(), editor->originalBytes().begin()));
    editor->discard();
    CHECK(std::equal(source.begin(), source.end(), editor->stagedBytes().begin()) && !editor->hasPendingChanges());
}

// Round 11 (owner H34: "the pc items doesnt get filled"): the PC Items scope fills the 50-slot PC
// list only; the 20-slot Bag bytes stay exactly as they were. Bag + PC Items plans each list on its
// own (an ordinary item may natively be in both), Key Items / HMs offered by the PC list are reported.
void pc() {
    // Bag: Potion, Antidote and 16 filler (2 slots free). PC: Potion x5, Bicycle, HM01, Max Elixer x2.
    std::vector<std::pair<uint8_t, uint8_t>> bag{{20, 3}, {11, 1}};
    for (const auto id : addable(I::ClassicGame::Red, I::ClassicPocket::Items)) {
        if (bag.size() >= 18) break;
        if (id != 10) bag.push_back({static_cast<uint8_t>(id), 1});   // 10 (Moon Stone) stays free for both lists
    }
    const std::vector<std::pair<uint8_t, uint8_t>> pcList{{20, 5}, {6, 1}, {196, 1}, {83, 2}};
    const auto raw = fixture(bag, pcList);
    const auto source = raw;
    auto parsed = G1::parse(raw, G1::SourceGame::Blue);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    std::string err;
    auto editor = G1::StagedInventoryEditor::create(*parsed.save, err);
    CHECK(editor != nullptr);
    if (!editor) return;
    const auto state = B::Gen1Bag::pockets(*editor, err);
    CHECK(err.empty() && B::hasPcItems(state));
    const auto pcState = std::find_if(state.begin(), state.end(), [](const B::PocketState& p) { return p.pocket == B::Pocket::PCItems; });
    CHECK(pcState != state.end() && pcState->slotCapacity == 50 && pcState->slotGroup < 0);
    const auto bagBefore = editor->entries(I::ClassicPocket::Items, err);

    // PC Items: adds only into the PC, at most its 46 free slots; Potion x5 -> 99; story items reported.
    const auto pcPlan = B::plan(QP::Game::Blue, Model::request(scoped(Model::Scope::PcItems, 99), B::Pocket::Items, state), state);
    planIsSafe(QP::Game::Blue, pcPlan, state, true);
    noStoryChange(QP::Game::Blue, pcPlan);
    for (const auto& a : pcPlan.adds) CHECK(a.pocket == B::Pocket::PCItems);
    for (const auto& t : pcPlan.topUps) CHECK(t.pocket == B::Pocket::PCItems);
    CHECK(addsIn(pcPlan, B::Pocket::PCItems) == 46 && count(pcPlan, B::SkipReason::PocketFull, B::Pocket::PCItems) > 0);
    CHECK(topUp(pcPlan, 20) && topUp(pcPlan, 20)->from == 5 && topUp(pcPlan, 20)->to == 99);
    CHECK(topUp(pcPlan, 83) && topUp(pcPlan, 83)->to == 99);
    // Round 12 owner H12 ("can we add all of the tm/hm's instead of excluding them"): an HM is an
    // ordinary unique TM/HM item now, so a held one is "only one" (never topped up), not excluded.
    CHECK(skipped(pcPlan, 6, B::SkipReason::KeyOrStoryItem) && skipped(pcPlan, 196, B::SkipReason::OnlyOnce));
    for (const uint16_t key : {5, 31}) CHECK(!added(pcPlan, B::Pocket::PCItems, key) && skipped(pcPlan, key, B::SkipReason::KeyOrStoryItem));
    // HM02-05 are eligible; the 46 free PC slots take lower item ids first, so they did not fit.
    for (const uint16_t hm : {197, 198, 199, 200}) CHECK(skipped(pcPlan, hm, B::SkipReason::PocketFull));
    CHECK(pcPlan.full.size() == 1 && !pcPlan.full[0].sharedList && pcPlan.full[0].slots == 50 && pcPlan.full[0].freeBefore == 46);

    const auto result = B::apply(*editor, pcPlan, B::Gen1Bag::Adapter(*editor));
    CHECK(result.applied && result.added == 46);
    readsBack(*editor, pcPlan, B::Gen1Bag::Adapter(*editor));
    CHECK(editor->entries(I::ClassicPocket::Items, err).size() == bagBefore.size());
    CHECK(std::equal(raw.begin() + 0x25C9, raw.begin() + 0x25C9 + 42, editor->stagedBytes().begin() + 0x25C9));  // Bag bytes
    CHECK(editor->freeSlots(I::ClassicPocket::PCItems, err) == 0);
    const auto out = editor->finalize(err);
    CHECK(!out.empty());
    auto reparsed = out.empty() ? G1::ParseResult{} : G1::parse(out, G1::SourceGame::Blue);
    CHECK(static_cast<bool>(reparsed));
    if (reparsed) {
        auto check = G1::StagedInventoryEditor::create(*reparsed.save, err);
        CHECK(check != nullptr);
        if (check) {
            readsBack(*check, pcPlan, B::Gen1Bag::Adapter(*check));
            CHECK(B::Gen1Bag::Adapter(*check).quantity(*check, B::Pocket::PCItems, 6, SIZE_MAX) == 1);
            CHECK(B::Gen1Bag::Adapter(*check).quantity(*check, B::Pocket::PCItems, 196, SIZE_MAX) == 1);
        }
    }
    editor->discard();
    CHECK(std::equal(source.begin(), source.end(), editor->stagedBytes().begin()) && !editor->hasPendingChanges());

    // Bag + PC Items: the shared Bag takes its 2 free slots, the PC its own 46; Moon Stone (10), an
    // ordinary item held nowhere, may go into BOTH lists (native); Potion is topped up in each list.
    const auto both = B::plan(QP::Game::Blue, Model::request(scoped(Model::Scope::BagAndPc, 99), B::Pocket::Items, state), state);
    planIsSafe(QP::Game::Blue, both, state, true);
    noStoryChange(QP::Game::Blue, both);
    std::size_t bagAdds = 0;
    for (const auto& a : both.adds) bagAdds += a.pocket != B::Pocket::PCItems;
    CHECK(bagAdds == 2 && addsIn(both, B::Pocket::PCItems) == 46);
    CHECK(added(both, B::Pocket::Items, 10) && added(both, B::Pocket::PCItems, 10));
    CHECK(topUp(both, 20) && topUpsIn(both, B::Pocket::PCItems) >= 2);
    CHECK(both.full.size() == 2);   // the shared Bag once, the PC once
    CHECK(B::apply(*editor, both, B::Gen1Bag::Adapter(*editor)).applied);
    readsBack(*editor, both, B::Gen1Bag::Adapter(*editor));
    CHECK(editor->freeSlots(I::ClassicPocket::Items, err) == 0 && editor->freeSlots(I::ClassicPocket::PCItems, err) == 0);
    const auto out2 = editor->finalize(err);
    auto reparsed2 = out2.empty() ? G1::ParseResult{} : G1::parse(out2, G1::SourceGame::Blue);
    CHECK(static_cast<bool>(reparsed2));
    if (reparsed2) {
        auto check = G1::StagedInventoryEditor::create(*reparsed2.save, err);
        CHECK(check != nullptr);
        if (check) readsBack(*check, both, B::Gen1Bag::Adapter(*check));
    }
    CHECK(raw == source);
    editor->discard();
    CHECK(std::equal(source.begin(), source.end(), editor->stagedBytes().begin()) && !editor->hasPendingChanges());
}

// Round 11 skeptic: Max Elixer (83) is a Medicine. It used to be in the PC list only, so a Max
// Elixer in the Bag showed on no category (yet held one of the 20 slots) and was never topped up.
void maxElixer() {
    const auto raw = fixture({{83, 4}});
    auto parsed = G1::parse(raw, G1::SourceGame::Red);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    std::string err;
    auto editor = G1::StagedInventoryEditor::create(*parsed.save, err);
    CHECK(editor != nullptr);
    if (!editor) return;
    const auto medicine = editor->entries(I::ClassicPocket::Medicines, err);
    CHECK(medicine.size() == 1 && medicine[0].itemId == 83 && medicine[0].quantity == 4);
    const auto state = B::Gen1Bag::pockets(*editor, err);
    const auto plan = B::plan(QP::Game::Red, Model::request(scoped(Model::Scope::AllBag, 99, B::Mode::TopUpExisting),
                                                              B::Pocket::Items, state), state);
    CHECK(topUp(plan, 83) && topUp(plan, 83)->pocket == B::Pocket::Medicine && topUp(plan, 83)->to == 99);
}
} // namespace gen1

// ------------------------------------------------------------------------------------ Gen II --
namespace gen2 {
using namespace PokeVault::Integration::Gen2;
// The PKHeX-shaped international layout of tests/test_bag_add_flow_gen15.cpp (offsets: PKHeX SAV2Offsets).
struct L {
    VersionFamily family;
    std::size_t money, idx, names, party, current, end, sum, gender;
    std::size_t tm, items, key, balls, pc;
};
constexpr std::size_t N = static_cast<std::size_t>(-1);
constexpr L C{VersionFamily::Crystal, 0x23DC, 0x2700, 0x2703, 0x2865, 0x2D10, 0x2B82, 0x2D0D, 0x3E3D,
              0x23E7, 0x2420, 0x244A, 0x2465, 0x247F};
std::size_t stride() { return 1 + 21 + 20 * 32 + 2 * 20 * 11 + 2; }
std::size_t boxStart(std::size_t b) { return b < 7 ? 0x4000 + b * stride() : 0x6000 + (b - 7) * stride(); }
void name(std::vector<uint8_t>& d, std::size_t ofs, std::size_t len, char c) {
    std::fill_n(d.begin() + static_cast<std::ptrdiff_t>(ofs), len, 0x50);
    d[ofs] = static_cast<uint8_t>(0x80 + c - 'A');
}
void pokemonList(std::vector<uint8_t>& d, std::size_t ofs, std::size_t cap, std::size_t bodySize, std::size_t str, bool party) {
    constexpr uint8_t species = 25;
    d[ofs] = 1; d[ofs + 1] = species; d[ofs + 2] = 0xFF;
    const std::size_t body = ofs + 1 + (cap + 1);
    d[body] = species; d[body + 1] = 1; d[body + 2] = 33; d[body + 3] = 45;
    d[body + 6] = 0x12; d[body + 7] = 0x34; d[body + 9] = 0x10;
    for (int i = 0; i < 5; i++) { d[body + 11 + 2 * i] = 0; d[body + 12 + 2 * i] = static_cast<uint8_t>(i + 1); }
    d[body + 21] = 0x7A; d[body + 22] = 0xAA; d[body + 23] = 10; d[body + 24] = 20;
    d[body + 27] = 123; d[body + 28] = 0x21; d[body + 31] = 15;
    if (party) {
        auto be = [&](std::size_t o, uint16_t v) { d[body + o] = static_cast<uint8_t>(v >> 8); d[body + o + 1] = static_cast<uint8_t>(v); };
        d[body + 32] = 0x08; be(34, 35); be(36, 40); be(38, 30); be(40, 20); be(42, 25); be(44, 26); be(46, 27);
    }
    const std::size_t ot = body + cap * bodySize;
    name(d, ot, str, 'O'); name(d, ot + cap * str, str, 'M');
}
std::vector<uint8_t> fixture(const L& l, const std::vector<std::pair<uint8_t, uint8_t>>& items,
                             const std::vector<std::pair<uint8_t, uint8_t>>& pc = {}) {
    std::vector<uint8_t> d(0x8000);
    d[0x2009] = 0x12; d[0x200A] = 0x34; name(d, 0x200B, 11, 'A');
    d[l.money] = 0x01; d[l.money + 1] = 0xE2; d[l.money + 2] = 0x40;
    d[l.idx] = 0; if (l.gender != N) d[l.gender] = 1;
    for (std::size_t b = 0; b < 14; b++) { d[boxStart(b)] = 0; d[boxStart(b) + 1] = 0xFF; name(d, l.names + b * 9, 9, static_cast<char>('A' + b)); }
    pokemonList(d, boxStart(0), 20, 32, 11, false);
    d[l.current] = 0; d[l.current + 1] = 0xFF; pokemonList(d, l.party, 6, 48, 11, true);
    std::fill_n(d.begin() + static_cast<std::ptrdiff_t>(l.tm), 57, 0);
    d[l.tm + 0] = 1;    // TM01 x1
    d[l.tm + 50] = 1;   // HM01
    d[l.items] = static_cast<uint8_t>(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) { d[l.items + 1 + 2 * i] = items[i].first; d[l.items + 2 + 2 * i] = items[i].second; }
    d[l.items + 1 + 2 * items.size()] = 0xFF;
    d[l.key] = 1; d[l.key + 1] = 7; d[l.key + 2] = 0xFF;      // Bicycle
    d[l.balls] = 0; d[l.balls + 1] = 0xFF;
    d[l.pc] = static_cast<uint8_t>(pc.size());
    for (std::size_t i = 0; i < pc.size(); ++i) { d[l.pc + 1 + 2 * i] = pc[i].first; d[l.pc + 2 + 2 * i] = pc[i].second; }
    d[l.pc + 1 + 2 * pc.size()] = 0xFF;
    uint16_t s = 0;
    for (std::size_t i = 0x2009; i <= l.end; i++) s = static_cast<uint16_t>(s + d[i]);
    d[l.sum] = static_cast<uint8_t>(s); d[l.sum + 1] = static_cast<uint8_t>(s >> 8);
    return d;
}
void run() {
    // Potion held in TWO stacks (40 and 30), Antidote x5, then filler up to 19 of 20 slots.
    std::vector<std::pair<uint8_t, uint8_t>> items{{kPotionItemId, 40}, {kPotionItemId, 30}, {9, 5}};
    for (const auto id : addable(I::ClassicGame::Crystal, I::ClassicPocket::Items)) {
        if (items.size() >= 19) break;
        if (std::none_of(items.begin(), items.end(), [&](const auto& e) { return e.first == id; }))
            items.push_back({static_cast<uint8_t>(id), 99});
    }
    const auto raw = fixture(C, items);
    const auto source = raw;
    auto parsed = parse(raw, SourceGame::Crystal);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    std::string err;
    auto editor = StagedEditor::create(*parsed.save, err);
    CHECK(editor != nullptr);
    if (!editor) return;
    const auto state = B::Gen2Bag::pockets(*editor, err);
    CHECK(err.empty() && state.size() == 5);

    // This pocket (Items), top up only: each Potion stack on its own.
    Model::State s;
    s.mode = B::Mode::TopUpExisting;
    CHECK(s.scope == Model::Scope::ThisPocket);   // a State that was never opened plans only the pocket on screen
    const auto items1 = B::plan(B::Gen2Bag::game(*editor), Model::request(s, B::Pocket::Items, state), state);
    planIsSafe(QP::Game::Crystal, items1, state);
    CHECK(topUp(items1, kPotionItemId, 0) && topUp(items1, kPotionItemId, 0)->to == 99);
    CHECK(topUp(items1, kPotionItemId, 1) && topUp(items1, kPotionItemId, 1)->from == 30);
    CHECK(topUp(items1, 9) && topUp(items1, 9)->to == 99 && items1.adds.empty());
    const auto staged0 = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
    // A refused top-up (the second Potion stack moved) changes nothing.
    {
        auto bad = items1;
        for (auto& t : bad.topUps) if (t.itemId == kPotionItemId && t.slot == 1) t.slot = 2;
        CHECK(!B::apply(*editor, bad, B::Gen2Bag::Adapter{}).applied);
        CHECK(std::equal(staged0.begin(), staged0.end(), editor->stagedBytes().begin()) && !editor->hasPendingChanges());
    }
    CHECK(B::apply(*editor, items1, B::Gen2Bag::Adapter{}).applied);
    readsBack(*editor, items1, B::Gen2Bag::Adapter{});
    bool secondStackRow = false;
    for (const auto& c : editor->pendingChanges()) secondStackRow |= c.label.find("(stack 2)") != std::string::npos && c.afterValue == "99";
    CHECK(secondStackRow);   // Review names the second stack on its own

    // All pockets, both modes: one free Items slot; Balls fill; Key Items / HMs untouched.
    s.mode = B::Mode::Both;
    s.scope = Model::Scope::AllBag;
    const auto state2 = B::Gen2Bag::pockets(*editor, err);
    const auto all = B::plan(B::Gen2Bag::game(*editor), Model::request(s, B::Pocket::Items, state2), state2);
    planIsSafe(QP::Game::Crystal, all, state2);
    CHECK(addsIn(all, B::Pocket::Items) == 1 && count(all, B::SkipReason::PocketFull, B::Pocket::Items) > 0);
    CHECK(addsIn(all, B::Pocket::Balls) > 0 && addsIn(all, B::Pocket::Balls) <= 12);
    // Round 12 owner H12 ("can we add all of the tm/hm's instead of excluding them"): an HM is an
    // ordinary unique TM/HM item now, so a held one is "only one" (never topped up), not excluded.
    CHECK(skipped(all, 7, B::SkipReason::KeyOrStoryItem) && skipped(all, 243, B::SkipReason::OnlyOnce));
    for (uint16_t hm = 244; hm <= 249; ++hm) CHECK(added(all, B::Pocket::TMHM, hm));   // HM02-07, one each
    CHECK(topUp(all, 191) && topUp(all, 191)->to == 99);   // TM01
    const auto staged1 = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
    const auto changes1 = editor->pendingChanges().size();
    CHECK(!B::apply(*editor, all, refusing(B::Gen2Bag::Adapter{}, all)).applied);
    CHECK(std::equal(staged1.begin(), staged1.end(), editor->stagedBytes().begin()) && editor->pendingChanges().size() == changes1);
    CHECK(B::apply(*editor, all, B::Gen2Bag::Adapter{}).applied);
    readsBack(*editor, all, B::Gen2Bag::Adapter{});
    CHECK(editor->stagedInventoryQuantity(InventoryPocket::KeyItems, 7) == 1);
    CHECK(editor->stagedInventoryQuantity(InventoryPocket::TMHM, 243) == 1);

    const auto out = editor->finalizedBytes(err);
    CHECK(!out.empty());
    auto reopened = parse(out, SourceGame::Crystal);
    CHECK(static_cast<bool>(reopened));
    if (reopened) {
        auto again = StagedEditor::create(*reopened.save, err);
        CHECK(again != nullptr);
        if (again) {
            readsBack(*again, items1, B::Gen2Bag::Adapter{});
            readsBack(*again, all, B::Gen2Bag::Adapter{});
            const auto list = again->inventoryEntries(InventoryPocket::Items, err);
            CHECK(list.size() == 20 && list[0].quantity == 99 && list[1].itemId == kPotionItemId && list[1].quantity == 99);
        }
    }
    CHECK(raw == source);
    CHECK(std::equal(source.begin(), source.end(), editor->originalBytes().begin()));
    editor->discard();
    CHECK(std::equal(source.begin(), source.end(), editor->stagedBytes().begin()) && !editor->hasPendingChanges());
}

// Round 11 (H34): Crystal PC Items. The PC is a 50-pair list where one item may sit in two slots:
// each stack is topped up on its own; new stacks fill only its free pairs; Key Items (Crystal's own
// 70 / 115 / 116 / 129 included) and HMs are reported; the Bag bytes stay as they were.
void pc() {
    std::vector<std::pair<uint8_t, uint8_t>> pcList{{kPotionItemId, 10}, {kPotionItemId, 20}, {7, 1}, {243, 1}};
    for (const auto id : addable(I::ClassicGame::Crystal, I::ClassicPocket::Items)) {
        if (pcList.size() >= 45) break;
        if (id != kPotionItemId) pcList.push_back({static_cast<uint8_t>(id), 99});
    }
    const auto raw = fixture(C, {{9, 5}}, pcList);
    const auto source = raw;
    auto parsed = parse(raw, SourceGame::Crystal);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    std::string err;
    auto editor = StagedEditor::create(*parsed.save, err);
    CHECK(editor != nullptr);
    if (!editor) return;
    const auto state = B::Gen2Bag::pockets(*editor, err);
    CHECK(err.empty() && B::hasPcItems(state));
    const auto pcState = std::find_if(state.begin(), state.end(), [](const B::PocketState& p) { return p.pocket == B::Pocket::PCItems; });
    CHECK(pcState != state.end() && pcState->slotCapacity == 50 &&
          pcState->addableItems == addable(I::ClassicGame::Crystal, I::ClassicPocket::PCItems));
    // Gold / Silver offer a different PC list (no Crystal-only key items).
    CHECK(addable(I::ClassicGame::Gold, I::ClassicPocket::PCItems) != addable(I::ClassicGame::Crystal, I::ClassicPocket::PCItems));

    const auto plan = B::plan(B::Gen2Bag::game(*editor), Model::request(scoped(Model::Scope::PcItems, 99), B::Pocket::Items, state), state);
    planIsSafe(QP::Game::Crystal, plan, state, true);
    noStoryChange(QP::Game::Crystal, plan);
    for (const auto& a : plan.adds) CHECK(a.pocket == B::Pocket::PCItems);
    for (const auto& t : plan.topUps) CHECK(t.pocket == B::Pocket::PCItems);
    CHECK(topUp(plan, kPotionItemId, 0) && topUp(plan, kPotionItemId, 0)->from == 10);
    CHECK(topUp(plan, kPotionItemId, 1) && topUp(plan, kPotionItemId, 1)->from == 20);
    // Round 12 owner H12 ("can we add all of the tm/hm's instead of excluding them"): an HM is an
    // ordinary unique TM/HM item now, so a held one is "only one" (never topped up), not excluded.
    CHECK(skipped(plan, 7, B::SkipReason::KeyOrStoryItem) && skipped(plan, 243, B::SkipReason::OnlyOnce));
    for (const uint16_t key : {70, 115, 116, 129}) CHECK(!added(plan, B::Pocket::PCItems, key));
    CHECK(addsIn(plan, B::Pocket::PCItems) == 50 - pcList.size() && count(plan, B::SkipReason::PocketFull, B::Pocket::PCItems) > 0);
    CHECK(plan.full.size() == 1 && plan.full[0].pocket == B::Pocket::PCItems && plan.full[0].slots == 50);

    const auto bagBefore = editor->inventoryEntries(InventoryPocket::Items, err);
    const auto staged0 = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
    CHECK(!B::apply(*editor, plan, refusing(B::Gen2Bag::Adapter{}, plan)).applied);
    CHECK(std::equal(staged0.begin(), staged0.end(), editor->stagedBytes().begin()) && !editor->hasPendingChanges());
    CHECK(B::apply(*editor, plan, B::Gen2Bag::Adapter{}).applied);
    readsBack(*editor, plan, B::Gen2Bag::Adapter{});
    const auto bagAfter = editor->inventoryEntries(InventoryPocket::Items, err);
    CHECK(bagAfter.size() == bagBefore.size() && bagAfter[0].itemId == 9 && bagAfter[0].quantity == 5);
    CHECK(editor->stagedInventoryQuantity(InventoryPocket::PCItems, 7) == 1);
    const auto out = editor->finalizedBytes(err);
    CHECK(!out.empty());
    auto reopened = out.empty() ? ParseResult{} : parse(out, SourceGame::Crystal);
    CHECK(static_cast<bool>(reopened));
    if (reopened) {
        auto again = StagedEditor::create(*reopened.save, err);
        CHECK(again != nullptr);
        if (again) {
            readsBack(*again, plan, B::Gen2Bag::Adapter{});
            CHECK(again->inventoryEntries(InventoryPocket::PCItems, err).size() == 50);
        }
    }
    CHECK(raw == source);
    editor->discard();
    CHECK(std::equal(source.begin(), source.end(), editor->stagedBytes().begin()) && !editor->hasPendingChanges());
}
} // namespace gen2

// ----------------------------------------------------------------------------------- Gen III --
namespace gen3 {
using namespace PokeVault::Integration::Gen3;
using LogicalSectors = std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;
void write16(std::span<uint8_t> b, std::size_t o, uint16_t v) { b[o] = static_cast<uint8_t>(v); b[o + 1] = static_cast<uint8_t>(v >> 8); }
void write32(std::span<uint8_t> b, std::size_t o, uint32_t v) { write16(b, o, static_cast<uint16_t>(v)); write16(b, o + 2, static_cast<uint16_t>(v >> 16)); }
struct Put { std::size_t pouchOffset; std::size_t slot; uint16_t id; uint16_t count; };
// Ruby / Sapphire (no item key): Items 0x560 x20, Key 0x5B0, Balls 0x600, TM 0x640, Berries 0x740
// (tests/test_gen3_staged_inventory.cpp layout; pret pokeemerald / PKHeX SAV3RS).
// `frlg`: section 0 game code 1 (FireRed / LeafGreen, security key 0 at 0xF20; tests/test_gen3_staged_inventory.cpp).
enum class Family { RS, Emerald, FRLG };
// Emerald: a nonzero security key at 0xAC and the 0xEE0 marker (tests/test_gen3_staged_inventory.cpp);
// the key masks quantities, so put counts are raw for RS / FRLG (key 0) only.
std::vector<uint8_t> fixture(const std::vector<Put>& puts, Family family) {
    LogicalSectors logical{};
    if (family == Family::Emerald) {
        write32(logical[0], 0xAC, 0xA1B2C3D4);
        logical[0][0xEE0] = 0x42;
    } else {
        write32(logical[0], 0xAC, family == Family::FRLG ? 1 : 0);
    }
    for (const auto& p : puts) {
        write16(logical[1], p.pouchOffset + p.slot * 4, p.id);
        write16(logical[1], p.pouchOffset + p.slot * 4 + 2, p.count);
    }
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    for (uint8_t slot = 0; slot < 2; ++slot) {
        const uint8_t rotation = slot == 0 ? 0 : 5;
        const uint32_t counter = slot == 0 ? 7 : 9;
        for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
            const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
            const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
            std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
            write16(save, offset + 0xFF4, id);
            write16(save, offset + 0xFF6, Detail::sectorChecksum(std::span<const uint8_t>(save).subspan(offset, Detail::kChunkLengths[id])));
            write32(save, offset + 0xFF8, Detail::kSectorSignature);
            write32(save, offset + 0xFFC, counter);
        }
    }
    return save;
}
std::vector<uint8_t> fixture(const std::vector<Put>& puts, bool frlg = false) {
    return fixture(puts, frlg ? Family::FRLG : Family::RS);
}
void run() {
    std::vector<Put> puts{{0x560, 0, 13, 3}, {0x560, 1, 13, 4},   // Potion in two stacks
                          {0x560, 2, 14, 5},                       // Antidote x5
                          {0x5B0, 0, 259, 1},                      // Mach Bike
                          {0x640, 0, 289, 1}, {0x640, 1, 339, 1}};  // TM01, HM01
    std::size_t slot = 3;
    for (const auto id : addable(I::ClassicGame::Ruby, I::ClassicPocket::Items)) {
        if (slot >= 18) break;
        if (id == 13 || id == 14) continue;
        puts.push_back({0x560, slot++, id, 1});
    }
    const auto raw = fixture(puts);
    const auto source = raw;
    std::string err;
    auto editor = StagedInventoryEditor::create(raw, SourceGame::RubyGBA, err);
    CHECK(editor != nullptr);
    if (!editor) return;
    CHECK(editor->freeSlots(InventoryPouch::Items) == 2);
    const auto state = B::Gen3Bag::pockets(*editor, err);
    CHECK(err.empty() && state.size() == 6);
    Model::State s;
    s.scope = Model::Scope::AllBag;
    s.quantity = 999;
    const auto plan = B::plan(B::Gen3Bag::game(*editor), Model::request(s, B::Pocket::Items, state), state);
    planIsSafe(QP::Game::Ruby, plan, state);
    CHECK(skipped(plan, 13, B::SkipReason::SeveralStacks) && !topUp(plan, 13));
    CHECK(topUp(plan, 14) && topUp(plan, 14)->to == 99);   // R/S cap 99 even when 999 is asked
    CHECK(topUp(plan, 289) && topUp(plan, 289)->to == 99);
    // Round 12 owner H12 ("can we add all of the tm/hm's instead of excluding them"): an HM is an
    // ordinary unique TM/HM item now, so a held one is "only one" (never topped up), not excluded.
    CHECK(skipped(plan, 259, B::SkipReason::KeyOrStoryItem) && skipped(plan, 339, B::SkipReason::OnlyOnce));
    for (uint16_t hm = 340; hm <= 346; ++hm) CHECK(added(plan, B::Pocket::TMHM, hm));   // HM02-08, one each
    CHECK(addsIn(plan, B::Pocket::Items) == 2 && count(plan, B::SkipReason::PocketFull, B::Pocket::Items) > 0);
    for (const auto& a : plan.adds) if (a.pocket == B::Pocket::Berries) CHECK(a.quantity == 999);  // Berry Pouch 999

    const auto before = editor->stagedBytes();
    CHECK(!B::apply(*editor, plan, refusing(B::Gen3Bag::Adapter{}, plan)).applied);
    CHECK(editor->stagedBytes() == before && !editor->hasPendingChanges());
    CHECK(B::apply(*editor, plan, B::Gen3Bag::Adapter{}).applied);
    readsBack(*editor, plan, B::Gen3Bag::Adapter{});
    CHECK(editor->stagedInventoryQuantity(InventoryPouch::KeyItems, 259) == 1);
    CHECK(editor->stagedInventoryQuantity(InventoryPouch::TMCase, 339) == 1);
    std::string listErr;
    std::size_t potionStacks = 0;
    for (const auto& e : editor->inventoryEntries(InventoryPouch::Items, listErr)) potionStacks += e.itemId == 13;
    CHECK(potionStacks == 2);   // both stacks kept exactly
    const auto out = editor->finalizedBytes(err);
    CHECK(!out.empty());
    auto again = StagedInventoryEditor::create(out, SourceGame::RubyGBA, err);
    CHECK(again != nullptr);
    if (again) readsBack(*again, plan, B::Gen3Bag::Adapter{});
    CHECK(raw == source && editor->originalBytes() == source);
    editor->discard();
    CHECK(editor->stagedBytes() == source && !editor->hasPendingChanges());
}

// Round 11 (H34): Gen III PC Items. Ruby / Sapphire: 50 slots at 0x498, cap 999, Key Items the R/S
// PC accepts (Mach Bike) and HMs are reported, TMs may be stored. FireRed / LeafGreen: 30 slots at
// 0x298, General items only. Bag + PC Items fills each list within its own native slots.
void pcOne(SourceGame game, std::size_t pcOffset, std::size_t pcSlots, std::size_t itemsOffset, QP::Game policy) {
    const auto classic = game == SourceGame::FireRedGBA ? I::ClassicGame::FireRed : I::ClassicGame::Ruby;
    std::vector<Put> puts{{pcOffset, 0, 13, 7}, {pcOffset, 1, 14, 998}};   // Potion x7, Antidote x998
    std::size_t slot = 2;
    if (game == SourceGame::RubyGBA) { puts.push_back({pcOffset, slot++, 259, 1}); puts.push_back({pcOffset, slot++, 339, 1}); }
    for (const auto id : addable(classic, I::ClassicPocket::Items)) {
        if (slot + 4 >= pcSlots) break;
        if (id != 13 && id != 14) puts.push_back({pcOffset, slot++, id, 1});
    }
    puts.push_back({itemsOffset, 0, 13, 2});   // the Bag holds a Potion too
    const auto raw = fixture(puts, game == SourceGame::FireRedGBA);
    const auto source = raw;
    std::string err;
    auto editor = StagedInventoryEditor::create(raw, game, err);
    CHECK(editor != nullptr);
    if (!editor) { std::cerr << "  " << err << '\n'; return; }
    CHECK(editor->freeSlots(InventoryPouch::PCItems) == 4);
    const auto state = B::Gen3Bag::pockets(*editor, err);
    const auto pcState = std::find_if(state.begin(), state.end(), [](const B::PocketState& p) { return p.pocket == B::Pocket::PCItems; });
    CHECK(pcState != state.end() && pcState->slotCapacity == pcSlots);
    if (pcState == state.end()) return;
    if (game == SourceGame::FireRedGBA) {   // FR/LG PC: General items only (PKHeX ItemStorage3FRLG)
        for (const auto id : pcState->addableItems) {
            const auto general = addable(classic, I::ClassicPocket::Items);
            CHECK(std::find(general.begin(), general.end(), id) != general.end());
        }
    }
    const auto plan = B::plan(policy, Model::request(scoped(Model::Scope::PcItems, 999), B::Pocket::Items, state), state);
    planIsSafe(policy, plan, state, true);
    noStoryChange(policy, plan);
    for (const auto& a : plan.adds) CHECK(a.pocket == B::Pocket::PCItems && a.quantity == 999);   // PC cap 999
    CHECK(addsIn(plan, B::Pocket::PCItems) == 4 && count(plan, B::SkipReason::PocketFull, B::Pocket::PCItems) > 0);
    CHECK(topUp(plan, 13) && topUp(plan, 13)->pocket == B::Pocket::PCItems && topUp(plan, 13)->to == 999);
    CHECK(topUp(plan, 14) && topUp(plan, 14)->from == 998 && topUp(plan, 14)->to == 999);
    if (game == SourceGame::RubyGBA)
        // Round 12 owner H12: a held HM is "only one" now, not excluded.
        CHECK(skipped(plan, 259, B::SkipReason::KeyOrStoryItem) && skipped(plan, 339, B::SkipReason::OnlyOnce));
    CHECK(plan.full.size() == 1 && plan.full[0].slots == pcSlots && plan.full[0].freeBefore == 4);
    CHECK(Model::previewLines(plan, {B::Pocket::PCItems}, [](B::Pocket, uint16_t id) { return std::to_string(id); },
                              [](B::Pocket) { return std::string("PC Items"); })[0].text ==
          "PC Items (" + std::to_string(pcSlots) + " slots): 4 free, " +
              std::to_string(count(plan, B::SkipReason::PocketFull, B::Pocket::PCItems)) + " more did not fit");

    CHECK(B::apply(*editor, plan, B::Gen3Bag::Adapter{}).applied);
    readsBack(*editor, plan, B::Gen3Bag::Adapter{});
    CHECK(editor->stagedInventoryQuantity(InventoryPouch::Items, 13) == 2);   // the Bag is not the PC
    const auto out = editor->finalizedBytes(err);
    CHECK(!out.empty());
    auto again = StagedInventoryEditor::create(out, game, err);
    CHECK(again != nullptr);
    if (again) readsBack(*again, plan, B::Gen3Bag::Adapter{});
    editor->discard();
    CHECK(editor->stagedBytes() == source && !editor->hasPendingChanges());

    // Bag + PC Items: the Bag's Items pouch and the PC each within their own slots; Potion is topped
    // up in both lists (an ordinary item may natively sit in both).
    const auto both = B::plan(policy, Model::request(scoped(Model::Scope::BagAndPc, 999), B::Pocket::Items, state), state);
    planIsSafe(policy, both, state, true);
    noStoryChange(policy, both);
    CHECK(addsIn(both, B::Pocket::PCItems) == 4 && addsIn(both, B::Pocket::Items) > 0);
    CHECK(topUpsIn(both, B::Pocket::Items) == 1 && topUpsIn(both, B::Pocket::PCItems) >= 2);
    CHECK(B::apply(*editor, both, B::Gen3Bag::Adapter{}).applied);
    readsBack(*editor, both, B::Gen3Bag::Adapter{});
    const auto out2 = editor->finalizedBytes(err);
    auto again2 = out2.empty() ? nullptr : StagedInventoryEditor::create(out2, game, err);
    CHECK(again2 != nullptr);
    if (again2) readsBack(*again2, both, B::Gen3Bag::Adapter{});
    CHECK(raw == source && editor->originalBytes() == source);
    editor->discard();
    CHECK(editor->stagedBytes() == source && !editor->hasPendingChanges());
}
void pc() {
    pcOne(SourceGame::RubyGBA, 0x498, 50, 0x560, QP::Game::Ruby);
    pcOne(SourceGame::FireRedGBA, 0x298, 30, 0x310, QP::Game::FireRed);
}
} // namespace gen3

// ------------------------------------------------------------------------------------ Gen IV --
namespace gen4 {
namespace G4 = PokeVault::Integration::Gen4;
void run() {
    auto raw = qasynth::gen4::save(qasynth::gen4::Kind::Platinum);
    const auto layout = G4::bagLayout(G4::Layout::Platinum);
    const auto balls = G4::gen4BagChoices(G4::Layout::Platinum, 6);
    CHECK(balls.size() > 4);
    const auto put = [&](std::size_t pocket, std::size_t slot, uint16_t id, uint16_t count) {
        for (std::size_t base : {std::size_t{0}, qasynth::gen4::kPartition}) {
            qasynth::w16(raw, base + layout[pocket].offset + slot * 4, id);
            qasynth::w16(raw, base + layout[pocket].offset + slot * 4 + 2, count);
        }
    };
    put(4, 0, 17, 3); put(4, 1, 17, 7);          // Medicine: Potion in two stacks
    put(1, 0, 450, 1);                           // Bicycle
    put(2, 0, 328, 1); put(2, 1, 420, 1);        // TM01, HM01
    // Balls: three kinds plus Poke Ball copies, one slot left of fifteen.
    for (std::size_t i = 0; i + 1 < layout[6].slots; ++i) put(6, i, i < 3 ? balls[i] : balls[0], 1);
    qasynth::gen4::forge(raw);
    const auto source = raw;
    std::string err;
    auto editor = G4::Gen4StagedPokemonEditor::create(raw, G4::Layout::Platinum, "platinum_nds", &err);
    CHECK(editor.has_value());
    if (!editor) return;
    const auto state = B::Gen4Bag::pockets(*editor, err);
    CHECK(err.empty() && state.size() == G4::BagPocketCount);
    for (std::size_t p = 0; p < state.size(); ++p) CHECK(state[p].addableItems == G4::gen4BagChoices(G4::Layout::Platinum, p));
    CHECK(state[6].slotCapacity == layout[6].slots && state[6].held.size() == layout[6].slots - 1);
    Model::State s;
    s.scope = Model::Scope::AllBag;
    s.quantity = 999;
    const auto game = B::Gen4Bag::game(*editor);
    CHECK(game && *game == QP::Game::Platinum);
    if (!game) return;
    CHECK(!B::hasPcItems(state));   // Gen IV: no PC item storage, so no PC scope is offered
    const auto plan = B::plan(*game, Model::request(s, B::Pocket::Medicine, state), state);
    planIsSafe(QP::Game::Platinum, plan, state);
    CHECK(topUp(plan, 17, 0) && topUp(plan, 17, 0)->to == 999 && topUp(plan, 17, 1) && topUp(plan, 17, 1)->from == 7);
    CHECK(topUp(plan, 328) && topUp(plan, 328)->to == 99);   // Gen IV TMs: native 99
    // Round 12 owner H12 ("can we add all of the tm/hm's instead of excluding them"): an HM is an
    // ordinary unique TM/HM item now, so a held one is "only one" (never topped up), not excluded.
    CHECK(skipped(plan, 450, B::SkipReason::KeyOrStoryItem) && skipped(plan, 420, B::SkipReason::OnlyOnce));
    for (uint16_t hm = 421; hm <= 427; ++hm) CHECK(added(plan, B::Pocket::TMHM, hm));   // HM02-08, one each
    CHECK(addsIn(plan, B::Pocket::Balls) == 1 && count(plan, B::SkipReason::PocketFull, B::Pocket::Balls) == balls.size() - 4);
    for (const auto& t : plan.topUps) if (t.pocket == B::Pocket::Balls) CHECK(t.to == 999);   // every Poke Ball stack

    const auto before = editor->stagedBytes();
    CHECK(!B::apply(*editor, plan, refusing(B::Gen4Bag::Adapter{}, plan)).applied);
    CHECK(editor->stagedBytes() == before && !editor->hasChanges());
    const auto result = B::apply(*editor, plan, B::Gen4Bag::Adapter{});
    CHECK(result.applied);
    readsBack(*editor, plan, B::Gen4Bag::Adapter{});
    CHECK(B::Gen4Bag::Adapter{}.quantity(*editor, B::Pocket::KeyItems, 450, SIZE_MAX) == 1);
    CHECK(B::Gen4Bag::Adapter{}.quantity(*editor, B::Pocket::TMHM, 420, SIZE_MAX) == 1);
    const auto out = editor->finalizedBytes(&err);
    CHECK(!out.empty());
    auto reopened = G4::Gen4StagedPokemonEditor::create(out, G4::Layout::Platinum, "platinum_nds", &err);
    CHECK(reopened.has_value());
    if (reopened) readsBack(*reopened, plan, B::Gen4Bag::Adapter{});
    CHECK(raw == source && editor->originalBytes() == source);
    editor->discard();
    CHECK(editor->stagedBytes() == source && !editor->hasChanges());
}
} // namespace gen4

// ------------------------------------------------------------------------------------- Gen V --
namespace gen5 {
namespace G5 = PokeVault::Integration::Gen5;
void run() {
    using Family = G5::SaveFamily;
    auto raw = qasynth::gen5::save(Family::BlackWhite, 21);   // Black, a few native stacks per pocket
    const auto medicine = G5::gen5BagChoices(Family::BlackWhite, 3);
    const auto spec = G5::bagPocketLayout5(Family::BlackWhite, 3);
    CHECK(medicine.size() > 4);
    const std::size_t items = G5::itemsBlockOffset(Family::BlackWhite);
    // Medicine: slot 0 already holds kMedicine[0] x12; add a second stack of it, two more kinds, then
    // copies until one slot is left.
    for (std::size_t slot = 1; slot + 1 < spec.slots; ++slot) {
        const uint16_t id = slot <= 2 ? medicine[slot] : G5::BagCatalog5::kMedicine[0];
        qasynth::w16(raw, items + spec.offset + slot * 4, id);
        qasynth::w16(raw, items + spec.offset + slot * 4 + 2, static_cast<uint16_t>(slot == 3 ? 40 : 1));
    }
    qasynth::gen5::forge(raw);
    const auto source = raw;
    std::string err;
    const auto parsed = G5::Gen5ReadOnlySave::parse(raw, "black_nds", &err, G5::SaveCopySelection::Primary);
    CHECK(parsed.has_value());
    if (!parsed) return;
    G5::Gen5StagedPokemonWorkspace ws(*parsed);
    auto* bag = ws.stagedBag(&err);
    CHECK(bag != nullptr);
    if (!bag) return;
    const auto state = B::Gen5Bag::pockets(*bag);
    for (std::size_t p = 0; p < state.size(); ++p) CHECK(state[p].addableItems == G5::gen5BagChoices(Family::BlackWhite, p));
    CHECK(state[3].slotCapacity == spec.slots);
    const auto game = B::Gen5Bag::game("black_nds");
    CHECK(game && *game == QP::Game::Black);
    if (!game) return;
    Model::State s;
    s.scope = Model::Scope::AllBag;
    s.quantity = 999;
    CHECK(!B::hasPcItems(state));   // Gen V: no PC item storage, so no PC scope is offered
    const auto plan = B::plan(*game, Model::request(s, B::Pocket::Medicine, state), state);
    planIsSafe(QP::Game::Black, plan, state);
    CHECK(topUp(plan, G5::BagCatalog5::kMedicine[0], 0) && topUp(plan, G5::BagCatalog5::kMedicine[0], 3) &&
          topUp(plan, G5::BagCatalog5::kMedicine[0], 3)->from == 40);   // each stack on its own
    CHECK(skipped(plan, G5::BagCatalog5::kKeyBW[0], B::SkipReason::KeyOrStoryItem));
    CHECK(skipped(plan, G5::BagCatalog5::kMachine[0], B::SkipReason::OnlyOnce));     // Gen V TMs are unique
    CHECK(addsIn(plan, B::Pocket::Medicine) == 1 && count(plan, B::SkipReason::PocketFull, B::Pocket::Medicine) > 0);
    for (const auto& a : plan.adds) if (a.pocket == B::Pocket::TMHM) CHECK(a.quantity == 1);

    const auto before = std::vector<uint8_t>(bag->staged().begin(), bag->staged().end());
    CHECK(!B::apply(*bag, plan, refusing(B::Gen5Bag::Adapter{}, plan)).applied);
    CHECK(std::equal(before.begin(), before.end(), bag->staged().begin()) && !bag->dirty());
    CHECK(B::apply(*bag, plan, B::Gen5Bag::Adapter{}).applied);
    readsBack(*bag, plan, B::Gen5Bag::Adapter{});
    CHECK(!ws.bagReviewChanges().empty());   // Review lists the staged Bag changes
    const auto candidate = G5::buildStagedSaveCandidate(ws, &err);
    CHECK(candidate.has_value() && candidate && candidate->bagChanged);
    if (candidate) {
        const auto reparsed = G5::Gen5ReadOnlySave::parse(candidate->bytes, "black_nds", &err, G5::SaveCopySelection::Primary);
        CHECK(reparsed.has_value());
        if (reparsed) {
            auto check = G5::Gen5StagedBag::create(*reparsed, &err);
            CHECK(check.has_value());
            if (check) readsBack(*check, plan, B::Gen5Bag::Adapter{});
            const auto key = G5::decodeBag5(*reparsed);
            CHECK(key && (*key)[1].size() == 1 && (*key)[1][0].itemId == G5::BagCatalog5::kKeyBW[0]);
        }
    }
    CHECK(raw == source);
    ws.discardAll();
    CHECK(!ws.hasChanges() && ws.bagReviewChanges().empty());
    const auto restored = ws.bagPresentation();
    const auto original = G5::decodeBag5(*parsed);
    CHECK(restored && original && (*restored)[3].size() == (*original)[3].size());
}
} // namespace gen5

// ------------------------------------------------------------------- Round 12 R12-006 / R12-007 --
// Owner H14 (Round 12): "in the top up feature doesnt work correctly it only selects like 20 items or
// something we need the all items in that slot not just 20, check this for every gen". Owner H12:
// "can we add all of the tm/hm's instead of excluding them".
// Every generation, every scope, through the REAL adapters on synthetic saves: each list's slot count
// is the native one; the plan adds exactly min(eligible missing items, free native slots) per native
// list; every candidate the Add picker offers that is not held is either planned or listed with a
// reason (nothing silently dropped); no list ever exceeds its native slots; and every TM and HM the
// game's own TM/HM list defines is planned (Key Items stay excluded, with their reason).
namespace coverage {
using G1Game = PokeVault::Integration::Gen1::SourceGame;
// No Key Item is ever planned (they gate story events); TMs and HMs may be.
void noKeyItem(QP::Game game, const B::Plan& plan) {
    for (const auto& a : plan.adds) CHECK(QP::quantityRule(game, a.pocket, a.itemId).kind != QP::ItemKind::KeyItem);
    for (const auto& t : plan.topUps) CHECK(QP::quantityRule(game, t.pocket, t.itemId).kind != QP::ItemKind::KeyItem);
}
struct Native { B::Pocket pocket; std::size_t slots; };
const char* scopeName(Model::Scope s) {
    switch (s) {
        case Model::Scope::ThisPocket: return "This pocket";
        case Model::Scope::AllBag: return "All Bag";
        case Model::Scope::PcItems: return "PC Items";
        case Model::Scope::BagAndPc: return "Bag + PC";
    }
    return "?";
}
bool verbose() { const char* v = std::getenv("POKEBANK_TOPUP_VERBOSE"); return v && *v; }

// One plan against the exact-count contract. Returns the number of adds in the TM/HM pocket.
void exact(const std::string& label, const B::Request& req, const B::Plan& plan,
           std::span<const B::PocketState> state, std::span<const Native> native) {
    const auto requested = [&](B::Pocket p) { return std::find(req.pockets.begin(), req.pockets.end(), p) != req.pockets.end(); };
    for (const auto& n : native) {
        const auto p = std::find_if(state.begin(), state.end(), [&](const B::PocketState& s) { return s.pocket == n.pocket; });
        CHECK(p != state.end() && p->slotCapacity == n.slots);
        if (p != state.end() && p->slotCapacity != n.slots)
            std::fprintf(stderr, "  %s: pocket %d has %zu slots, native %zu\n", label.c_str(), static_cast<int>(n.pocket), p->slotCapacity, n.slots);
    }
    for (const auto& a : plan.adds) CHECK(requested(a.pocket));
    for (const auto& t : plan.topUps) CHECK(requested(t.pocket));
    for (const auto& k : plan.skipped) CHECK(requested(k.pocket) && !B::skipText(k).empty());
    // Per native list (a slotGroup is ONE list): eligible = planned adds + "pocket full" skips.
    std::map<int, std::pair<std::size_t, std::size_t>> groupAddsEligible;
    for (const auto& p : state) {
        if (!requested(p.pocket)) continue;
        std::vector<uint16_t> missing;
        for (const uint16_t id : p.addableItems)
            if (id != 0 && std::none_of(p.held.begin(), p.held.end(), [id](const B::HeldStack& h) { return h.itemId == id; }))
                missing.push_back(id);
        std::sort(missing.begin(), missing.end());
        missing.erase(std::unique(missing.begin(), missing.end()), missing.end());
        std::size_t adds = 0, fullSkips = 0;
        if (req.mode != B::Mode::TopUpExisting) {
            for (const uint16_t id : missing) {
                const auto a = std::count_if(plan.adds.begin(), plan.adds.end(), [&](const B::Add& x) { return x.pocket == p.pocket && x.itemId == id; });
                const auto k = std::count_if(plan.skipped.begin(), plan.skipped.end(), [&](const B::Skip& x) { return x.pocket == p.pocket && x.itemId == id; });
                CHECK(a + k == 1);   // planned or listed with its reason; never silently dropped
                if (a + k != 1) std::fprintf(stderr, "  %s: item %u in pocket %d planned %ld, listed %ld\n", label.c_str(), id, static_cast<int>(p.pocket), static_cast<long>(a), static_cast<long>(k));
                adds += static_cast<std::size_t>(a);
                fullSkips += static_cast<std::size_t>(std::count_if(plan.skipped.begin(), plan.skipped.end(), [&](const B::Skip& x) {
                    return x.pocket == p.pocket && x.itemId == id && x.reason == B::SkipReason::PocketFull; }));
            }
        }
        if (req.mode != B::Mode::AddMissing)
            for (const auto& h : p.held) {
                const bool planned = std::any_of(plan.topUps.begin(), plan.topUps.end(), [&](const B::TopUp& t) { return t.pocket == p.pocket && t.slot == h.slot; });
                const bool listed = std::any_of(plan.skipped.begin(), plan.skipped.end(), [&](const B::Skip& k) { return k.pocket == p.pocket && k.itemId == h.itemId; });
                CHECK(planned || listed);
            }
        const std::size_t eligible = adds + fullSkips;
        if (p.slotGroup >= 0) {
            groupAddsEligible[p.slotGroup].first += adds;
            groupAddsEligible[p.slotGroup].second += eligible;
        } else {
            const std::size_t free = p.slotCapacity - p.held.size();
            CHECK(adds == std::min(eligible, free));
            CHECK(p.held.size() + adds <= p.slotCapacity);
            if (req.mode != B::Mode::TopUpExisting) {
                // The preview's exact count for this list (Plan::rooms) is the same arithmetic.
                const auto room = std::find_if(plan.rooms.begin(), plan.rooms.end(), [&](const B::ListRoom& r) { return r.pocket == p.pocket; });
                CHECK(room != plan.rooms.end());
                if (room != plan.rooms.end())
                    CHECK(room->slots == p.slotCapacity && room->freeBefore == free && room->eligible == eligible &&
                          room->planned == adds && room->notFitted == eligible - adds && !room->sharedList);
            }
            if (adds != std::min(eligible, free))
                std::fprintf(stderr, "  %s: pocket %d planned %zu, eligible %zu, free %zu\n", label.c_str(), static_cast<int>(p.pocket), adds, eligible, free);
        }
        if (verbose())
            std::fprintf(stderr, "  %-28s pocket %d: slots %zu held %zu offered %zu missing %zu eligible %zu planned %zu\n",
                         label.c_str(), static_cast<int>(p.pocket), p.slotCapacity, p.held.size(), p.addableItems.size(),
                         missing.size(), eligible, adds);
    }
    for (const auto& [group, ae] : groupAddsEligible) {
        std::size_t held = 0, cap = 0;
        for (const auto& p : state) if (p.slotGroup == group) { held += p.held.size(); cap = p.slotCapacity; }
        CHECK(ae.first == std::min(ae.second, cap - held));
        CHECK(held + ae.first <= cap);
        // One room for the whole shared list (Gen I Bag), whichever categories were planned.
        CHECK(std::count_if(plan.rooms.begin(), plan.rooms.end(), [](const B::ListRoom& r) { return r.sharedList; }) == 1);
        for (const auto& r : plan.rooms)
            if (r.sharedList) CHECK(r.slots == cap && r.freeBefore == cap - held && r.planned == ae.first && r.eligible == ae.second);
    }
    // The preview opens with one capacity line per native list (full or not), before any item line.
    if (req.mode != B::Mode::TopUpExisting) {
        const auto lines = Model::previewLines(plan, req.pockets, [](B::Pocket, uint16_t id) { return std::to_string(id); },
                                               [](B::Pocket p) { return "P" + std::to_string(static_cast<int>(p)); });
        CHECK(lines.size() >= plan.rooms.size());
        for (std::size_t i = 0; i < plan.rooms.size() && i < lines.size(); ++i)
            CHECK(lines[i].kind == Model::LineKind::Note && lines[i].text.find(" slots") != std::string::npos);
    }
}

// Every TM and HM of the game's own TM/HM list that is not held is planned (R12-007): no
// "Key Item or HM (excluded)" skip in the TM/HM pocket. `fits` = the list has room for all of them.
void allMachines(const std::string& label, const B::Plan& plan, std::span<const B::PocketState> state, bool fits) {
    const auto tm = std::find_if(state.begin(), state.end(), [](const B::PocketState& p) { return p.pocket == B::Pocket::TMHM; });
    CHECK(tm != state.end());
    if (tm == state.end()) return;
    for (const auto& k : plan.skipped)
        if (k.pocket == B::Pocket::TMHM) CHECK(k.reason != B::SkipReason::KeyOrStoryItem && k.reason != B::SkipReason::NotInPocket);
    std::size_t missing = 0;
    for (const uint16_t id : tm->addableItems) {
        if (std::any_of(tm->held.begin(), tm->held.end(), [id](const B::HeldStack& h) { return h.itemId == id; })) continue;
        ++missing;
        if (fits) {
            CHECK(added(plan, B::Pocket::TMHM, id));
            if (!added(plan, B::Pocket::TMHM, id)) std::fprintf(stderr, "  %s: machine %u not planned\n", label.c_str(), id);
        }
    }
    if (fits) CHECK(addsIn(plan, B::Pocket::TMHM) == missing);
}

template <class Run>
void everyScope(const std::string& game, std::span<const B::PocketState> state, B::Pocket current, Run&& run) {
    std::vector<Model::Scope> scopes{Model::Scope::ThisPocket, Model::Scope::AllBag};
    if (B::hasPcItems(state)) { scopes.push_back(Model::Scope::PcItems); scopes.push_back(Model::Scope::BagAndPc); }
    for (const auto scope : scopes)
        for (const auto mode : {B::Mode::AddMissing, B::Mode::Both}) {
            const auto req = Model::request(scoped(scope, 999, mode), current, state);
            run(game + " " + scopeName(scope) + (mode == B::Mode::Both ? " +top" : ""), req);
        }
}

void g1() {
    for (const auto& g : {std::pair{G1Game::Red, QP::Game::Red}, std::pair{G1Game::Yellow, QP::Game::Yellow}}) {
        auto raw = gen1::fixture({});
        if (g.first == G1Game::Yellow) {   // Yellow evidence: starter 0x54, Pikachu friendship (tests/test_bag_add_flow_gen15.cpp)
            raw[0x29C3] = 0x54;
            raw[0x271C] = 90;
            qasynth::gen1::forge(raw);
        }
        auto parsed = PokeVault::Integration::Gen1::parse(raw, g.first);
        CHECK(static_cast<bool>(parsed));
        if (!parsed) continue;
        std::string err;
        auto editor = PokeVault::Integration::Gen1::StagedInventoryEditor::create(*parsed.save, err);
        CHECK(editor != nullptr);
        if (!editor) continue;
        const auto state = B::Gen1Bag::pockets(*editor, err);
        // pret pokered: one 20-slot Bag (every category a view of it), a 50-slot PC.
        const std::vector<Native> native{{B::Pocket::Items, 20}, {B::Pocket::Medicine, 20}, {B::Pocket::Balls, 20},
                                         {B::Pocket::KeyItems, 20}, {B::Pocket::TMHM, 20}, {B::Pocket::PCItems, 50}};
        for (const auto current : {B::Pocket::Items, B::Pocket::TMHM, B::Pocket::PCItems})
            everyScope(std::string("Gen I ") + (g.first == G1Game::Red ? "Red" : "Yellow"), state, current,
                       [&](const std::string& label, const B::Request& req) {
                const auto plan = B::plan(g.second, req, state);
                exact(label, req, plan, state, native);
                noKeyItem(g.second, plan);
                if (req.pockets == std::vector<B::Pocket>{B::Pocket::TMHM}) {
                    // 55 machines, 20 shared slots: 20 planned (HM01-05 first), 35 "Bag full", no exclusion.
                    allMachines(label, plan, state, false);
                    CHECK(addsIn(plan, B::Pocket::TMHM) == 20 && plan.full.size() == 1 && plan.full[0].sharedList);
                    for (const uint16_t hm : {196, 197, 198, 199, 200}) CHECK(added(plan, B::Pocket::TMHM, hm));
                }
            });
        // TM/HM alone, applied: HM01-05 and TM01-15 staged, exported, strictly reparsed.
        const auto req = Model::request(scoped(Model::Scope::ThisPocket, 99, B::Mode::AddMissing), B::Pocket::TMHM, state);
        const auto plan = B::plan(g.second, req, state);
        CHECK(B::apply(*editor, plan, B::Gen1Bag::Adapter(*editor)).applied);
        readsBack(*editor, plan, B::Gen1Bag::Adapter(*editor));
        const auto out = editor->finalize(err);
        auto reparsed = out.empty() ? decltype(parsed){} : PokeVault::Integration::Gen1::parse(out, g.first);
        CHECK(static_cast<bool>(reparsed));
        if (reparsed) {
            auto check = PokeVault::Integration::Gen1::StagedInventoryEditor::create(*reparsed.save, err);
            CHECK(check != nullptr);
            if (check) {
                readsBack(*check, plan, B::Gen1Bag::Adapter(*check));
                CHECK(check->freeSlots(I::ClassicPocket::TMHM, err) == 0);
            }
        }
        CHECK(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()));
        CHECK(Model::appliedStatus(0, plan.adds.size(), plan).rfind("Partly staged: 0 topped up, 20 added; 35 did not fit", 0) == 0);
    }
}

void g2() {
    using namespace PokeVault::Integration::Gen2;
    const auto raw = gen2::fixture(gen2::C, {});
    auto parsed = parse(raw, SourceGame::Crystal);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    std::string err;
    auto editor = StagedEditor::create(*parsed.save, err);
    CHECK(editor != nullptr);
    if (!editor) return;
    const auto state = B::Gen2Bag::pockets(*editor, err);
    // pret pokecrystal: Items 20, Balls 12, Key Items 25, TM/HM one fixed slot per machine (57), PC 50.
    const std::vector<Native> native{{B::Pocket::Items, 20}, {B::Pocket::Balls, 12}, {B::Pocket::KeyItems, 25},
                                     {B::Pocket::TMHM, 57}, {B::Pocket::PCItems, 50}};
    for (const auto current : {B::Pocket::Items, B::Pocket::TMHM, B::Pocket::PCItems})
        everyScope("Gen II Crystal", state, current, [&](const std::string& label, const B::Request& req) {
            const auto plan = B::plan(QP::Game::Crystal, req, state);
            exact(label, req, plan, state, native);
            noKeyItem(QP::Game::Crystal, plan);
            if (std::find(req.pockets.begin(), req.pockets.end(), B::Pocket::TMHM) != req.pockets.end())
                allMachines(label, plan, state, true);
        });
    // All Bag pockets, applied: every TM and HM (HM02-07 added; TM01 / HM01 already held), the
    // finalized save reparses strictly and reads back.
    const auto req = Model::request(scoped(Model::Scope::AllBag, 99, B::Mode::Both), B::Pocket::TMHM, state);
    const auto plan = B::plan(QP::Game::Crystal, req, state);
    CHECK(addsIn(plan, B::Pocket::TMHM) == 55);
    for (uint8_t hm = 244; hm <= 249; ++hm) CHECK(added(plan, B::Pocket::TMHM, hm));
    CHECK(skipped(plan, 243, B::SkipReason::OnlyOnce));   // HM01 held: one only, nothing to top up
    CHECK(skipped(plan, 7, B::SkipReason::KeyOrStoryItem));   // the Bicycle stays a Key Item
    CHECK(B::apply(*editor, plan, B::Gen2Bag::Adapter{}).applied);
    readsBack(*editor, plan, B::Gen2Bag::Adapter{});
    const auto out = editor->finalizedBytes(err);
    auto reopened = out.empty() ? ParseResult{} : parse(out, SourceGame::Crystal);
    CHECK(static_cast<bool>(reopened));
    if (reopened) {
        auto again = StagedEditor::create(*reopened.save, err);
        CHECK(again != nullptr);
        if (again) {
            readsBack(*again, plan, B::Gen2Bag::Adapter{});
            for (uint8_t hm = 243; hm <= 249; ++hm) CHECK(again->stagedInventoryQuantity(InventoryPocket::TMHM, hm) == 1);
        }
    }
    CHECK(raw == std::vector<uint8_t>(editor->originalBytes().begin(), editor->originalBytes().end()));
}

void g3() {
    using namespace PokeVault::Integration::Gen3;
    struct G { SourceGame source; QP::Game policy; const char* name; gen3::Family family; std::vector<Native> native; };
    // Owner brief (pret pokeruby / pokeemerald / pokefirered): Items RS 20, E 30, FRLG 42; Balls 16/16/13;
    // TM/HM 64/64/58; Berries 46/46/43; Key Items 20/30/30; PC 50/50/30.
    const std::vector<G> games{
        {SourceGame::RubyGBA, QP::Game::Ruby, "Ruby", gen3::Family::RS,
         {{B::Pocket::Items, 20}, {B::Pocket::Balls, 16}, {B::Pocket::TMHM, 64}, {B::Pocket::Berries, 46}, {B::Pocket::KeyItems, 20}, {B::Pocket::PCItems, 50}}},
        {SourceGame::EmeraldGBA, QP::Game::Emerald, "Emerald", gen3::Family::Emerald,
         {{B::Pocket::Items, 30}, {B::Pocket::Balls, 16}, {B::Pocket::TMHM, 64}, {B::Pocket::Berries, 46}, {B::Pocket::KeyItems, 30}, {B::Pocket::PCItems, 50}}},
        {SourceGame::FireRedGBA, QP::Game::FireRed, "FireRed", gen3::Family::FRLG,
         {{B::Pocket::Items, 42}, {B::Pocket::Balls, 13}, {B::Pocket::TMHM, 58}, {B::Pocket::Berries, 43}, {B::Pocket::KeyItems, 30}, {B::Pocket::PCItems, 30}}},
        {SourceGame::LeafGreenGBA, QP::Game::LeafGreen, "LeafGreen", gen3::Family::FRLG,
         {{B::Pocket::Items, 42}, {B::Pocket::Balls, 13}, {B::Pocket::TMHM, 58}, {B::Pocket::Berries, 43}, {B::Pocket::KeyItems, 30}, {B::Pocket::PCItems, 30}}},
    };
    for (const auto& g : games) {
        const auto raw = gen3::fixture({}, g.family);
        std::string err;
        auto editor = StagedInventoryEditor::create(raw, g.source, err);
        CHECK(editor != nullptr);
        if (!editor) { std::fprintf(stderr, "  Gen III %s: %s\n", g.name, err.c_str()); continue; }
        const auto state = B::Gen3Bag::pockets(*editor, err);
        CHECK(err.empty() && state.size() == 6);
        for (const auto current : {B::Pocket::Items, B::Pocket::TMHM, B::Pocket::Berries, B::Pocket::PCItems})
            everyScope(std::string("Gen III ") + g.name, state, current, [&](const std::string& label, const B::Request& req) {
                const auto plan = B::plan(g.policy, req, state);
                exact(label, req, plan, state, g.native);
                noKeyItem(g.policy, plan);
                if (std::find(req.pockets.begin(), req.pockets.end(), B::Pocket::TMHM) != req.pockets.end())
                    allMachines(label, plan, state, true);
            });
        // The Items pouch: every native slot is filled (RS 20, E 30, FRLG 42), never a fixed 20.
        const auto items = Model::request(scoped(Model::Scope::ThisPocket, 99, B::Mode::AddMissing), B::Pocket::Items, state);
        const auto itemsPlan = B::plan(g.policy, items, state);
        CHECK(addsIn(itemsPlan, B::Pocket::Items) == g.native[0].slots);
        // Bag + PC, applied: every TM / HM, every Berry, a full Items pouch and PC, strict reparse.
        const auto req = Model::request(scoped(Model::Scope::BagAndPc, 99, B::Mode::Both), B::Pocket::Items, state);
        const auto plan = B::plan(g.policy, req, state);
        CHECK(addsIn(plan, B::Pocket::TMHM) == 58);
        for (uint16_t hm = 339; hm <= 346; ++hm) CHECK(added(plan, B::Pocket::TMHM, hm));
        CHECK(addsIn(plan, B::Pocket::Berries) == 43 && addsIn(plan, B::Pocket::Items) == g.native[0].slots);
        CHECK(addsIn(plan, B::Pocket::KeyItems) == 0);
        CHECK(B::apply(*editor, plan, B::Gen3Bag::Adapter{}).applied);
        readsBack(*editor, plan, B::Gen3Bag::Adapter{});
        CHECK(editor->freeSlots(InventoryPouch::Items) == 0);
        const auto out = editor->finalizedBytes(err);
        CHECK(!out.empty());
        auto again = out.empty() ? nullptr : StagedInventoryEditor::create(out, g.source, err);
        CHECK(again != nullptr);
        if (again) {
            readsBack(*again, plan, B::Gen3Bag::Adapter{});
            for (uint16_t hm = 339; hm <= 346; ++hm) CHECK(again->stagedInventoryQuantity(InventoryPouch::TMCase, hm) == 1);
        }
        CHECK(editor->originalBytes() == raw);
    }
}

void g4() {
    namespace G4 = PokeVault::Integration::Gen4;
    struct G { qasynth::gen4::Kind kind; G4::Layout layout; const char* id; QP::Game policy; };
    for (const auto& g : {G{qasynth::gen4::Kind::DiamondPearl, G4::Layout::DiamondPearl, "diamond_nds", QP::Game::Diamond},
                          G{qasynth::gen4::Kind::Platinum, G4::Layout::Platinum, "platinum_nds", QP::Game::Platinum},
                          G{qasynth::gen4::Kind::HeartGoldSoulSilver, G4::Layout::HeartGoldSoulSilver, "heartgold_nds", QP::Game::HeartGold}}) {
        auto raw = qasynth::gen4::save(g.kind);
        qasynth::gen4::forge(raw);
        std::string err;
        auto editor = G4::Gen4StagedPokemonEditor::create(raw, g.layout, g.id, &err);
        CHECK(editor.has_value());
        if (!editor) { std::fprintf(stderr, "  Gen IV %s: %s\n", g.id, err.c_str()); continue; }
        const auto state = B::Gen4Bag::pockets(*editor, err);
        CHECK(err.empty() && state.size() == G4::BagPocketCount);
        const auto layout = G4::bagLayout(g.layout);
        std::vector<Native> native;
        for (std::size_t p = 0; p < state.size(); ++p) native.push_back({state[p].pocket, layout[p].slots});
        for (const auto current : {B::Pocket::Items, B::Pocket::TMHM, B::Pocket::Medicine})
            everyScope(std::string("Gen IV ") + g.id, state, current, [&](const std::string& label, const B::Request& req) {
                const auto plan = B::plan(g.policy, req, state);
                exact(label, req, plan, state, native);
                noKeyItem(g.policy, plan);
                if (std::find(req.pockets.begin(), req.pockets.end(), B::Pocket::TMHM) != req.pockets.end())
                    allMachines(label, plan, state, true);
            });
        const auto req = Model::request(scoped(Model::Scope::AllBag, 999, B::Mode::Both), B::Pocket::TMHM, state);
        const auto plan = B::plan(g.policy, req, state);
        CHECK(addsIn(plan, B::Pocket::TMHM) == 100);   // TM01-92 and HM01-08
        for (uint16_t hm = 420; hm <= 427; ++hm) CHECK(added(plan, B::Pocket::TMHM, hm));
        CHECK(B::apply(*editor, plan, B::Gen4Bag::Adapter{}).applied);
        readsBack(*editor, plan, B::Gen4Bag::Adapter{});
        const auto out = editor->finalizedBytes(&err);
        CHECK(!out.empty());
        auto reopened = G4::Gen4StagedPokemonEditor::create(out, g.layout, g.id, &err);
        CHECK(reopened.has_value());
        if (reopened) readsBack(*reopened, plan, B::Gen4Bag::Adapter{});
        CHECK(editor->originalBytes() == raw);
    }
}

void g5() {
    namespace G5 = PokeVault::Integration::Gen5;
    using Family = G5::SaveFamily;
    struct G { Family family; uint8_t version; const char* id; QP::Game policy; };
    for (const auto& g : {G{Family::BlackWhite, 21, "black_nds", QP::Game::Black}, G{Family::Black2White2, 23, "black2_nds", QP::Game::Black2}}) {
        auto raw = qasynth::gen5::save(g.family, g.version);
        qasynth::gen5::forge(raw);
        std::string err;
        const auto parsed = G5::Gen5ReadOnlySave::parse(raw, g.id, &err, G5::SaveCopySelection::Primary);
        CHECK(parsed.has_value());
        if (!parsed) { std::fprintf(stderr, "  Gen V %s: %s\n", g.id, err.c_str()); continue; }
        G5::Gen5StagedPokemonWorkspace ws(*parsed);
        auto* bag = ws.stagedBag(&err);
        CHECK(bag != nullptr);
        if (!bag) continue;
        const auto state = B::Gen5Bag::pockets(*bag);
        std::vector<Native> native;
        for (std::size_t p = 0; p < state.size(); ++p) native.push_back({state[p].pocket, G5::bagPocketLayout5(g.family, p).slots});
        for (const auto current : {B::Pocket::Items, B::Pocket::TMHM, B::Pocket::Medicine})
            everyScope(std::string("Gen V ") + g.id, state, current, [&](const std::string& label, const B::Request& req) {
                const auto plan = B::plan(g.policy, req, state);
                exact(label, req, plan, state, native);
                noKeyItem(g.policy, plan);
                if (std::find(req.pockets.begin(), req.pockets.end(), B::Pocket::TMHM) != req.pockets.end())
                    allMachines(label, plan, state, true);
            });
        const auto req = Model::request(scoped(Model::Scope::AllBag, 999, B::Mode::Both), B::Pocket::TMHM, state);
        const auto plan = B::plan(g.policy, req, state);
        for (uint16_t hm = 420; hm <= 425; ++hm) CHECK(added(plan, B::Pocket::TMHM, hm));
        for (const auto& a : plan.adds) if (a.pocket == B::Pocket::TMHM) CHECK(a.quantity == 1);   // Gen V machines: one each
        CHECK(B::apply(*bag, plan, B::Gen5Bag::Adapter{}).applied);
        readsBack(*bag, plan, B::Gen5Bag::Adapter{});
        const auto candidate = G5::buildStagedSaveCandidate(ws, &err);
        CHECK(candidate.has_value() && candidate && candidate->bagChanged);
        if (candidate) {
            const auto reparsed = G5::Gen5ReadOnlySave::parse(candidate->bytes, g.id, &err, G5::SaveCopySelection::Primary);
            CHECK(reparsed.has_value());
            if (reparsed) {
                auto check = G5::Gen5StagedBag::create(*reparsed, &err);
                CHECK(check.has_value());
                if (check) readsBack(*check, plan, B::Gen5Bag::Adapter{});
            }
        }
    }
}
void run() { g1(); g2(); g3(); g4(); g5(); }
} // namespace coverage

// The shared model: SETUP choices, request scope, the preview lines and B / A steps.
void model() {
    Model::State s;
    // Round 11 (H04 / H34): Scope is the first row (the cursor starts on it); on a Bag tab it starts
    // at All Bag pockets, its row shows "< ... >" so the other choices are discoverable.
    Model::open(s, B::Pocket::Medicine, true);
    CHECK(s.step == Model::Step::Setup && s.row == 0 && s.mode == B::Mode::Both && s.quantity == 99);
    CHECK(static_cast<Model::Row>(s.row) == Model::Row::Scope && s.scope == Model::Scope::AllBag && !s.scopeChosen);
    CHECK(Model::rowDetail(s, Model::Row::Scope, "Medicine") == "< All Bag pockets >");
    {
        // Round 11 QA: Gen I's one 20-slot shared Bag is named as such, never as per-pocket room.
        Model::State g1;
        Model::open(g1, B::Pocket::Medicine, true, 20);
        CHECK(Model::rowDetail(g1, Model::Row::Scope, "Medicine") == "< Whole Bag (20 slots shared) >");
        // A partial fill reports what did not fit and why; a complete one keeps the plain status.
        B::Plan full;
        full.full.push_back({B::Pocket::Items, 20, 3, 17, true});
        const std::string partial = Model::appliedStatus(0, 3, full);
        CHECK(partial.rfind("Partly staged: 0 topped up, 3 added; 17 did not fit because the Bag is full (20 slots shared", 0) == 0);
        CHECK(partial.find("Nothing overflowed") != std::string::npos);
        CHECK(Model::appliedStatus(1, 2, B::Plan{}) == Model::appliedStatus(1, 2));
        B::Plan pocketFull;
        pocketFull.full.push_back({B::Pocket::Items, 30, 0, 4, false});
        CHECK(Model::appliedStatus(0, 0, pocketFull).find("4 did not fit because a pocket is full") != std::string::npos);
        // Round 12 (owner H14): surfaces name the full list and its native size, never "a pocket".
        const auto named = [](B::Pocket p) { return p == B::Pocket::Items ? std::string("Items") : std::string("PC Items"); };
        CHECK(Model::appliedStatus(0, 20, pocketFull, named) ==
              "Partly staged: 0 topped up, 20 added; 4 did not fit because Items (30 native slots) is full. "
              "Nothing overflowed; source save unchanged (Review lists them)");
        pocketFull.full.push_back({B::Pocket::PCItems, 50, 10, 6, false});
        CHECK(Model::appliedStatus(1, 2, pocketFull, named).find(
                  "10 did not fit because Items (30 native slots) and PC Items (50 native slots) are full.") != std::string::npos);
        CHECK(Model::appliedStatus(0, 3, full, named) == Model::appliedStatus(0, 3, full));   // Gen I wording kept
        CHECK(Model::appliedStatus(1, 2, B::Plan{}, named) == Model::appliedStatus(1, 2));
        // Every list Add Missing covered gets a capacity line: full ones first, then the others.
        B::Plan rooms;
        rooms.full.push_back({B::Pocket::Items, 20, 20, 118, false});
        rooms.adds = {{B::Pocket::Items, 13, 99}};
        rooms.rooms = {{B::Pocket::Items, 20, 20, 138, 20, 118, false}, {B::Pocket::Balls, 16, 16, 11, 11, 0, false},
                       {B::Pocket::KeyItems, 20, 20, 0, 0, 0, false}};
        const auto roomLines = Model::previewLines(rooms, {B::Pocket::Items}, [](B::Pocket, uint16_t id) { return std::to_string(id); },
                                                   [](B::Pocket p) { return p == B::Pocket::Items ? std::string("Items")
                                                                          : p == B::Pocket::Balls ? std::string("Balls") : std::string("Key Items"); });
        CHECK(roomLines.size() >= 3 && roomLines[0].text == "Items (20 slots): 20 free, 118 more did not fit" &&
              roomLines[1].text == "Balls (16 slots): 16 free, all 11 missing added" &&
              roomLines[2].text == "Key Items (20 slots): 20 free, nothing missing to add");
    }
    // Gen I-III (PC item storage): This pocket -> All Bag -> PC Items -> Bag + PC Items, both ways.
    CHECK((Model::scopes(s) == std::vector<Model::Scope>{Model::Scope::ThisPocket, Model::Scope::AllBag,
                                                         Model::Scope::PcItems, Model::Scope::BagAndPc}));
    CHECK(Model::setupInput(s, {.right = true}) == Model::Action::None && s.scope == Model::Scope::PcItems && s.scopeChosen);
    CHECK(Model::rowDetail(s, Model::Row::Scope, "Medicine") == "< PC Items >");
    CHECK(Model::setupInput(s, {.a = true}) == Model::Action::None && s.scope == Model::Scope::BagAndPc);
    CHECK(Model::setupInput(s, {.right = true}) == Model::Action::None && s.scope == Model::Scope::ThisPocket);
    CHECK(Model::rowDetail(s, Model::Row::Scope, "Medicine") == "< This pocket (Medicine) >");
    CHECK(Model::setupInput(s, {.left = true}) == Model::Action::None && s.scope == Model::Scope::BagAndPc);
    CHECK(Model::scopeLabel(Model::Scope::BagAndPc, "x") == "Bag + PC Items");
    // A picked scope is kept for the session; one the save cannot serve is not.
    Model::close(s);
    Model::open(s, B::Pocket::Items, true);
    CHECK(s.scope == Model::Scope::BagAndPc && s.scopeChosen);
    Model::close(s);
    Model::open(s, B::Pocket::Items, false);   // Gen IV / V: no PC Items
    CHECK(s.scope == Model::Scope::AllBag && !s.scopeChosen);
    CHECK((Model::scopes(s) == std::vector<Model::Scope>{Model::Scope::ThisPocket, Model::Scope::AllBag}));
    CHECK(Model::setupInput(s, {.right = true}) == Model::Action::None && s.scope == Model::Scope::ThisPocket);
    CHECK(Model::setupInput(s, {.right = true}) == Model::Action::None && s.scope == Model::Scope::AllBag);
    CHECK(Model::setupInput(s, {.left = true}) == Model::Action::None && s.scope == Model::Scope::ThisPocket);
    // Never picked: the PC Items tab starts on the PC (Gen I-III only).
    {
        Model::State t;
        Model::open(t, B::Pocket::PCItems, true);
        CHECK(t.scope == Model::Scope::PcItems);
        Model::State u;
        Model::open(u, B::Pocket::TMHM, true);
        CHECK(u.scope == Model::Scope::AllBag);
    }
    // Row 1 is Mode: Left / Right / A change it.
    s.row = 0;
    CHECK(Model::setupInput(s, {.down = true}) == Model::Action::None && s.row == 1);
    CHECK(Model::setupInput(s, {.right = true}) == Model::Action::None && s.mode == B::Mode::TopUpExisting);
    CHECK(Model::setupInput(s, {.left = true}) == Model::Action::None && s.mode == B::Mode::Both);
    CHECK(Model::setupInput(s, {}, 2) == Model::Action::PromptAmount && s.row == 2);
    Model::setAmount(s, 5000);
    CHECK(s.quantity == Model::kMaxQuantity);
    Model::setAmount(s, 0);
    CHECK(s.quantity == 1);
    CHECK(Model::setupInput(s, {.up = true, .down = false}) == Model::Action::None && s.row == 1);
    CHECK(Model::setupInput(s, {}, 3) == Model::Action::BuildPreview);
    // A request: this pocket, every Bag pocket (never the PC), the PC alone, or Bag + PC.
    std::vector<B::PocketState> st{{B::Pocket::Items, {}, 1, {}}, {B::Pocket::Balls, {}, 1, {}}, {B::Pocket::PCItems, {}, 50, {}}};
    s.scope = Model::Scope::ThisPocket;
    CHECK(Model::request(s, B::Pocket::Balls, st).pockets == std::vector<B::Pocket>{B::Pocket::Balls});
    s.scope = Model::Scope::AllBag;
    CHECK((Model::request(s, B::Pocket::Balls, st).pockets == std::vector<B::Pocket>{B::Pocket::Items, B::Pocket::Balls}));
    s.scope = Model::Scope::PcItems;
    CHECK(Model::request(s, B::Pocket::Balls, st).pockets == std::vector<B::Pocket>{B::Pocket::PCItems});
    s.scope = Model::Scope::BagAndPc;
    CHECK((Model::request(s, B::Pocket::Balls, st).pockets ==
           std::vector<B::Pocket>{B::Pocket::Items, B::Pocket::Balls, B::Pocket::PCItems}));
    // A save without PC Items never gets an empty request (the planner reads empty as "every pocket").
    std::vector<B::PocketState> nds{{B::Pocket::Items, {}, 1, {}}, {B::Pocket::Medicine, {}, 1, {}}};
    s.scope = Model::Scope::PcItems;
    CHECK(Model::request(s, B::Pocket::Medicine, nds).pockets == std::vector<B::Pocket>{B::Pocket::Medicine});
    s.scope = Model::Scope::BagAndPc;
    CHECK(Model::request(s, B::Pocket::Medicine, nds).pockets.size() == 2);
    CHECK(B::bagPockets(st) == (std::vector<B::Pocket>{B::Pocket::Items, B::Pocket::Balls}));
    CHECK(B::pcPockets(st) == std::vector<B::Pocket>{B::Pocket::PCItems} && B::pcPockets(nds).empty());
    CHECK(B::hasPcItems(st) && !B::hasPcItems(nds));
    s.scope = Model::Scope::AllBag;

    B::Plan plan;
    plan.topUps = {{B::Pocket::Items, 0, 13, 3, 99}};
    plan.adds = {{B::Pocket::Balls, 2, 99}};
    plan.skipped = {{B::Pocket::Balls, 1, B::SkipReason::PocketFull}};
    const auto name = [](B::Pocket, uint16_t id) { return id == 13 ? std::string("Potion") : id == 2 ? std::string("Ultra Ball") : std::string("Master Ball"); };
    const auto pocketName = [](B::Pocket p) { return p == B::Pocket::Items ? std::string("Items") : std::string("Balls"); };
    Model::showPreview(s, plan, {B::Pocket::Items, B::Pocket::Balls}, name, pocketName);
    CHECK(s.step == Model::Step::Preview);
    std::vector<std::string> texts;
    for (const auto& l : s.lines) texts.push_back(l.text);
    CHECK((texts == std::vector<std::string>{"Items", "Potion 3 \xE2\x86\x92 99", "Balls", "+ Ultra Ball \xC3\x97" "99",
                                             "Master Ball: pocket full"}));
    CHECK(s.totals == "1 topped up, 1 added, 1 skipped");
    CHECK(Model::previewInput(s, {.down = true}, 2) == Model::Action::None && s.scroll == 1);
    // A page is one line short of the view (one line of context stays); the end clamps.
    CHECK(Model::previewInput(s, {.pageDown = true}, 2) == Model::Action::None && s.scroll == 2);
    CHECK(Model::previewInput(s, {.pageDown = true}, 2) == Model::Action::None && s.scroll == 3);
    CHECK(Model::previewInput(s, {.pageDown = true}, 2) == Model::Action::None && s.scroll == 3);
    CHECK(Model::previewInput(s, {.a = true}, 2) == Model::Action::Confirm);
    CHECK(Model::previewInput(s, {.b = true}, 2) == Model::Action::None && s.step == Model::Step::Setup && s.plan.empty());
    CHECK(Model::setupInput(s, {.b = true}) == Model::Action::Close && !s.active());
    // An empty plan cannot be confirmed; its preview says why.
    Model::open(s);
    Model::showPreview(s, B::Plan{}, {B::Pocket::Items}, name, pocketName);
    CHECK(!s.lines.empty() && s.lines[0].kind == Model::LineKind::Note);
    CHECK(Model::previewInput(s, {.a = true}, 5) == Model::Action::None && s.step == Model::Step::Preview);
    CHECK(Model::samePlan(plan, plan) && !Model::samePlan(plan, B::Plan{}));
}

} // namespace

int main() {
    model();
    gen1::run();
    gen1::pc();
    gen1::maxElixer();
    gen2::run();
    gen2::pc();
    gen3::run();
    gen3::pc();
    gen4::run();
    gen5::run();
    coverage::run();
    if (failures) {
        std::cerr << failures << " Top Up / Add Missing adapter check(s) failed\n";
        return 1;
    }
    std::cout << "Top Up / Add Missing adapters Gen I-V (plan + apply over the real staged Bags, atomic, "
                 "story items untouched, native caps, pocket full, strict reparse, Discard; Gen I-III "
                 "PC Items and Bag + PC Items scopes; Round 12: every Gen I-V game family and scope plans "
                 "exactly min(eligible, native free slots) per list, nothing dropped, every TM / HM): PASS\n";
    return 0;
}
