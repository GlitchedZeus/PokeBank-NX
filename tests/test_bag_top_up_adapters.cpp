// Top Up / Add Missing (owner Oct 28, Feature B): the Gen I-V adapters over the REAL staged Bag
// editors, end to end on synthetic saves (builders shared with tests/test_bag_add_flow_gen15.cpp and
// tests/fuzz_synthetic_saves.h; no real save, ROM or key is read).
//
// For every generation: the plan uses the Add picker's own item list; plan + apply reads back exactly
// in the staged editor; Key Items and HMs are never touched; per-item native caps hold; a full
// pocket is reported, never forced; a refused step leaves the editor byte-identical (edits staged
// before included); the finalized / export candidate reparses strictly with the new counts; the
// source bytes never change; Discard restores the source. Gen II tops up each of an item's two
// stacks; Gen I / III report an item held in several stacks instead of merging it.
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
// new items are exactly the Add picker's list for their pocket.
void planIsSafe(QP::Game game, const B::Plan& plan, std::span<const B::PocketState> state) {
    for (const auto& t : plan.topUps) {
        const auto rule = QP::quantityRule(game, t.pocket, t.itemId);
        CHECK(rule.valid && !B::storySensitive(rule) && t.to <= rule.maximum && t.to > t.from);
    }
    for (const auto& a : plan.adds) {
        const auto rule = QP::quantityRule(game, a.pocket, a.itemId);
        CHECK(rule.valid && !B::storySensitive(rule) && a.quantity >= 1 && a.quantity <= rule.maximum);
        const auto p = std::find_if(state.begin(), state.end(), [&](const B::PocketState& s) { return s.pocket == a.pocket; });
        CHECK(p != state.end() && std::find(p->addableItems.begin(), p->addableItems.end(), a.itemId) != p->addableItems.end());
        CHECK(a.pocket != B::Pocket::PCItems);   // "all pockets" never fills the PC
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

std::vector<uint16_t> addable(I::ClassicGame g, I::ClassicPocket p) {
    const auto s = I::addableItems(g, p);
    return {s.begin(), s.end()};
}

// ------------------------------------------------------------------------------------- Gen I --
namespace gen1 {
namespace G1 = PokeVault::Integration::Gen1;
std::vector<uint8_t> fixture(const std::vector<std::pair<uint8_t, uint8_t>>& bag) {
    auto raw = qasynth::gen1::save();
    raw[0x25C9] = static_cast<uint8_t>(bag.size());          // pret wNumBagItems
    for (std::size_t i = 0; i < bag.size(); ++i) { raw[0x25CA + 2 * i] = bag[i].first; raw[0x25CB + 2 * i] = bag[i].second; }
    raw[0x25CA + 2 * bag.size()] = 0xFF;
    raw[0x27E6] = 0; raw[0x27E7] = 0xFF;                      // empty PC item list
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
        [] { Model::State s; s.allPockets = true; s.quantity = 999; return s; }(), B::Pocket::Medicine, B::bagPockets(state));
    const auto plan = B::plan(B::Gen1Bag::game(*editor), request, state);
    planIsSafe(QP::Game::Red, plan, state);
    CHECK(skipped(plan, 20, B::SkipReason::SeveralStacks) && !topUp(plan, 20));   // never merges two stacks
    CHECK(topUp(plan, 4) && topUp(plan, 4)->from == 7 && topUp(plan, 4)->to == 99);  // 999 asked, native 99
    CHECK(topUp(plan, 201) && topUp(plan, 201)->to == 99);
    CHECK(skipped(plan, 6, B::SkipReason::KeyOrStoryItem) && skipped(plan, 196, B::SkipReason::KeyOrStoryItem));
    CHECK(plan.adds.size() == 2);   // one 20-slot Bag shared by every category: 2 free slots in all
    std::size_t full = 0;
    for (const auto& s : plan.skipped) full += s.reason == B::SkipReason::PocketFull;
    CHECK(full > 0);

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
std::vector<uint8_t> fixture(const L& l, const std::vector<std::pair<uint8_t, uint8_t>>& items) {
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
    d[l.pc] = 0; d[l.pc + 1] = 0xFF;
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
    const auto items1 = B::plan(B::Gen2Bag::game(*editor), Model::request(s, B::Pocket::Items, B::bagPockets(state)), state);
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
    s.allPockets = true;
    const auto state2 = B::Gen2Bag::pockets(*editor, err);
    const auto all = B::plan(B::Gen2Bag::game(*editor), Model::request(s, B::Pocket::Items, B::bagPockets(state2)), state2);
    planIsSafe(QP::Game::Crystal, all, state2);
    CHECK(addsIn(all, B::Pocket::Items) == 1 && count(all, B::SkipReason::PocketFull, B::Pocket::Items) > 0);
    CHECK(addsIn(all, B::Pocket::Balls) > 0 && addsIn(all, B::Pocket::Balls) <= 12);
    CHECK(skipped(all, 7, B::SkipReason::KeyOrStoryItem) && skipped(all, 243, B::SkipReason::KeyOrStoryItem));
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
std::vector<uint8_t> fixture(const std::vector<Put>& puts) {
    LogicalSectors logical{};
    write32(logical[0], 0xAC, 0);
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
    s.allPockets = true;
    s.quantity = 999;
    const auto plan = B::plan(B::Gen3Bag::game(*editor), Model::request(s, B::Pocket::Items, B::bagPockets(state)), state);
    planIsSafe(QP::Game::Ruby, plan, state);
    CHECK(skipped(plan, 13, B::SkipReason::SeveralStacks) && !topUp(plan, 13));
    CHECK(topUp(plan, 14) && topUp(plan, 14)->to == 99);   // R/S cap 99 even when 999 is asked
    CHECK(topUp(plan, 289) && topUp(plan, 289)->to == 99);
    CHECK(skipped(plan, 259, B::SkipReason::KeyOrStoryItem) && skipped(plan, 339, B::SkipReason::KeyOrStoryItem));
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
    s.allPockets = true;
    s.quantity = 999;
    const auto game = B::Gen4Bag::game(*editor);
    CHECK(game && *game == QP::Game::Platinum);
    if (!game) return;
    const auto plan = B::plan(*game, Model::request(s, B::Pocket::Medicine, B::bagPockets(state)), state);
    planIsSafe(QP::Game::Platinum, plan, state);
    CHECK(topUp(plan, 17, 0) && topUp(plan, 17, 0)->to == 999 && topUp(plan, 17, 1) && topUp(plan, 17, 1)->from == 7);
    CHECK(topUp(plan, 328) && topUp(plan, 328)->to == 99);   // Gen IV TMs: native 99
    CHECK(skipped(plan, 450, B::SkipReason::KeyOrStoryItem) && skipped(plan, 420, B::SkipReason::KeyOrStoryItem));
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
    s.allPockets = true;
    s.quantity = 999;
    const auto plan = B::plan(*game, Model::request(s, B::Pocket::Medicine, B::bagPockets(state)), state);
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

// The shared model: SETUP choices, request scope, the preview lines and B / A steps.
void model() {
    Model::State s;
    Model::open(s);
    CHECK(s.step == Model::Step::Setup && s.row == 0 && s.mode == B::Mode::Both && s.quantity == 99);
    CHECK(Model::setupInput(s, {.right = true}) == Model::Action::None && s.mode == B::Mode::TopUpExisting);
    CHECK(Model::setupInput(s, {.left = true}) == Model::Action::None && s.mode == B::Mode::Both);
    CHECK(Model::setupInput(s, {.down = true}) == Model::Action::None && s.row == 1);
    CHECK(Model::setupInput(s, {.a = true}) == Model::Action::None && s.allPockets);
    CHECK(Model::setupInput(s, {}, 2) == Model::Action::PromptAmount && s.row == 2);
    Model::setAmount(s, 5000);
    CHECK(s.quantity == Model::kMaxQuantity);
    Model::setAmount(s, 0);
    CHECK(s.quantity == 1);
    CHECK(Model::setupInput(s, {.up = true, .down = false}) == Model::Action::None && s.row == 1);
    CHECK(Model::setupInput(s, {}, 3) == Model::Action::BuildPreview);
    // A request: this pocket or every Bag pocket (never the PC).
    s.allPockets = false;
    CHECK(Model::request(s, B::Pocket::Balls, {B::Pocket::Items, B::Pocket::Balls}).pockets == std::vector<B::Pocket>{B::Pocket::Balls});
    s.allPockets = true;
    CHECK(Model::request(s, B::Pocket::Balls, {B::Pocket::Items, B::Pocket::Balls}).pockets.size() == 2);
    std::vector<B::PocketState> st{{B::Pocket::Items, {}, 1, {}}, {B::Pocket::PCItems, {}, 50, {}}};
    CHECK(B::bagPockets(st) == std::vector<B::Pocket>{B::Pocket::Items});

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
    gen2::run();
    gen3::run();
    gen4::run();
    gen5::run();
    if (failures) {
        std::cerr << failures << " Top Up / Add Missing adapter check(s) failed\n";
        return 1;
    }
    std::cout << "Top Up / Add Missing adapters Gen I-V (plan + apply over the real staged Bags, atomic, "
                 "story items untouched, native caps, pocket full, strict reparse, Discard): PASS\n";
    return 0;
}
