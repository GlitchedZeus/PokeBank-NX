// Feature B (owner Oct 28; adapted from PKSE d2ad323): the Top Up / Add Missing planner. Pure preview,
// real per-game rules (QuantityPolicy) and real Add-picker candidate lists (ClassicInventoryCatalog).
#include "Inventory/BulkItemPlan.h"
#include "Inventory/ClassicInventoryCatalog.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

namespace B = PokeVault::Inventory::BulkItems;
namespace C = PokeVault::Inventory;
using B::Game;
using B::Pocket;

namespace {
std::vector<uint16_t> addable(C::ClassicGame g, C::ClassicPocket p) {
    const auto s = C::addableItems(g, p);
    return {s.begin(), s.end()};
}
const B::TopUp* topUp(const B::Plan& plan, uint16_t id) {
    for (const auto& t : plan.topUps) if (t.itemId == id) return &t;
    return nullptr;
}
const B::Add* add(const B::Plan& plan, uint16_t id) {
    for (const auto& a : plan.adds) if (a.itemId == id) return &a;
    return nullptr;
}
bool skipped(const B::Plan& plan, uint16_t id, B::SkipReason r) {
    return std::any_of(plan.skipped.begin(), plan.skipped.end(),
                       [&](const B::Skip& s) { return s.itemId == id && s.reason == r; });
}
}  // namespace

