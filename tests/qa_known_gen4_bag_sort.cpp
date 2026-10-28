// OPEN-ISSUE reproduction (NOT in any default gate; fails while open): Gen IV Bag Add must keep the
// Berries and TM/HM pockets sorted ascending by item id with empties last. Native source:
// pret/pokeplatinum bag.c Bag_TryAddItem -> Pocket_Sort and pret/pokeheartgold bag.c Bag_AddItem ->
// SortPocket, both only for POCKET_BERRIES and POCKET_TMHMS.
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "fuzz_synthetic_saves.h"
#include <iostream>
using namespace PokeVault::Integration::Gen4;
namespace G = qasynth::gen4;
int main() {
    const struct { G::Kind k; Layout l; const char* gid; } games[] = {
        {G::Kind::DiamondPearl, Layout::DiamondPearl, "diamond_nds"},
        {G::Kind::Platinum, Layout::Platinum, "platinum_nds"},
        {G::Kind::HeartGoldSoulSilver, Layout::HeartGoldSoulSilver, "heartgold_nds"}};
    int failures = 0;
    for (const auto& g : games) {
        auto raw = G::save(g.k, g.k == G::Kind::HeartGoldSoulSilver ? 7 : 0);
        std::string err;
        auto ed = Gen4StagedPokemonEditor::create(raw, g.l, g.gid, &err);
        if (!ed) { std::cerr << "create failed " << err << "\n"; return 2; }
        const struct { size_t pocket; uint16_t first, second; const char* name; } cases[] = {
            {5, 160, 150, "Berries"}, {2, 340, 328, "TM/HM"}};
        for (const auto& c : cases) {
            if (!ed->stageBagAdd(c.pocket, c.first, 1, &err) || !ed->stageBagAdd(c.pocket, c.second, 1, &err)) {
                std::cerr << "add failed " << err << "\n"; return 2; }
            auto p = Gen4ReadOnlySave::parse(ed->finalizedBytes(&err), g.l, g.gid, &err);
            const auto bag = decodeReadOnlyBag(*p);
            const auto& v = (*bag)[c.pocket];
            const bool sorted = v.size() == 2 && v[0].itemId == c.second && v[1].itemId == c.first;
            std::cout << g.gid << " " << c.name << " after Add(" << c.first << "), Add(" << c.second
                      << "): order = " << v[0].itemId << "," << v[1].itemId
                      << (sorted ? "  (native sorted)\n" : "  <-- appended, native game would sort\n");
            if (!sorted) ++failures;
        }
    }
    return failures ? 1 : 0;
}
