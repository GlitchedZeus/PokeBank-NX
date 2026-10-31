// QA regression: Gen IV staged Bag Remove must pack the pocket (no hole), because the game's bag
// list stops at the first empty slot (pret/pokeplatinum bag.c Pocket_SortEmpty;
// src/applications/bag/main.c list build `break`s on item==NONE || quantity==0).
// Covers DP/Pt/HGSS; first/middle/last/only slot; full pockets; repeated Remove/Add; discard;
// checksum + strict reparse; ghost/unused slots preserved; source bytes immutable.
// Integration review (pret pokediamond/pokeplatinum/pokeheartgold bag.c): only the slots the game
// lists are packed; slots past the list stay byte-identical; a pocket with stacks after an empty
// slot, or an item ID 0xFFFF stack inside the list, is pre-existing damage and Remove is refused
// with the staged bytes unchanged instead of rearranging it.
#include "Integration/Gen4/Gen4BagCatalog.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "fuzz_synthetic_saves.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <span>
#include <vector>

using namespace PokeVault::Integration::Gen4;
namespace G = qasynth::gen4;
#define CHECK(c) do{ if(!(c)){ std::cerr<<"FAIL line "<<__LINE__<<": "#c"\n"; std::exit(1);} }while(0)

struct Slot { uint16_t id, n; };
static constexpr size_t kBalls = 6;

static size_t generalSize(Layout l){ return l==Layout::DiamondPearl?0xC100:l==Layout::Platinum?0xCF2C:0xF628; }

static qasynth::Bytes build(G::Kind k, Layout l, size_t pocket, const std::vector<Slot>& slots) {
    auto raw = G::save(k, k == G::Kind::HeartGoldSoulSilver ? 7 : 0);
    const auto lay = bagLayout(l)[pocket];
    for (size_t part = 0; part < 2; ++part)
        for (size_t s = 0; s < lay.slots; ++s) {
            const size_t at = part * G::kPartition + lay.offset + s * 4;
            Slot v = s < slots.size() ? slots[s] : Slot{0, 0};
            qasynth::w16(raw, at, v.id); qasynth::w16(raw, at + 2, v.n);
        }
    G::forge(raw);
    return raw;
}
static std::vector<Slot> slotsOf(std::span<const uint8_t> b, const Gen4ReadOnlySave& p, Layout l, size_t pocket) {
    const auto lay = bagLayout(l)[pocket];
    std::vector<Slot> out;
    for (size_t s = 0; s < lay.slots; ++s) {
        const size_t at = p.generalSelection().offset + lay.offset + s * 4;
        out.push_back({uint16_t(b[at] | b[at + 1] << 8), uint16_t(b[at + 2] | b[at + 3] << 8)});
    }
    return out;
}
static size_t gameVisible(const std::vector<Slot>& v){ size_t n=0; for(auto&s:v){ if(!s.id||!s.n)break; ++n;} return n; }

static void checkState(const Gen4StagedPokemonEditor& ed, Layout l, const char* gid, size_t pocket,
                       const std::vector<Slot>& expected, bool requireGameVisible = true) {
    std::string err;
    const auto fin = ed.finalizedBytes(&err);
    CHECK(!fin.empty());
    auto p = Gen4ReadOnlySave::parse(fin, l, gid, &err);       // strict reparse + CRC
    CHECK(p);
    const auto got = slotsOf(fin, *p, l, pocket);
    CHECK(got.size() == expected.size());
    for (size_t i = 0; i < got.size(); ++i) CHECK(got[i].id == expected[i].id && got[i].n == expected[i].n);
    const auto bag = decodeReadOnlyBag(*p);
    CHECK(bag);
    if (requireGameVisible) CHECK((*bag)[pocket].size() == gameVisible(got));    // app list == what the game shows
}

