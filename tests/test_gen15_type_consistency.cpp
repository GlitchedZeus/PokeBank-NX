// Round 16 (owner: Gen V Ninjask shown as Ghost/Flying; "audit all Gen I-V types").
// Cross-generation consistency of the native type tables the editors draw from, in the CANONICAL
// sprite id space SpriteManager::getTypeSprite() takes (Normal 0 ... Bug 6, Ghost 7, Steel 8,
// Fire 9 ... Dark 16). Type changes between Generations I and V are rare and documented
// (Bulbapedia "Type" / PKHeX PersonalTable): Gen II added Steel/Dark (Magnemite, Magneton are
// Electric/Steel from Gen II). No species changed type between Gen II and Gen V (Fairy is Gen VI).
// Every other species must agree across the Gen I, II, III, IV (HGSS) and V (BW, B2W2) tables.
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen5/Gen5PersonalTypes.h"
#include "Pokemon/PersonalInfo4HGSS.h"
#include "Pokemon/PersonalInfoTable.h"
#include "UI/ClassicGameContext.h"
#include "UI/SharedSpeciesPicker.h"

#include <array>
#include <iostream>
#include <set>
#include <string>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; if (failures < 40) std::cerr << "FAIL: " << what << '\n'; }
}
using Pair = std::array<int, 2>;
Pair norm(Pair p) { if (p[1] == p[0] || p[1] == 255) p[1] = -1; return p; }   // single type: second == first or 255
std::string str(Pair p) { return std::to_string(p[0]) + "/" + std::to_string(p[1]); }
Pair classic(uint8_t a, uint8_t b) {
    using PokeBank::UIModel::ClassicGameContext::normalizedTypeSpriteId;
    return norm({normalizedTypeSpriteId(a), normalizedTypeSpriteId(b)});
}
}

int main() {
    const std::set<int> gen1ToGen2Changes = {81, 82};   // Magnemite, Magneton: + Steel in Gen II
    for (int s = 1; s <= 649; ++s) {
        const auto bw = PokeVault::Integration::Gen5::personalTypes5(static_cast<uint32_t>(s), 0, false);
        const auto b2 = PokeVault::Integration::Gen5::personalTypes5(static_cast<uint32_t>(s), 0, true);
        const Pair v5 = norm({bw[0], bw[1]}), v5b = norm({b2[0], b2[1]});
        const std::string sp = "species " + std::to_string(s);
        check(v5[0] >= 0 && v5[0] <= 16 && v5[1] <= 16, sp + ": BW types in the canonical Gen I-V range (" + str(v5) + ")");
        check(v5 == v5b, sp + ": BW and B2W2 agree (" + str(v5) + " vs " + str(v5b) + ")");
        // The species-picker preview converts to classic ids and ClassicTypeBadges converts back:
        // the round trip must be the identity for every Gen V type.
        for (int i = 0; i < 2; ++i) {
            const int back = PokeBank::UIModel::ClassicGameContext::normalizedTypeSpriteId(
                UI::SharedSpeciesPicker::canonicalToClassicType(bw[static_cast<size_t>(i)]));
            check(back == bw[static_cast<size_t>(i)], sp + ": canonical->classic->canonical round trip");
        }
        if (s <= 493) {
            const auto& g4 = Pokemon::getPersonalInfo4HGSS(static_cast<uint16_t>(s), 0);
            check(norm({g4.type1, g4.type2}) == v5, sp + ": Gen IV HGSS " + str(norm({g4.type1, g4.type2})) + " == Gen V " + str(v5));
        }
        if (s <= 386) {
            const auto& g3 = Pokemon::getPersonalInfoG3(static_cast<uint16_t>(s));
            check(norm({g3.type1, g3.type2}) == v5, sp + ": Gen III " + str(norm({g3.type1, g3.type2})) + " == Gen V " + str(v5));
        }
        if (s <= 251) {
            const auto* g2 = PokeVault::Integration::Gen2::personalRecord(static_cast<uint16_t>(s));
            check(g2 && classic(g2->rawType1, g2->rawType2) == v5,
                  sp + ": Gen II " + (g2 ? str(classic(g2->rawType1, g2->rawType2)) : "missing") + " == Gen V " + str(v5));
        }
        if (s <= 151) {
            const auto g1 = PokeVault::Integration::Gen1::StagedPokemonEditor::personalTypes(static_cast<uint16_t>(s));
            const Pair t1 = classic(g1[0], g1[1]);
            if (gen1ToGen2Changes.count(s)) check(t1 == Pair{12, -1} && v5 == Pair{12, 8}, sp + ": Gen I Electric, later Electric/Steel");
            else check(t1 == v5, sp + ": Gen I " + str(t1) + " == Gen V " + str(v5));
        }
    }
    // The owner's report and its neighbours, pinned by name.
    const auto t = [](int s) { const auto x = PokeVault::Integration::Gen5::personalTypes5(s, 0, false); return norm({x[0], x[1]}); };
    check(t(291) == Pair{6, 2}, "Ninjask is Bug/Flying");
    check(t(290) == Pair{6, 4}, "Nincada is Bug/Ground");
    check(t(292) == Pair{6, 7}, "Shedinja is Bug/Ghost");
    check(t(22) == Pair{0, 2}, "Fearow is Normal/Flying");
    check(t(24) == Pair{3, -1}, "Arbok is Poison");
    check(t(94) == Pair{7, 3}, "Gengar is Ghost/Poison");
    if (failures) { std::cerr << failures << " of " << checks << " check(s) failed\n"; return 1; }
    std::cout << "Gen I-V type tables consistent (canonical ids, BW/B2W2/HGSS/Gen III/Gen II/Gen I; "
              << checks << " checks): PASS\n";
    return 0;
}
