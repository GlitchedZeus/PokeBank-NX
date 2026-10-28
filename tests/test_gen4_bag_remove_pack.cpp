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
    }
    std::cout << "Gen IV Bag Remove packs pocket (DP/Pt/HGSS): PASS\n";
    return 0;
}
