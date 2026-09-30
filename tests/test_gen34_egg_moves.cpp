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
    static_assert(isEggMove("ruby_gba", 1, 80));   // Petal Dance
    static_assert(isEggMove("firered_gba", 1, 345)); // Magical Leaf
    static_assert(!isEggMove("ruby_gba", 1, 57)); // Surf is not egg evidence.

    // Gen IV DP egg pool contains the inherited Gen III set plus Gen IV additions.
    static_assert(isEggMove("diamond_nds", 1, 80));
    static_assert(isEggMove("diamond_nds", 1, 437)); // Leaf Storm
    static_assert(!isEggMove("diamond_nds", 1, 57));

    static_assert(speciesHasEggMoves("platinum_nds", 1));
    static_assert(!speciesHasEggMoves("unknown", 1));
    static_assert(!isEggMove("platinum_nds", 494, 80));
    static_assert(!isEggMove("platinum_nds", 1, 468));

    std::cout << "Gen III/IV egg-move legality evidence: PASS\n";
}
