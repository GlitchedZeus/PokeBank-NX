// Round 14 (owner Round 13 H07 "again only adds some of the items see screen shots", H08 "Every TM/HM";
// shared-UX audit P0-9 "a partial Bag Top Up / Add Missing with no list of what was skipped is a silent
// drop"). The shared planner (Inventory/BulkItemPlan.h) and model (UI/BagTopUpModel.h) on realistic,
// partly filled Bags built from the real per-game Add-picker lists:
//   * the pocket on screen is planned first, so on Gen I's ONE 20-slot Bag the TM/HM tab fills with
//     TMs and HMs instead of the lowest item ids of the Items category;
//   * Bag + PC Items covers as many DIFFERENT items as the native slots allow: the PC first takes what
//     did not fit in the Bag (each such item says "added to PC Items instead"), then items held
//     nowhere, and repeats a Bag item only when nothing else is left;
//   * before A, PREVIEW opens with one emphasised plain-words line per native list ("Items: 30
//     native slots, 5 used. Only 25 fit; 113 missing items won't fit"), its totals count "won't fit"
//     apart from other skips, and every eligible missing item is listed exactly once, as an add or
//     with its reason;
//   * after A, RESULT lists the same lines as what was / was not staged; A or B closes it.
#include "Inventory/BulkItemPlan.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "UI/BagTopUpModel.h"

#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace B = PokeVault::Inventory::BulkItems;
namespace C = PokeVault::Inventory;
namespace Model = PokeBank::UIModel::BagTopUp;
using B::Pocket;

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

std::vector<uint16_t> addable(C::ClassicGame g, C::ClassicPocket p) {
    const auto s = C::addableItems(g, p);
    return {s.begin(), s.end()};
}
// `count` held stacks taken from the start of the pocket's own list (a realistic partly filled Bag).
B::PocketState pocket(Pocket p, std::size_t slots, std::vector<uint16_t> list, std::size_t count) {
    B::PocketState s{p, {}, slots, list};
    for (std::size_t i = 0; i < count && i < list.size(); ++i) s.held.push_back({i, list[i], 1});
    return s;
}
std::size_t addsIn(const B::Plan& plan, Pocket p) {
    return static_cast<std::size_t>(std::count_if(plan.adds.begin(), plan.adds.end(), [p](const B::Add& a) { return a.pocket == p; }));
}
std::string itemName(Pocket, uint16_t id) { return "#" + std::to_string(id); }
std::string pocketName(Pocket p) {
    switch (p) {
        case Pocket::Items: return "Items";
        case Pocket::Medicine: return "Medicine";
        case Pocket::Balls: return "Balls";
        case Pocket::KeyItems: return "Key Items";
        case Pocket::TMHM: return "TMs & HMs";
        case Pocket::Berries: return "Berries";
        case Pocket::PCItems: return "PC Items";
        default: return "Pocket";
    }
}

// P0-9: every candidate the Add picker offers that is not held is listed exactly once in PREVIEW,
// either as "+ name" or as "name: <reason>".
void everyMissingItemListed(const std::string& label, const std::vector<B::PocketState>& state, const B::Request& req,
                            const std::vector<Model::Line>& lines) {
    for (const auto& p : state) {
        if (std::find(req.pockets.begin(), req.pockets.end(), p.pocket) == req.pockets.end()) continue;
        std::set<uint16_t> missing(p.addableItems.begin(), p.addableItems.end());
        for (const auto& h : p.held) missing.erase(h.itemId);
        missing.erase(0);
        // Lines are grouped under the pocket's heading; count the item's lines in that group.
        for (const uint16_t id : missing) {
            int listed = 0;
            bool inGroup = req.pockets.size() == 1;
            for (const auto& l : lines) {
                if (l.kind == Model::LineKind::Heading) { inGroup = l.text == pocketName(p.pocket); continue; }
                if (!inGroup) continue;
                if (l.kind == Model::LineKind::Add && l.text.rfind("+ " + itemName(p.pocket, id) + " ", 0) == 0) ++listed;
                if (l.kind == Model::LineKind::Skip && l.text.rfind(itemName(p.pocket, id) + ": ", 0) == 0) ++listed;
            }
            CHECK(listed == 1);
            if (listed != 1) std::fprintf(stderr, "  %s: item %u of %s listed %d times\n", label.c_str(), id, pocketName(p.pocket).c_str(), listed);
        }
    }
}
} // namespace

