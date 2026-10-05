#include "Legality/Gen4StaticEncounter.h"
#include "Legality/Gen4StaticEvolutionEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Static;

    // Preserve the old exact-species primitive for callers that require it.
    const auto* directEevee = findDirectMatch(
        "diamond_nds", 133, 10, 5, 0, 0, 4, 0, 0, false, false);
    assert(directEevee != nullptr);
    assert(species(*directEevee) == 133);
    assert(findDirectMatch(
        "diamond_nds", 134, 10, 5, 0, 0, 4, 0, 0, false, false) == nullptr);

    // The central findMatch API used by Legality.cpp must now preserve the
    // original static source row after evolution.
    const auto* centralVaporeon = findMatch(
        "diamond_nds", 134, 10, 5, 0, 0, 4, 0, 0, false, false);
    assert(centralVaporeon != nullptr);
    assert(species(*centralVaporeon) == 133);
    assert(pidCategoryForRow(*centralVaporeon) == PidCategory::Method1OrCuteCharm);

    const auto vaporeonEvidence = Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 134, 10, 5, 0, 0, 4, 0, 0, false, false);
    assert(vaporeonEvidence.matched());
    assert(vaporeonEvidence.evolved);
    assert(!vaporeonEvidence.hatchedGiftEgg);
    assert(vaporeonEvidence.sourceSpecies == 133);
    assert(vaporeonEvidence.row == centralVaporeon);

    // Hatched static-gift descendants also flow through the same central API.
    assert(findDirectMatch(
        "diamond_nds", 448, 4, 0, 0, 2010, 4, 0, 0, false, false) == nullptr);
    const auto* centralLucario = findMatch(
        "diamond_nds", 448, 4, 0, 0, 2010, 4, 0, 0, false, false);
    assert(centralLucario != nullptr);
    assert(species(*centralLucario) == 447);
    assert(eggLocation(*centralLucario) == 2010);

    const auto* tradedEggLucario = findMatch(
        "diamond_nds", 448, 4, 0, 0, 2002, 4, 0, 0, false, false);
    assert(tradedEggLucario != nullptr);
    assert(species(*tradedEggLucario) == 447);

    // Missing persisted egg evidence remains unproven rather than being promoted.
    assert(findMatch(
        "diamond_nds", 448, 4, 0, 0, 0, 4, 0, 0, false, false) == nullptr);
    assert(findMatch(
        "diamond_nds", 448, 4, 1, 0, 2010, 4, 0, 0, false, false) == nullptr);

    std::cout << "Gen IV central static evolution matcher: PASS\n";
}
