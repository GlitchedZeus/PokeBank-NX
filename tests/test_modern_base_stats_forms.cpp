#include "Pokemon/BaseStatsGen89.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace Names {
const char* getSpeciesName(uint16_t) { return ""; }
const char* getItemName(uint16_t) { return ""; }
const char* getNatureName(uint8_t) { return ""; }
const char* getAbilityName(uint16_t) { return ""; }
}

namespace {
void expectStats(uint16_t species, uint8_t form,
                 int hp, int atk, int def, int spa, int spd, int spe) {
    const auto* s = Pokemon::getBaseStatsGen89(species, form);
    assert(s);
    assert(s->id == species);
    assert(s->hp == hp);
    assert(s->atk == atk);
    assert(s->def == def);
    assert(s->spa == spa);
    assert(s->spd == spd);
    assert(s->spe == spe);
}

void expectEmpty(uint16_t species, uint8_t form) {
    const auto* s = Pokemon::getBaseStatsGen89(species, form);
    assert(s);
    assert(s->id == 0);
    assert(s->hp == 0 && s->atk == 0 && s->def == 0);
    assert(s->spa == 0 && s->spd == 0 && s->spe == 0);
}
}

int main() {
    // AUDIT-028: Tauros form 0 is Kanto; Paldean breeds are forms 1..3.
    expectStats(128, 0, 75, 100, 95, 40, 70, 110);
    for (uint8_t f = 1; f <= 3; ++f)
        expectStats(128, f, 75, 110, 105, 30, 70, 100);
    expectEmpty(128, 4);

    // Darmanitan raw form numbering is Standard, Zen, Galarian, Galarian Zen.
    expectStats(555, 0, 105, 140, 55, 30, 55, 95);
    expectStats(555, 1, 105, 30, 105, 140, 105, 55);
    expectStats(555, 2, 105, 140, 55, 30, 55, 95);
    expectStats(555, 3, 105, 160, 55, 30, 55, 135);
    expectEmpty(555, 4);

    // Combined genie table must stay species-local.
    expectStats(641, 0, 79, 115, 70, 125, 80, 111);
    expectStats(641, 1, 79, 100, 80, 110, 90, 121);
    expectStats(642, 0, 79, 115, 70, 125, 80, 111);
    expectStats(642, 1, 79, 105, 70, 145, 80, 101);
    expectStats(645, 0, 89, 125, 90, 115, 80, 101);
    expectStats(645, 1, 89, 145, 90, 105, 80, 91);

    // Dedicated arrays which previously fell through to dense zero placeholders.
    expectStats(681, 0, 60, 50, 140, 50, 140, 60);
    expectStats(681, 1, 60, 140, 50, 140, 50, 60);
    expectStats(746, 0, 45, 20, 20, 25, 25, 40);
    expectStats(746, 1, 45, 140, 130, 140, 135, 30);
    expectStats(875, 0, 75, 80, 110, 65, 90, 50);
    expectStats(875, 1, 75, 80, 70, 65, 50, 130);
    expectStats(877, 0, 58, 95, 58, 70, 58, 97);
    expectStats(877, 1, 58, 95, 58, 70, 58, 97);
    expectStats(964, 0, 100, 70, 72, 53, 62, 100);
    expectStats(964, 1, 100, 160, 97, 106, 87, 100);

    // Minior raw forms 0..6 share Meteor stats; 7..13 share Core stats.
    expectStats(774, 0, 60, 60, 100, 60, 100, 60);
    expectStats(774, 6, 60, 60, 100, 60, 100, 60);
    expectStats(774, 7, 60, 100, 60, 100, 60, 120);
    expectStats(774, 13, 60, 100, 60, 100, 60, 120);
    expectEmpty(774, 14);

    // Base + one-row dedicated forms must not raw-index past the dedicated array.
    expectStats(901, 0, 130, 140, 105, 45, 80, 50);
    expectStats(901, 1, 113, 70, 120, 135, 65, 52);
    expectEmpty(901, 2);
    expectStats(890, 0, 140, 85, 95, 145, 95, 130);
    expectStats(890, 1, 255, 115, 250, 125, 250, 130);

    // Terapagos dedicated table contains all three forms; raw form maps directly.
    expectStats(1024, 0, 90, 65, 85, 65, 85, 60);
    expectStats(1024, 1, 95, 95, 110, 105, 110, 85);
    expectStats(1024, 2, 160, 105, 110, 130, 110, 85);
    expectEmpty(1024, 3);

    // SWSH historical overrides still apply, but invalid forms now fail through
    // the bounded generic router instead of silently becoming Hero form.
    const auto* zacianHero = Pokemon::getBaseStatsSWSH(888, 0);
    const auto* zacianCrowned = Pokemon::getBaseStatsSWSH(888, 1);
    assert(zacianHero->atk == 130 && zacianHero->spe == 138);
    assert(zacianCrowned->atk == 170 && zacianCrowned->spe == 148);
    expectEmpty(888, 2);

    std::cout << "Modern base-stat form routing: PASS\n";
    return 0;
}