int main() {
    // 1. Gen I (pret pokered wNumBagItems): Items, Medicine, Balls, Key Items and TMs are views of ONE
    //    20-slot Bag. 3 stacks held (Potion, Poke Ball, TM01). On the TM/HM tab, "All Bag pockets"
    //    fills the 17 free slots with TMs / HMs (H08), not with the lowest Items ids.
    {
        std::vector<B::PocketState> state{
            pocket(Pocket::Items, 20, addable(C::ClassicGame::Red, C::ClassicPocket::Items), 0),
            pocket(Pocket::Medicine, 20, addable(C::ClassicGame::Red, C::ClassicPocket::Medicines), 1),
            pocket(Pocket::Balls, 20, addable(C::ClassicGame::Red, C::ClassicPocket::Balls), 1),
            pocket(Pocket::KeyItems, 20, addable(C::ClassicGame::Red, C::ClassicPocket::KeyItems), 0),
            pocket(Pocket::TMHM, 20, addable(C::ClassicGame::Red, C::ClassicPocket::TMHM), 1),
        };
        for (auto& p : state) { p.slotGroup = 0; p.perSlotTopUp = false; }
        Model::State s;
        s.scope = Model::Scope::AllBag;
        s.mode = B::Mode::AddMissing;
        const auto req = Model::request(s, Pocket::TMHM, state);
        CHECK(!req.pockets.empty() && req.pockets.front() == Pocket::TMHM);
        const auto plan = B::plan(B::Game::Red, req, state);
        CHECK(plan.adds.size() == 17);
        CHECK(addsIn(plan, Pocket::TMHM) == 17);   // base: 0 (Items 1..19 took every slot)
        CHECK(plan.rooms.size() == 1 && plan.rooms[0].sharedList && plan.rooms[0].slots == 20 &&
              plan.rooms[0].freeBefore == 17 && plan.rooms[0].planned == 17);
        // The capacity line says it plainly, before any item line, and is emphasised.
        Model::showPreview(s, plan, req.pockets, itemName, pocketName);
        CHECK(!s.lines.empty() && s.lines[0].emphasis && s.lines[0].kind == Model::LineKind::Note &&
              s.lines[0].text.rfind("Bag (all categories share 20 native slots): 3 used. Only 17 fit; ", 0) == 0 &&
              s.lines[0].text.find("missing items won't fit") != std::string::npos);
        CHECK(s.totals.find("17 added, ") != std::string::npos && s.totals.find(" won't fit (native slots), ") != std::string::npos);
        everyMissingItemListed("Red All Bag", state, req, s.lines);
    }

    // 2. Emerald Bag + PC Items (pret pokeemerald: Items 30, PC 50). 5 Items held, 1 PC item. The
    //    PC takes what did not fit in the Items pocket first: 25 + 49 = 74 DIFFERENT items, none
    //    repeated, and every overflow item the PC took says so.
    {
        const auto items = addable(C::ClassicGame::Emerald, C::ClassicPocket::Items);
        std::vector<B::PocketState> state{
            pocket(Pocket::Items, 30, items, 5),
            pocket(Pocket::PCItems, 50, addable(C::ClassicGame::Emerald, C::ClassicPocket::PCItems), 0),
        };
        state[1].held.push_back({0, items[0], 1});   // the PC holds one Potion-like stack too
        for (auto& p : state) p.perSlotTopUp = false;
        Model::State s;
        s.scope = Model::Scope::BagAndPc;
        s.mode = B::Mode::AddMissing;
        const auto req = Model::request(s, Pocket::Items, state);
        CHECK((req.pockets == std::vector<Pocket>{Pocket::Items, Pocket::PCItems}));
        const auto plan = B::plan(B::Game::Emerald, req, state);
        CHECK(addsIn(plan, Pocket::Items) == 25 && addsIn(plan, Pocket::PCItems) == 49);
        std::set<uint16_t> distinct;
        for (const auto& a : plan.adds) distinct.insert(a.itemId);
        CHECK(distinct.size() == plan.adds.size());   // base: the PC repeated the Bag's lowest ids
        for (const auto& h : state[0].held)   // nor a second copy of what the Bag already holds
            CHECK(std::none_of(plan.adds.begin(), plan.adds.end(), [&](const B::Add& a) { return a.itemId == h.itemId; }));
        std::size_t moved = 0;
        for (const auto& k : plan.skipped)
            if (k.pocket == Pocket::Items && k.reason == B::SkipReason::PocketFull) {
                const bool inPc = std::any_of(plan.adds.begin(), plan.adds.end(), [&](const B::Add& a) {
                    return a.pocket == Pocket::PCItems && a.itemId == k.itemId; });
                CHECK(k.addedToPc == inPc);
                CHECK(B::skipText(k).rfind("won't fit (this pocket has only 30 native slots)", 0) == 0);
                CHECK((B::skipText(k).find("added to PC Items instead") != std::string::npos) == inPc);
                moved += inPc;
            }
        CHECK(moved == 49);
        Model::showPreview(s, plan, req.pockets, itemName, pocketName);
        CHECK(s.lines.size() >= 2 && s.lines[0].emphasis && s.lines[1].emphasis);
        CHECK(s.lines[0].text.rfind("Items: 30 native slots, 5 used. Only 25 fit; ", 0) == 0);
        CHECK(s.lines[1].text.rfind("PC Items: 50 native slots, 1 used. Only 49 fit; ", 0) == 0);
        everyMissingItemListed("Emerald Bag + PC", state, req, s.lines);

        // 3. RESULT after A: what was staged and what was not, the same lines; A or B closes it.
        auto lines = s.lines;
        Model::close(s);
        Model::showResult(s, lines, plan);
        CHECK(s.step == Model::Step::Result && s.active() && s.plan.empty());
        CHECK(s.lines.size() == lines.size() + 1 && s.lines[0].text.rfind("Staged: ", 0) == 0);
        CHECK(s.totals.rfind("Staged: 0 topped up, 74 added, ", 0) == 0);
        CHECK(Model::resultInput(s, {.down = true}, 5) == Model::Action::None && s.scroll == 1 && s.step == Model::Step::Result);
        CHECK(Model::previewInput(s, {.a = true}, 5) == Model::Action::None);   // never a second Confirm
        CHECK(Model::resultInput(s, {.a = true}, 5) == Model::Action::Close && !s.active());
        Model::showResult(s, lines, plan);
        CHECK(Model::resultInput(s, {.b = true}, 5) == Model::Action::Close && !s.active());
    }

    // 4. A list where everything fits says so in the same plain words; nothing is listed as skipped.
    {
        std::vector<B::PocketState> state{pocket(Pocket::Balls, 16, addable(C::ClassicGame::Emerald, C::ClassicPocket::Balls), 2)};
        Model::State s;
        s.scope = Model::Scope::ThisPocket;
        s.mode = B::Mode::AddMissing;
        const auto req = Model::request(s, Pocket::Balls, state);
        const auto plan = B::plan(B::Game::Emerald, req, state);
        Model::showPreview(s, plan, req.pockets, itemName, pocketName);
        CHECK(!s.lines.empty() && s.lines[0].text == "Balls: 16 native slots, 2 used. All 9 missing items fit");
        CHECK(s.totals == "0 topped up, 9 added, 0 skipped");
    }

    if (failures) {
        std::fprintf(stderr, "%d Top Up capacity check(s) failed\n", failures);
        return 1;
    }
    std::puts("Top Up / Add Missing capacity (Round 14 H07 / H08 / P0-9: screen pocket first, Bag + PC distinct, "
              "plain native-slot lines, every missing item listed, RESULT): PASS");
    return 0;
}