int main() {
    const struct { G::Kind k; Layout l; const char* gid; } games[] = {
        {G::Kind::DiamondPearl, Layout::DiamondPearl, "diamond_nds"},
        {G::Kind::Platinum, Layout::Platinum, "platinum_nds"},
        {G::Kind::HeartGoldSoulSilver, Layout::HeartGoldSoulSilver, "heartgold_nds"}};
    for (const auto& g : games) {
        const size_t cap = bagLayout(g.l)[kBalls].slots;
        auto expectPad = [&](std::vector<Slot> v){ v.resize(cap, Slot{0,0}); return v; };
        auto start = [&](std::vector<Slot> v, std::optional<Gen4StagedPokemonEditor>& ed, qasynth::Bytes& raw){
            raw = build(g.k, g.l, kBalls, v); std::string e;
            ed = Gen4StagedPokemonEditor::create(raw, g.l, g.gid, &e); CHECK(ed);
        };
        std::string err; std::optional<Gen4StagedPokemonEditor> ed; qasynth::Bytes raw;
        const std::vector<Slot> three{{1,10},{2,20},{3,30}};
        // first / middle / last
        for (size_t victim = 0; victim < 3; ++victim) {
            start(three, ed, raw); const auto src = raw;
            CHECK(ed->stageBagRemove(kBalls, victim, &err));
            std::vector<Slot> exp; for (size_t i=0;i<3;++i) if(i!=victim) exp.push_back(three[i]);
            checkState(*ed, g.l, g.gid, kBalls, expectPad(exp));
            CHECK(raw == src);                                   // source buffer untouched
            CHECK(ed->originalBytes().size()==src.size() &&
                  std::equal(src.begin(), src.end(), ed->originalBytes().begin()));
            // discard restores the original image exactly
            ed->discard(); CHECK(std::equal(src.begin(), src.end(), ed->stagedBytes().begin()));
        }
        // only occupied slot
        start({{4,5}}, ed, raw);
        CHECK(ed->stageBagRemove(kBalls, 0, &err));
        checkState(*ed, g.l, g.gid, kBalls, expectPad({}));
        // full pocket: remove first, then Add lands in the freed LAST slot with no hole
        { std::vector<Slot> full; const uint16_t ids[] = {1,2,3,4,6,7,8,9,10,11,12,13,14,15};
          for (size_t i=0;i<cap;++i) full.push_back({ids[i%14],uint16_t(i+1)});
          start(full, ed, raw);
          CHECK(ed->stageBagRemove(kBalls, 0, &err));
          auto exp = full; exp.erase(exp.begin()); exp.push_back({0,0});
          checkState(*ed, g.l, g.gid, kBalls, exp);
          // the last slot of a full pocket
          start(full, ed, raw);
          CHECK(ed->stageBagRemove(kBalls, cap - 1, &err));
          exp = full; exp.back() = {0,0};
          checkState(*ed, g.l, g.gid, kBalls, exp);
        }
        // repeated Remove/Add cycles keep the pocket packed
        start({{1,1},{2,2},{3,3}}, ed, raw);
        for (int round = 0; round < 6; ++round) {
            // rotate: remove the first stack and re-add the same ball id at the end
            const uint16_t id = uint16_t(round % 3 + 1);
            const auto cur = ed->finalizedBytes(&err); CHECK(!cur.empty());
            auto cp = Gen4ReadOnlySave::parse(cur, g.l, g.gid, &err); CHECK(cp);
            const auto vis = (*decodeReadOnlyBag(*cp))[kBalls]; CHECK(vis.size() == 3);
            size_t idx = 0; while (vis[idx].itemId != id) ++idx;
            CHECK(ed->stageBagRemove(kBalls, idx, &err));
            CHECK(ed->stageBagAdd(kBalls, id, uint16_t(round + 1), &err));
            const auto fin = ed->finalizedBytes(&err); CHECK(!fin.empty());
            auto p = Gen4ReadOnlySave::parse(fin, g.l, g.gid, &err); CHECK(p);
            const auto got = slotsOf(fin, *p, g.l, kBalls);
            CHECK(gameVisible(got) == 3);
            for (size_t s = 3; s < got.size(); ++s) CHECK(got[s].id == 0 && got[s].n == 0);
        }
        // ghost / unused slots past the game's list (id!=0,count==0; 0xFFFF filler) are not
        // listed in game and stay exactly where they are: only the listed stacks are packed.
        start({{1,10},{2,20},{9,0},{0xFFFF,0xFFFF}}, ed, raw);
        CHECK(ed->stageBagRemove(kBalls, 0, &err));
        { std::vector<Slot> exp{{2,20},{0,0},{9,0},{0xFFFF,0xFFFF}}; checkState(*ed, g.l, g.gid, kBalls, expectPad(exp)); }
        // A PRE-EXISTING interior hole (not created by us) is damage the game would hide stacks
        // behind: Remove is refused for every stack and the staged image is untouched (no silent
        // repair, no rearranging of other stacks).
        for (const auto& damaged : {std::vector<Slot>{{1,10},{0,0},{3,30}}, std::vector<Slot>{{1,10},{5,0},{3,30}},
                                    std::vector<Slot>{{0,0},{2,20}}, std::vector<Slot>{{1,10},{0xFFFF,4},{3,30}}}) {
            start(damaged, ed, raw);
            const std::vector<uint8_t> staged(ed->stagedBytes().begin(), ed->stagedBytes().end());
            for (size_t victim = 0; victim < 3; ++victim) {
                CHECK(!ed->stageBagRemove(kBalls, victim, &err));
                CHECK(std::equal(staged.begin(), staged.end(), ed->stagedBytes().begin()));
                if (victim == 0) CHECK(err.find("already has an empty or unreadable slot") != std::string::npos);
            }
            CHECK(std::equal(raw.begin(), raw.end(), staged.begin()));
        }
        // other pockets and unrelated bytes identical outside the pocket tail + checksum
        start(three, ed, raw);
        CHECK(ed->stageBagRemove(kBalls, 0, &err));
        const auto fin = ed->finalizedBytes(&err); const auto lay = bagLayout(g.l)[kBalls];
        auto p = Gen4ReadOnlySave::parse(fin, g.l, g.gid, &err); CHECK(p);
        const size_t sel = p->generalSelection().offset, from = sel + lay.offset, to = from + lay.slots*4;
        const size_t crc = sel + generalSize(g.l) - 2;
        for (size_t i = 0; i < fin.size(); ++i) {
            if (i >= from && i < to) continue;
            if (i == crc || i == crc+1) continue;
            CHECK(fin[i] == raw[i]);
        }
        // failed Remove (bad index) leaves staged bytes byte-identical
        const std::vector<uint8_t> beforeBad(ed->stagedBytes().begin(), ed->stagedBytes().end());
        CHECK(!ed->stageBagRemove(kBalls, 99, &err));
        CHECK(std::equal(beforeBad.begin(), beforeBad.end(), ed->stagedBytes().begin()));

        // ---- Add after the Remove fix (QA campaign 2 sort, integration review) -------------------
        // Native Add (pret Bag_AddItem / Bag_TryAddItem) sorts only Berries and TM/HM by item ID.
        const size_t kBerries = static_cast<size_t>(BagPocket::Berries);
        const size_t kItems = static_cast<size_t>(BagPocket::Items);
        const size_t bcap = bagLayout(g.l)[kBerries].slots;
        auto startIn = [&](size_t pocket, std::vector<Slot> v) {
            raw = build(g.k, g.l, pocket, v); std::string e;
            ed = Gen4StagedPokemonEditor::create(raw, g.l, g.gid, &e); CHECK(ed);
        };
        auto pad = [](std::vector<Slot> v, size_t n){ v.resize(n, Slot{0,0}); return v; };
        // Only slots [lo, hi) of the pocket and the General CRC differ between two images.
        auto onlyChanged = [&](const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, size_t pocket,
                               size_t lo, size_t hi) {
            auto p = Gen4ReadOnlySave::parse(b, g.l, g.gid, &err); CHECK(p);
            const size_t sel = p->generalSelection().offset, at = sel + bagLayout(g.l)[pocket].offset;
            const size_t crc = sel + generalSize(g.l) - 2;
            CHECK(a.size() == b.size());
            for (size_t i = 0; i < a.size(); ++i) {
                if (i >= at + lo * 4 && i < at + hi * 4) continue;
                if (i == crc || i == crc + 1) continue;
                CHECK(a[i] == b[i]);
            }
        };
        auto staged = [&]{ return std::vector<uint8_t>(ed->stagedBytes().begin(), ed->stagedBytes().end()); };
        // Remove (middle) then Add: the pocket stays packed and sorted; the insert moves only the
        // slots from its sorted position to the end of the list.
        startIn(kBerries, {{149,5},{151,6},{153,7}});
        CHECK(ed->stageBagRemove(kBerries, 1, &err));
        checkState(*ed, g.l, g.gid, kBerries, pad({{149,5},{153,7}}, bcap));
        auto mid = staged();
        CHECK(ed->stageBagAdd(kBerries, 150, 9, &err));
        checkState(*ed, g.l, g.gid, kBerries, pad({{149,5},{150,9},{153,7}}, bcap));
        onlyChanged(mid, staged(), kBerries, 1, 3);
        // Add then Remove (first), then Add at the front and the back.
        CHECK(ed->stageBagAdd(kBerries, 152, 1, &err));
        checkState(*ed, g.l, g.gid, kBerries, pad({{149,5},{150,9},{152,1},{153,7}}, bcap));
        CHECK(ed->stageBagRemove(kBerries, 0, &err));
        checkState(*ed, g.l, g.gid, kBerries, pad({{150,9},{152,1},{153,7}}, bcap));
        mid = staged();
        CHECK(ed->stageBagAdd(kBerries, 212, 2, &err));
        onlyChanged(mid, staged(), kBerries, 3, 4);                    // largest ID: one slot only
        CHECK(ed->stageBagAdd(kBerries, 149, 3, &err));
        checkState(*ed, g.l, g.gid, kBerries, pad({{149,3},{150,9},{152,1},{153,7},{212,2}}, bcap));
        ed->discard();
        CHECK(std::equal(raw.begin(), raw.end(), ed->stagedBytes().begin()));
        // Repeated Remove/Add on the Berry pocket stays packed and ascending.
        startIn(kBerries, {{149,1},{150,1},{151,1},{152,1}});
        for (int round = 0; round < 8; ++round) {
            const uint16_t id = uint16_t(149 + (round * 3) % 4);
            auto p = Gen4ReadOnlySave::parse(ed->finalizedBytes(&err), g.l, g.gid, &err); CHECK(p);
            const auto vis = (*decodeReadOnlyBag(*p))[kBerries];
            size_t idx = 0; while (idx < vis.size() && vis[idx].itemId != id) ++idx;
            CHECK(idx < vis.size());
            CHECK(ed->stageBagRemove(kBerries, idx, &err));
            CHECK(ed->stageBagAdd(kBerries, id, uint16_t(round + 1), &err));
            const auto fin = ed->finalizedBytes(&err);
            auto q = Gen4ReadOnlySave::parse(fin, g.l, g.gid, &err); CHECK(q);
            const auto got = slotsOf(fin, *q, g.l, kBerries);
            for (size_t s = 0; s < 4; ++s) CHECK(got[s].id == 149 + s && got[s].n != 0);
            for (size_t s = 4; s < got.size(); ++s) CHECK(got[s].id == 0 && got[s].n == 0);
        }
        // A clean Berry pocket NOT in native order (e.g. written by another tool) is not
        // re-sorted: the existing stacks keep their slots and the new stack is appended.
        startIn(kBerries, {{153,1},{149,2}});
        mid = staged();
        CHECK(ed->stageBagAdd(kBerries, 151, 3, &err));
        checkState(*ed, g.l, g.gid, kBerries, pad({{153,1},{149,2},{151,3}}, bcap));
        onlyChanged(mid, staged(), kBerries, 2, 3);
        // Items pocket is never sorted natively: append.
        startIn(kItems, {{100,1},{70,1}});
        CHECK(ed->stageBagAdd(kItems, 90, 1, &err));
        checkState(*ed, g.l, g.gid, kItems, pad({{100,1},{70,1},{90,1}}, bagLayout(g.l)[kItems].slots));
        // A ghost two slots after the list (behind a blank) is preserved in place by Add.
        startIn(kBerries, {{149,5},{0,0},{160,0}});
        CHECK(ed->stageBagAdd(kBerries, 150, 1, &err));
        checkState(*ed, g.l, g.gid, kBerries, pad({{149,5},{150,1},{160,0}}, bcap));
        // 0xFFFF/0xFFFF filler: the games list any slot whose item and quantity are both non-zero,
        // so filler right after the stacks would show in game while the app hides it (refused
        // below); filler behind a real blank slot is never reached by the game and stays as is.
        startIn(kBerries, {{149,5},{0,0},{0,0},{0xFFFF,0xFFFF}});
        CHECK(ed->stageBagAdd(kBerries, 150, 1, &err));
        checkState(*ed, g.l, g.gid, kBerries, pad({{149,5},{150,1},{0,0},{0xFFFF,0xFFFF}}, bcap));
        // ...but an Add that would put filler directly after the new last stack (where the game
        // would show it) is refused and rolled back.
        startIn(kBerries, {{149,5},{0,0},{0xFFFF,0xFFFF}});
        { const auto before = staged();
          CHECK(!ed->stageBagAdd(kBerries, 150, 1, &err));
          CHECK(err.find("would appear in game") != std::string::npos && staged() == before); }
        // Malformed pockets: Add is refused with a clear message and the staged image is
        // byte-identical (no silent repair, no stack the game would hide).
        const struct { std::vector<Slot> slots; const char* message; } badAdd[] = {
            {{{149,5},{0,0},{153,5}}, "already has an empty or unreadable slot"},    // interior hole
            {{{0,0},{153,5}}, "already has an empty or unreadable slot"},            // leading hole
            {{{149,5},{150,0},{153,5}}, "already has an empty or unreadable slot"},  // ghost inside
            {{{149,5},{0xFFFF,3},{153,5}}, "already has an empty or unreadable slot"},// 0xFFFF stack
            {{{149,5},{0xFFFF,0xFFFF}}, "already has an empty or unreadable slot"},    // listed filler
            {{{149,5},{160,0}}, "leftover native data"},                              // ghost after list
            {{{149,5},{0,7}}, "leftover native data"},                                // count, no item
        };
        for (const auto& bad : badAdd) {
            startIn(kBerries, bad.slots);
            const auto before = staged();
            CHECK(!ed->stageBagAdd(kBerries, 151, 1, &err));
            CHECK(err.find(bad.message) != std::string::npos);
            CHECK(staged() == before && std::equal(raw.begin(), raw.end(), before.begin()));
        }
        // The same damaged pockets refuse Remove too (Remove of a listed stack).
        for (const auto& bad : badAdd) {
            if (std::string(bad.message).find("leftover") != std::string::npos) continue;
            startIn(kBerries, bad.slots);
            const auto before = staged();
            CHECK(!ed->stageBagRemove(kBerries, 0, &err));
            CHECK(staged() == before);
        }
        // Full Berry pocket: Add refused byte-inert; Remove last then Add refills in order.
        { std::vector<Slot> full; for (size_t i = 0; i < bcap; ++i) full.push_back({uint16_t(149 + i), 1});
          startIn(kBerries, full);
          const auto before = staged();
          CHECK(!ed->stageBagAdd(kBerries, uint16_t(149 + bcap - 1), 1, &err) && staged() == before);
          CHECK(ed->stageBagRemove(kBerries, 0, &err));
          CHECK(ed->stageBagAdd(kBerries, 149, 4, &err));
          auto exp = full; exp[0].n = 4;
          checkState(*ed, g.l, g.gid, kBerries, exp);
        }
    }
    std::cout << "Gen IV Bag Remove packs pocket; Add sorts Berries/TM, refuses damaged pockets (DP/Pt/HGSS): PASS\n";
    return 0;
}