int main() {
    // Emerald Items pocket: Potion (13) x5 topped up to the native 99 even when 999 is asked (pret /
    // PKHeX PlayerBag3E; upstream d2ad323 would give 999). A stack already above target is not lowered.
    {
        B::PocketState items{Pocket::Items, {{0, 13, 5}, {1, 17, 120}}, 30, addable(C::ClassicGame::Emerald, C::ClassicPocket::Items)};
        const auto plan = B::plan(Game::Emerald, {B::Mode::TopUpExisting, 999, {}}, std::span(&items, 1));
        assert(topUp(plan, 13) && topUp(plan, 13)->from == 5 && topUp(plan, 13)->to == 99 && topUp(plan, 13)->slot == 0);
        assert(!topUp(plan, 17) && skipped(plan, 17, B::SkipReason::AlreadyAtTarget));
        assert(plan.adds.empty());
        const auto small = B::plan(Game::Emerald, {B::Mode::TopUpExisting, 3, {}}, std::span(&items, 1));
        assert(small.empty() && skipped(small, 13, B::SkipReason::AlreadyAtTarget));
        assert(B::plan(Game::Emerald, {B::Mode::Both, 0, {}}, std::span(&items, 1)).empty());
    }
    // Add Missing fills only free slots, ascending id; the rest are reported as pocket full.
    {
        const auto balls = addable(C::ClassicGame::Emerald, C::ClassicPocket::Balls);
        B::PocketState pocket{Pocket::Balls, {}, 16, balls};
        for (std::size_t i = 0; i + 1 < 16 && i < balls.size(); ++i) pocket.held.push_back({i, balls[i], 1});
        const auto plan = B::plan(Game::Emerald, {B::Mode::AddMissing, 10, {}}, std::span(&pocket, 1));
        const std::size_t free = 16 - pocket.held.size();
        assert(plan.adds.size() == std::min<std::size_t>(free, balls.size() - pocket.held.size()));
        for (const auto& a : plan.adds) assert(a.quantity == 10);
        std::size_t full = 0;
        for (const auto& s : plan.skipped) full += s.reason == B::SkipReason::PocketFull;
        assert(plan.adds.size() + full == balls.size() - pocket.held.size());
    }
    // Key Items and HMs are never topped up or added; TMs are.
    {
        B::PocketState key{Pocket::KeyItems, {{0, 259, 1}}, 30, addable(C::ClassicGame::Emerald, C::ClassicPocket::KeyItems)};
        B::PocketState tmhm{Pocket::TMHM, {{0, 289, 1}, {1, 339, 1}}, 64, addable(C::ClassicGame::Emerald, C::ClassicPocket::TMHM)};
        std::vector<B::PocketState> bag{key, tmhm};
        const auto plan = B::plan(Game::Emerald, {B::Mode::Both, 99, {}}, bag);
        for (const auto& a : plan.adds) assert(a.pocket != Pocket::KeyItems && !(a.itemId >= 339 && a.itemId <= 346));
        assert(skipped(plan, 259, B::SkipReason::KeyOrStoryItem) && skipped(plan, 339, B::SkipReason::KeyOrStoryItem));
        assert(skipped(plan, 272, B::SkipReason::KeyOrStoryItem));          // Acro Bike: a Key Item anyway
        assert(topUp(plan, 289) && topUp(plan, 289)->to == 99);             // TM01 (single-use in Gen III)
        assert(add(plan, 290) && add(plan, 290)->quantity == 99);
    }
    // Pocket filter: only the selected pockets change.
    {
        B::PocketState items{Pocket::Items, {{0, 13, 5}}, 30, addable(C::ClassicGame::Emerald, C::ClassicPocket::Items)};
        B::PocketState balls{Pocket::Balls, {{0, 4, 1}}, 16, addable(C::ClassicGame::Emerald, C::ClassicPocket::Balls)};
        std::vector<B::PocketState> bag{items, balls};
        const auto plan = B::plan(Game::Emerald, {B::Mode::Both, 50, {Pocket::Balls}}, bag);
        for (const auto& t : plan.topUps) assert(t.pocket == Pocket::Balls);
        for (const auto& a : plan.adds) assert(a.pocket == Pocket::Balls);
        assert(topUp(plan, 4) && topUp(plan, 4)->to == 50);
    }
    // Gen II: one item may legally sit in two slots; each stack is topped up on its own.
    {
        B::PocketState items{Pocket::Items, {{0, 0x12, 99}, {1, 0x12, 40}}, 20, {}};
        const auto plan = B::plan(Game::Crystal, {B::Mode::TopUpExisting, 99, {}}, std::span(&items, 1));
        assert(plan.topUps.size() == 1 && plan.topUps[0].slot == 1 && plan.topUps[0].from == 40 && plan.topUps[0].to == 99);
    }
    // Gen I: Exp. All (75) is an ordinary item now (pret key_items.asm), so it tops up to 99.
    {
        B::PocketState items{Pocket::Items, {{0, 75, 1}}, 20, addable(C::ClassicGame::Red, C::ClassicPocket::Items)};
        const auto plan = B::plan(Game::Red, {B::Mode::TopUpExisting, 99, {}}, std::span(&items, 1));
        assert(topUp(plan, 75) && topUp(plan, 75)->to == 99);
    }
    // Mutually exclusive items: holding the Dome Fossil (41, Gen I) keeps the Helix Fossil (42) out.
    {
        B::PocketState key{Pocket::KeyItems, {{0, 41, 1}}, 20, addable(C::ClassicGame::Red, C::ClassicPocket::KeyItems)};
        const auto plan = B::plan(Game::Red, {B::Mode::AddMissing, 1, {}}, std::span(&key, 1));
        assert(!add(plan, 42));
    }
    // Gen I: pockets that view ONE native list share its free slots (slotGroup), whichever are planned.
    {
        B::PocketState items{Pocket::Items, {{0, 75, 1}}, 3, addable(C::ClassicGame::Red, C::ClassicPocket::Items)};
        B::PocketState medicine{Pocket::Medicine, {{0, 20, 1}}, 3, addable(C::ClassicGame::Red, C::ClassicPocket::Medicines)};
        items.slotGroup = medicine.slotGroup = 0;
        std::vector<B::PocketState> bag{items, medicine};
        const auto plan = B::plan(Game::Red, {B::Mode::AddMissing, 5, {}}, bag);
        assert(plan.adds.size() == 1);   // 3 slots, 2 held: one new stack in all
        const auto medOnly = B::plan(Game::Red, {B::Mode::AddMissing, 5, {Pocket::Medicine}}, bag);
        assert(medOnly.adds.size() == 1 && medOnly.adds[0].pocket == Pocket::Medicine);
    }
    // A writer that sets an item by id (perSlotTopUp = false) never merges two stacks: reported once.
    {
        B::PocketState items{Pocket::Items, {{0, 13, 5}, {1, 13, 7}, {2, 14, 1}}, 20, {}};
        items.perSlotTopUp = false;
        const auto plan = B::plan(Game::Emerald, {B::Mode::TopUpExisting, 99, {}}, std::span(&items, 1));
        assert(!topUp(plan, 13) && topUp(plan, 14));
        assert(std::count_if(plan.skipped.begin(), plan.skipped.end(), [](const B::Skip& s) {
                   return s.itemId == 13 && s.reason == B::SkipReason::SeveralStacks; }) == 1);
        assert(B::skipReasonText(B::SkipReason::SeveralStacks) == "held in several stacks; edit it on its own");
    }
    // "Every pocket" never includes the PC.
    {
        std::vector<B::PocketState> bag{{Pocket::Items, {}, 1, {}}, {Pocket::PCItems, {}, 50, {}}, {Pocket::Balls, {}, 1, {}}};
        assert((B::bagPockets(bag) == std::vector<Pocket>{Pocket::Items, Pocket::Balls}));
    }
    std::cout << "Top Up / Add Missing planner (caps, exclusions, capacity, filters): PASS\n";
    return 0;
}
