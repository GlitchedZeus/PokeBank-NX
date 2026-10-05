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

    // Pinned EncounterStatic4 permits Link Trade 2002 only after the static gift
    // egg has hatched. With explicit state, a still-Riolu traded egg can therefore
    // be proven after hatching without accepting the same fields while unhatched.
    assert(findDirectMatch(
        "diamond_nds", 447, 4, 0, 0, 2002, 4, 0, 0, false, false) == nullptr);
    const auto* hatchedTradedRiolu = findDirectMatchWithEggState(
        "diamond_nds", 447, 4, 0, 0, 2002, 4, 0, 0, false, false, false);
    assert(hatchedTradedRiolu != nullptr);
    assert(species(*hatchedTradedRiolu) == 447);
    assert(findDirectMatchWithEggState(
        "diamond_nds", 447, 4, 0, 0, 2002, 4, 0, 0, false, false, true) == nullptr);

    const auto sameSpeciesHatched =
        Legality::Gen4StaticEvolution::matchEvolutionLineWithEggState(
            "diamond_nds", 447, 4, 0, 0, 2002, 4, 0, 0, false, false, false);
    assert(sameSpeciesHatched.matched());
    assert(!sameSpeciesHatched.evolved);
    assert(sameSpeciesHatched.hatchedGiftEgg);
    assert(sameSpeciesHatched.sourceSpecies == 447);

    const auto sameSpeciesUnhatched =
        Legality::Gen4StaticEvolution::matchEvolutionLineWithEggState(
            "diamond_nds", 447, 4, 0, 0, 2002, 4, 0, 0, false, false, true);
    assert(!sameSpeciesUnhatched.matched());

    // An unhatched PK4 cannot already be an evolved descendant of the source egg.
    assert(findMatchWithEggState(
        "diamond_nds", 448, 4, 0, 0, 2010, 4, 0, 0, false, false, true) == nullptr);
    const auto* statefulLucario = findMatchWithEggState(
        "diamond_nds", 448, 4, 0, 0, 2010, 4, 0, 0, false, false, false);
    assert(statefulLucario != nullptr);
    assert(species(*statefulLucario) == 447);

    // A same-species hatched gift egg that kept the original egg location remains
    // direct provenance and is explicitly marked as a hatched gift history.
    const auto sameSpeciesOriginalLocation =
        Legality::Gen4StaticEvolution::matchEvolutionLineWithEggState(
            "diamond_nds", 447, 4, 0, 0, 2010, 4, 0, 0, false, false, false);
    assert(sameSpeciesOriginalLocation.matched());
    assert(!sameSpeciesOriginalLocation.evolved);
    assert(sameSpeciesOriginalLocation.hatchedGiftEgg);

    // Missing persisted egg evidence remains unproven rather than being promoted.
    assert(findMatch(
        "diamond_nds", 448, 4, 0, 0, 0, 4, 0, 0, false, false) == nullptr);
    assert(findMatch(
        "diamond_nds", 448, 4, 1, 0, 2010, 4, 0, 0, false, false) == nullptr);

    std::cout << "Gen IV central static evolution matcher: PASS\n";
}
