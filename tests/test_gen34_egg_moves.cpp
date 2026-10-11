#include "Legality/Gen34EggMoveEvidence.h"
#include "Legality/Gen4EggMoveFamilyEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen34EggMove;

    static_assert(groupForId("ruby_gba") == Group::Gen3);
    static_assert(groupForId("leafgreen_gba") == Group::Gen3);
    static_assert(groupForId("diamond_nds") == Group::DiamondPearl);
    static_assert(groupForId("platinum_nds") == Group::Platinum);
    static_assert(groupForId("heartgold_nds") == Group::HeartGoldSoulSilver);
    static_assert(groupForId("unknown") == Group::None);

    // Bulbasaur Gen III egg moves from PKHeX eggmove_rs.pkl.
    assert(isEggMove("ruby_gba", 1, 80));      // Petal Dance
    assert(isEggMove("firered_gba", 1, 345)); // Magical Leaf
    assert(!isEggMove("ruby_gba", 1, 57));    // Surf is not egg evidence.

    static_assert(preEvolution("ruby_gba", 2) == 1); // Ivysaur -> Bulbasaur
    static_assert(preEvolution("ruby_gba", 3) == 2); // Venusaur -> Ivysaur
    const auto ivysaurPetal = classify("ruby_gba", 2, 80);
    assert(ivysaurPetal.evidence == MoveEvidence::PreEvolutionEggMove);
    assert(ivysaurPetal.sourceSpecies == 1);
    const auto venusaurPetal = classify("ruby_gba", 3, 80);
    assert(venusaurPetal.evidence == MoveEvidence::PreEvolutionEggMove);
    assert(venusaurPetal.sourceSpecies == 1);

    // Gen IV DP egg pool contains the inherited Gen III set plus Gen IV additions.
    assert(isEggMove("diamond_nds", 1, 80));
    assert(isEggMove("diamond_nds", 1, 437)); // Leaf Storm
    assert(!isEggMove("diamond_nds", 1, 57));

    // HG/SS added egg options that D/P/Pt did not have. Stored origin identity,
    // not the current Gen IV save container, must choose this evidence group.
    assert(!isEggMove("diamond_nds", 1, 124));
    assert(!isEggMove("platinum_nds", 1, 124));
    assert(isEggMove("heartgold_nds", 1, 124));

    const auto ivysaurLeafStorm = classify("diamond_nds", 2, 437);
    assert(ivysaurLeafStorm.evidence == MoveEvidence::PreEvolutionEggMove);
    assert(ivysaurLeafStorm.sourceSpecies == 1);

    assert(speciesHasEggMoves("platinum_nds", 1));
    assert(!speciesHasEggMoves("unknown", 1));
    assert(!isEggMove("platinum_nds", 494, 80));
    assert(!isEggMove("platinum_nds", 1, 468));

    // Gen IV egg-move family source cannot be inferred from the current
    // physical save. HGSS Bulbasaur Sludge #124 was not a D/P/Pt
    // egg move; Petal Dance #80 is shared. All conclusions are INFO-only.
    namespace Families=Legality::Gen4EggMoveFamily;
    const auto dp=Families::classify("diamond_nds",1,{80,124,0,0});
    assert(dp.hasKnownOrigin && dp.storedMoves==2);
    assert(dp.originFamilyCandidates==1);
    assert(dp.otherFamilyOnlyCandidates==1);
    assert(dp.unmodeled==0);
    const auto hg=Families::classify("heartgold_nds",1,{80,124,0,0});
    assert(hg.originFamilyCandidates==2 && hg.otherFamilyOnlyCandidates==0);
    const auto missing=Families::classify("unknown",1,{80,124,0,0});
    assert(!missing.hasKnownOrigin && missing.storedMoves==0);
    const auto eventUnknown=Families::classify("diamond_nds",1,{57,468,0,0});
    assert(eventUnknown.unmodeled==2 && eventUnknown.originFamilyCandidates==0);
    const auto evolved=Families::classify("diamond_nds",2,{80,124,0,0});
    assert(evolved.originFamilyCandidates==1 && evolved.otherFamilyOnlyCandidates==1);

    std::cout << "Gen III/IV egg-move legality evidence: PASS\n";
}
