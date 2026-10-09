#include "Legality/Gen34EggMoveEvidence.h"

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

    std::cout << "Gen III/IV egg-move legality evidence: PASS\n";
}
