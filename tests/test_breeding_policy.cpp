// Round 13 owner decision 5 ("Add proper egg-group and breeding compatibility data. No generic egg
// histories presented as verified."): the generated egg-group table (include/Data/EggGroups.h,
// tools/gen_egg_groups.py) and the breeding / egg-eligibility policy built on it
// (include/Integration/Encounter/BreedingPolicy.h).
#include "Data/EggGroups.h"
#include "Integration/Encounter/BreedingPolicy.h"

#include <cassert>
#include <cstdio>
#include <initializer_list>

namespace Data = PokeVault::Data::EggGroups;
namespace B = PokeVault::Integration::BreedingPolicy;

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL %s\n", what); ++failures; }
}
} // namespace

int main() {
    // ---- table spot checks (PKHeX personal tables; pret pokecrystal / pokeemerald agree) ----
    check(Data::maxSpecies(2) == 251 && Data::maxSpecies(3) == 386 && Data::maxSpecies(4) == 493 &&
          Data::maxSpecies(5) == 649, "dex sizes");
    for (const uint8_t gen : {2, 3, 4, 5}) {
        const auto* t = Data::table(gen);
        check(t[1].egg1 == 1 && t[1].egg2 == 7 && t[1].gender == 31, "Bulbasaur Monster/Grass 87.5% male");
        check(t[132].egg1 == 13 && t[132].egg2 == 13 && t[132].gender == 255, "Ditto group, genderless");
        check(t[172].egg1 == 15 && t[172].prevo == 0, "Pichu Undiscovered baby, no pre-evolution");
        check(t[25].prevo == 172 && t[26].prevo == 25, "Pikachu <- Pichu, Raichu <- Pikachu");
        check(t[30].egg1 == 15 && t[30].gender == 254 && t[30].prevo == 29, "Nidorina Undiscovered, female");
        check(t[29].egg1 == 1 && t[29].egg2 == 5, "Nidoran F Monster/Field");
        check(t[150].egg1 == 15 && t[150].gender == 255, "Mewtwo Undiscovered genderless");
        check(t[201].egg1 == 15, "Unown Undiscovered");
        check(t[183].prevo == (gen >= 3 ? 298 : 0), "Marill <- Azurill only from Gen III");
        check(t[202].prevo == (gen >= 3 ? 360 : 0), "Wobbuffet <- Wynaut only from Gen III");
        check(t[106].prevo == 236 && t[107].prevo == 236, "Hitmonlee / Hitmonchan <- Tyrogue");
        check(t[106].gender == 0 && t[124].gender == 254, "Hitmonlee male only, Jynx female only");
        check(t[143].prevo == (gen >= 4 ? 446 : 0), "Snorlax <- Munchlax only from Gen IV");
    }
    check(Data::kGen3[298].egg1 == 15 && Data::kGen3[298].gender == 191, "Azurill Undiscovered, 75% female");
    check(Data::kGen4[489].egg1 == 2 && Data::kGen4[489].egg2 == 6 && Data::kGen4[489].gender == 255,
          "Phione Water 1 / Fairy genderless");
    check(Data::kGen4[490].egg1 == 2 && Data::kGen4[490].egg2 == 6, "Manaphy Water 1 / Fairy");
    check(Data::kGen4[447].egg1 == 15 && Data::kGen4[448].prevo == 447, "Riolu baby, Lucario <- Riolu");
    check(Data::kGen5[649].egg1 == 15 && Data::kGen5[495].egg1 == 5 && Data::kGen5[495].egg2 == 7,
          "Genesect Undiscovered, Snivy Field / Grass");

    // ---- compatibility ----
    check(B::compatible(3, 25, 132) && B::compatible(3, 132, 25), "Pikachu + Ditto");
    check(!B::compatible(3, 132, 132), "Ditto + Ditto never");
    check(B::compatible(3, 81, 132) && !B::compatible(3, 81, 81), "Magnemite (genderless) only with Ditto");
    check(B::compatible(3, 106, 124) && !B::compatible(3, 106, 106), "Hitmonlee x Jynx (Human-Like), not male x male");
    check(!B::compatible(3, 150, 132) && !B::compatible(2, 30, 32), "Undiscovered never breeds");
    check(B::compatible(4, 1, 7), "Bulbasaur x Squirtle share Monster");
    check(!B::compatible(4, 1, 25), "Bulbasaur x Pikachu share no egg group");
    check(B::compatible(4, 490, 132) && !B::compatible(4, 490, 489), "Manaphy only with Ditto");

    // ---- egg species / hatchability ----
    check(B::eggSpecies(2, 26, false) == 172 && B::eggSpecies(3, 25, false) == 172, "Raichu / Pikachu eggs hatch as Pichu");
    check(B::eggSpecies(2, 184, false) == 183, "Gen II Azumarill eggs hatch as Marill (no Azurill)");
    check(B::eggSpecies(3, 184, false) == 183 && B::eggSpecies(3, 184, true) == 298, "Gen III Sea Incense -> Azurill");
    check(B::eggSpecies(3, 202, false) == 202 && B::eggSpecies(3, 202, true) == 360, "Gen III Lax Incense -> Wynaut");
    check(B::eggSpecies(3, 143, false) == 143 && B::eggSpecies(4, 143, false) == 143 && B::eggSpecies(4, 143, true) == 446,
          "Snorlax: Munchlax only Gen IV with Full Incense");
    check(B::eggSpecies(4, 242, false) == 113 && B::eggSpecies(4, 242, true) == 440, "Blissey -> Chansey / Happiny (Luck Incense)");
    check(B::eggSpecies(4, 407, false) == 315 && B::eggSpecies(4, 407, true) == 406, "Roserade -> Roselia / Budew (Rose Incense)");
    check(B::eggSpecies(4, 448, false) == 447, "Lucario eggs hatch as Riolu (no incense)");
    check(B::eggSpecies(4, 490, false) == 489 && B::eggSpecies(5, 489, false) == 489, "Manaphy / Phione eggs hatch as Phione");
    check(B::eggSpecies(3, 30, false) == 0 && B::eggSpecies(3, 132, false) == 0, "Nidorina / Ditto parent nothing");
    check(B::eggSpecies(3, 34, false) == 32 && B::eggSpecies(3, 29, false) == 29, "Nidoking -> Nidoran M");

    for (const uint8_t gen : {2, 3, 4, 5}) {
        check(B::hatchable(gen, 172) && !B::hatchable(gen, 25) && !B::hatchable(gen, 26), "Pichu hatchable, Pikachu not");
        check(!B::hatchable(gen, 132), "Ditto never hatchable");
        check(!B::hatchable(gen, 150) && !B::hatchable(gen, 151) && !B::hatchable(gen, 249), "legendaries never hatchable");
        check(!B::hatchable(gen, 201), "Unown never hatchable");
        check(!B::hatchable(gen, 30) && !B::hatchable(gen, 31), "Nidorina / Nidoqueen not hatchable");
        check(B::hatchable(gen, 29) && B::hatchable(gen, 32), "Nidoran F and M hatchable");
        check(B::hatchable(gen, 236) && !B::hatchable(gen, 106), "Tyrogue hatchable, Hitmonlee not");
        check(B::hatchable(gen, 1) && !B::hatchable(gen, 2), "Bulbasaur hatchable, Ivysaur not");
        check(B::hatchable(gen, 183), "Marill hatchable every generation (no incense)");
        check(B::hatchable(gen, 202), "Wobbuffet hatchable every generation (no incense)");
        check(B::hatchLevel(gen) == (gen <= 3 ? 5 : 1), "hatch level");
    }
    check(!B::hatchable(2, 298) && B::hatchable(3, 298) && B::hatchable(3, 360), "Azurill / Wynaut hatchable from Gen III");
    check(B::hatchable(3, 113) && B::hatchable(4, 113) && B::hatchable(4, 440), "Chansey hatchable; Happiny Gen IV");
    check(B::hatchable(3, 313) && B::hatchable(3, 314), "Volbeat / Illumise both hatchable");
    check(B::hatchable(4, 489) && !B::hatchable(4, 490) && B::hatchable(5, 489) && !B::hatchable(5, 490),
          "Phione hatchable, Manaphy not (Gen IV / V)");
    check(B::hatchable(4, 447) && !B::hatchable(3, 447), "Riolu Gen IV only");
    check(!B::hatchable(5, 494) && !B::hatchable(5, 649) && B::hatchable(5, 495), "Victini / Genesect no, Snivy yes");
    check(!B::hatchable(3, 387) && !B::hatchable(2, 252), "species outside the generation");

    // PKHeX 6501f0ab Breeding.IsAbleToHatchFromEgg: every listed species is never hatchable here.
    const uint16_t neverHatch[] = {132, 144, 145, 146, 150, 151, 201, 243, 244, 245, 249, 250, 251, 377, 378, 379, 380,
                             381, 382, 383, 384, 385, 386, 480, 481, 482, 483, 484, 485, 486, 487, 488, 490, 491,
                             492, 493, 494, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649};
    // Exhaustive agreement with PKHeX's devolve-then-list rule (EncounterGenerator4 TryGetEgg): a
    // species hatches exactly when it is a first stage (or an incense split stage) not on that list.
    // Here it is derived independently from egg groups and genders.
    for (const uint8_t gen : {2, 3, 4, 5}) {
        int count = 0;
        for (uint16_t s = 1; s <= Data::maxSpecies(gen); ++s) {
            bool listed = false;
            for (const auto never : neverHatch) listed = listed || never == s;
            const bool expected = !listed && (Data::table(gen)[s].prevo == 0 || B::incenseStage(gen, s));
            if (B::hatchable(gen, s) != expected) {
                std::printf("FAIL Gen %u species %u hatchable=%d expected=%d\n", gen, s, B::hatchable(gen, s), expected);
                ++failures;
            }
            count += B::hatchable(gen, s);
        }
        std::printf("hatchable species: Gen %u %d\n", gen, count);
    }

    // ---- eggHistory verdicts (Met Location) ----
    {
        const auto raichu = B::eggHistory(2, 26, 30);
        check(raichu.verified() && raichu.hatchedAs == 172, "Gen II Raichu via Pichu");
        check(B::eggHistory(3, 172, 4).reason == B::Reason::LevelBelowHatch, "Gen III Lv. 4 never hatched (Lv. 5)");
        check(B::eggHistory(4, 172, 1).verified(), "Gen IV Lv. 1 Pichu hatched");
        check(B::eggHistory(3, 150, 70).reason == B::Reason::Undiscovered, "Mewtwo Undiscovered");
        check(B::eggHistory(4, 132, 30).reason == B::Reason::Ditto, "Ditto");
        const auto manaphy = B::eggHistory(4, 490, 50);
        check(manaphy.reason == B::Reason::HatchesAsOther && manaphy.eggsHatchAs == 489, "Manaphy eggs hatch as Phione");
        const auto azumarill = B::eggHistory(3, 184, 30);
        check(azumarill.verified() && azumarill.hatchedAs == 183, "Gen III Azumarill via Marill");
        const auto nidoqueen = B::eggHistory(3, 31, 40);
        check(nidoqueen.verified() && nidoqueen.hatchedAs == 29, "Nidoqueen via Nidoran F");
        check(B::eggHistory(5, 0, 5).reason == B::Reason::NotInGeneration, "species 0");
    }

    if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
    std::printf("breeding policy: all checks passed\n");
    return 0;
}
