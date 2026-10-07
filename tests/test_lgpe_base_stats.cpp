#include "Pokemon/BaseStatsGen7.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Names {
const char* getSpeciesName(uint16_t) { return ""; }
const char* getItemName(uint16_t) { return ""; }
const char* getNatureName(uint8_t) { return ""; }
}

namespace {
void expect(uint16_t species, uint8_t form,
            int hp, int atk, int def, int spa, int spd, int spe) {
    const auto* s = Pokemon::getBaseStatsGen7(species, form);
    assert(s);
    assert(s->id == species);
    assert(s->hp == hp && s->atk == atk && s->def == def);
    assert(s->spa == spa && s->spd == spd && s->spe == spe);
}

void expectEmpty(uint16_t species, uint8_t form = 0) {
    const auto* s = Pokemon::getBaseStatsGen7(species, form);
    assert(s);
    assert(s->id == 0 && s->hp == 0 && s->atk == 0 && s->def == 0);
    assert(s->spa == 0 && s->spd == 0 && s->spe == 0);
}

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
}

int main() {
    // Dense Kanto rows remain direct.
    expect(1, 0, 45, 49, 49, 65, 65, 45);
    expect(151, 0, 100, 100, 100, 100, 100, 100);

    // Sparse rows appended after Mew are resolved by their record id, not row index.
    expect(808, 0, 46, 65, 65, 55, 35, 34);
    expect(809, 0, 135, 143, 143, 80, 65, 34);

    // The old row-count check aliased these unsupported dex ids to Meltan/Melmetal.
    expectEmpty(152);
    expectEmpty(153);
    expectEmpty(810);
    expectEmpty(808, 1);

    // Existing regional routing remains intact.
    expect(26, 1, 60, 85, 50, 95, 85, 110);

    // PB7 stat/CP writeback must refuse unresolved base-stat records rather than
    // serializing zero-base calculations into the party tail.
    const std::string pb7 = read("src/Pokemon/Pokemon7LGPE.cpp");
    assert(pb7.find("const auto* base = getBaseStatsGen7(species, form());") != std::string::npos);
    assert(pb7.find("base->id != species || base->hp == 0") != std::string::npos);
    assert(pb7.find("preserve the existing party-stat/CP tail") != std::string::npos);

    std::cout << "LGPE sparse base-stat routing: PASS\n";
    return 0;
}
