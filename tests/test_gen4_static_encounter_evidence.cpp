#include "Legality/Gen4StaticEncounter.h"
#include "Legality/Gen4StaticEvolutionEvidence.h"
#include "Legality/Gen4ReleaseEvidence.h"

#include <cassert>
#include <iostream>
#include <string_view>

int main() {
    using namespace Legality::Gen4Static;

    assert(countForGame("diamond_nds") == 24);
    assert(countForGame("pearl_nds") == 24);
    assert(countForGame("platinum_nds") == 35);
    assert(countForGame("heartgold_nds") == 57);
    assert(countForGame("soulsilver_nds") == 57);

    const auto match = [](std::string_view game, uint16_t species,
                          uint16_t location, uint8_t level, uint8_t form,
                          uint16_t eggLocation, uint8_t ball = 4,
                          uint8_t gender = 0, uint8_t nature = 0,
                          bool shiny = false, bool fateful = false) {
        return matches(
            game, species, location, level, form, eggLocation, ball,
            gender, nature, shiny, fateful);
    };

    // Diamond Dialga / Pearl Palkia are version-specific static encounters.
    assert(match("diamond_nds", 483, 51, 47, 0, 0));
    assert(!match("pearl_nds", 483, 51, 47, 0, 0));
    assert(match("pearl_nds", 484, 51, 47, 0, 0));
    const auto* dialga = findMatch(
        "diamond_nds", 483, 51, 47, 0, 0, 4, 0, 0, false, false);
    assert(dialga != nullptr);
    assert(pidCategoryForRow(*dialga) == PidCategory::Method1OrCuteCharm);

    // Form-changeable statics retain provenance after a legitimate same-species
    // form change. Distortion World Giratina may later be Altered, and Rotom may
    // change appliances. Spiky-eared Pichu remains fixed and is tested below.
    assert(match("platinum_nds", 487, 117, 47, 1, 0));
    assert(match("platinum_nds", 487, 117, 47, 0, 0));
    assert(match("platinum_nds", 479, 70, 20, 1, 0));
    static_assert(Legality::Gen4Form::formChangeableSpecies(487));
    static_assert(Legality::Gen4Form::formChangeableSpecies(479));
    static_assert(!Legality::Gen4Form::formChangeableSpecies(172));

    // D/P Riolu egg requires its exact egg-location evidence and fixed Poke Ball.
    assert(match("diamond_nds", 447, 40, 0, 0, 2010, 4));
    assert(!match("diamond_nds", 447, 40, 0, 0, 2010, 2));
    assert(!match("diamond_nds", 447, 40, 0, 0, 0, 4));
    assert(!match("diamond_nds", 447, 40, 1, 0, 2010, 4));

    // Diamond gift Eevee is fixed to a Poke Ball.
    assert(match("diamond_nds", 133, 10, 5, 0, 0, 4));
    assert(!match("diamond_nds", 133, 10, 5, 0, 0, 1));

    // Pinned EncounterStatic4 receives an EvoCriteria, so a later evolution does
    // not erase native static/gift provenance. Keep this positive-only and retain
    // the original source species/row for downstream PID evidence.
    const auto directEevee = Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 133, 10, 5, 0, 0, 4, 0, 0, false, false);
    assert(directEevee.matched());
    assert(!directEevee.evolved);
    assert(!directEevee.hatchedGiftEgg);
    assert(directEevee.sourceSpecies == 133);
    assert(directEevee.row != nullptr);
    assert(species(*directEevee.row) == 133);

    const auto evolvedVaporeon = Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 134, 10, 5, 0, 0, 4, 0, 0, false, false);
    assert(evolvedVaporeon.matched());
    assert(evolvedVaporeon.evolved);
    assert(!evolvedVaporeon.hatchedGiftEgg);
    assert(evolvedVaporeon.sourceSpecies == 133);
    assert(evolvedVaporeon.row != nullptr);
    assert(species(*evolvedVaporeon.row) == 133);
    assert(eggLocation(*evolvedVaporeon.row) == 0);
    assert(form(*evolvedVaporeon.row) == 0);

    // Persistent source fields still constrain evolved positive provenance.
    assert(!Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 134, 11, 5, 0, 0, 4, 0, 0, false, false).matched());
    assert(!Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 134, 10, 6, 0, 0, 4, 0, 0, false, false).matched());
    assert(!Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 134, 10, 5, 0, 0, 1, 0, 0, false, false).matched());
    assert(!Legality::Gen4StaticEvolution::matchEvolutionLine(
        "heartgold_nds", 134, 10, 5, 0, 0, 4, 0, 0, false, false).matched());

    // Static-gift egg descendants preserve the source egg location after hatching.
    // EncounterStatic4 does not compare a hatched PK4's met location to the gift
    // row; met level remains 0. A traded egg may instead preserve Link Trade 2002.
    const auto directRioluEgg = Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 447, 40, 0, 0, 2010, 4, 0, 0, false, false);
    assert(directRioluEgg.matched());
    assert(!directRioluEgg.evolved);
    assert(!directRioluEgg.hatchedGiftEgg);
    assert(directRioluEgg.sourceSpecies == 447);

    const auto evolvedLucario = Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 448, 4, 0, 0, 2010, 4, 0, 0, false, false);
    assert(evolvedLucario.matched());
    assert(evolvedLucario.evolved);
    assert(evolvedLucario.hatchedGiftEgg);
    assert(evolvedLucario.sourceSpecies == 447);
    assert(evolvedLucario.row != nullptr);
    assert(species(*evolvedLucario.row) == 447);
    assert(eggLocation(*evolvedLucario.row) == 2010);

    const auto tradedEggLucario = Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 448, 4, 0, 0, 2002, 4, 0, 0, false, false);
    assert(tradedEggLucario.matched());
    assert(tradedEggLucario.hatchedGiftEgg);
    assert(tradedEggLucario.sourceSpecies == 447);

    // Wrong persisted source fields do not become positive egg provenance.
    assert(!Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 448, 4, 0, 0, 0, 4, 0, 0, false, false).matched());
    assert(!Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 448, 4, 1, 0, 2010, 4, 0, 0, false, false).matched());
    assert(!Legality::Gen4StaticEvolution::matchEvolutionLine(
        "diamond_nds", 448, 4, 0, 0, 2010, 2, 0, 0, false, false).matched());

    // Platinum Shaymin's released static template is fateful. The encounter itself
    // was never released for Korean-language games.
    assert(match("platinum_nds", 492, 63, 30, 0, 0, 4, 0, 0, false, true));
    assert(!match("platinum_nds", 492, 63, 30, 0, 0, 4, 0, 0, false, false));
    static_assert(Legality::Gen4Release::staticEncounterUnreleased(492, 8));
    static_assert(!Legality::Gen4Release::staticEncounterUnreleased(492, 2));
    static_assert(!Legality::Gen4Release::staticEncounterUnreleased(491, 8));

    // Lake of Rage Gyarados is forced shiny and therefore uses Chain Shiny PID evidence.
    assert(match("heartgold_nds", 130, 135, 30, 0, 0, 4, 0, 0, true, false));
    assert(match("soulsilver_nds", 130, 135, 30, 0, 0, 4, 0, 0, true, false));
    assert(!match("heartgold_nds", 130, 135, 30, 0, 0, 4, 0, 0, false, false));
    const auto* redGyarados = findMatch(
        "heartgold_nds", 130, 135, 30, 0, 0, 4, 0, 0, true, false);
    assert(redGyarados != nullptr);
    assert(pidCategoryForRow(*redGyarados) == PidCategory::ChainShiny);

    // HG/SS Spiky-eared Pichu is fixed female, Naughty, non-shiny, form 1 and uses
    // the Pokewalker PID formula even though it is an in-game static encounter.
    assert(match("heartgold_nds", 172, 214, 30, 1, 0, 4, 1, 4, false, false));
    assert(!match("heartgold_nds", 172, 214, 30, 1, 0, 4, 0, 4, false, false));
    assert(!match("heartgold_nds", 172, 214, 30, 1, 0, 4, 1, 3, false, false));
    assert(!match("heartgold_nds", 172, 214, 30, 1, 0, 4, 1, 4, true, false));
    const auto* spikyPichu = findMatch(
        "heartgold_nds", 172, 214, 30, 1, 0, 4, 1, 4, false, false);
    assert(spikyPichu != nullptr);
    assert(pidCategoryForRow(*spikyPichu) == PidCategory::Pokewalker);

    // Roamers use a permitted route set rather than one fixed met location.
    assert(match("platinum_nds", 481, 20, 50, 0, 0));
    assert(!match("platinum_nds", 481, 100, 50, 0, 0));
    assert(match("heartgold_nds", 243, 180, 40, 0, 0));

    // Version-exclusive cover legends.
    assert(match("heartgold_nds", 250, 205, 45, 0, 0));
    assert(match("soulsilver_nds", 249, 218, 45, 0, 0));

    assert(hasSpecies("platinum_nds", 492));
    assert(!hasSpecies("unknown", 492));
    assert(countForGame("unknown") == 0);

    std::cout << "Gen IV static/gift encounter evidence: PASS\n";
}
