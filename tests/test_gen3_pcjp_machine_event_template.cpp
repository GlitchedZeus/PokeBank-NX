#include "Legality/Gen3PcjpMachineEventTemplate.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {
constexpr uint16_t trainerIdFor(Legality::Gen3PcjpMachineEvent::Distribution dist) {
    using Distribution = Legality::Gen3PcjpMachineEvent::Distribution;
    switch (dist) {
        case Distribution::First:  return 51126;
        case Distribution::Second: return 51224;
        case Distribution::Third:  return 60114;
        case Distribution::Fourth: return 60227;
        case Distribution::Fifth:  return 60321;
        case Distribution::Sixth:  return 60505;
        case Distribution::None:   return 0;
    }
    return 0;
}

Legality::Gen3PcjpMachineEvent::Candidate candidateFor(
    Legality::Gen3PcjpMachineEvent::Distribution dist,
    uint16_t species,
    uint32_t originSeed,
    std::u16string_view otName = u"トウキョー") {
    using namespace Legality::Gen3PcjpMachineEvent;
    return {
        species,
        trainerIdFor(dist),
        0,
        2, // Ruby origin
        1, // Japanese
        expectedOtGender(originSeed),
        10,
        255,
        4,
        false,
        false,
        otName,
    };
}
}

int main() {
    using namespace Legality::Gen3PcjpMachineEvent;
    using Legality::Gen3BacdPidIv::Result;
    using Legality::Gen3BacdPidIv::Variant;

    static_assert(kEventCount == 58);

    // First distribution: Hoenn starter descendants.
    static_assert(sourceSpeciesCompatible(252, 253));
    static_assert(sourceSpeciesCompatible(252, 254));
    static_assert(sourceSpeciesCompatible(255, 256));
    static_assert(sourceSpeciesCompatible(255, 257));
    static_assert(sourceSpeciesCompatible(258, 259));
    static_assert(sourceSpeciesCompatible(258, 260));

    // Second distribution: Johto starter descendants.
    static_assert(sourceSpeciesCompatible(152, 153));
    static_assert(sourceSpeciesCompatible(152, 154));
    static_assert(sourceSpeciesCompatible(155, 156));
    static_assert(sourceSpeciesCompatible(155, 157));
    static_assert(sourceSpeciesCompatible(158, 159));
    static_assert(sourceSpeciesCompatible(158, 160));

    // Third distribution: all source species with descendants available in Gen III.
    static_assert(sourceSpeciesCompatible(23, 24));
    static_assert(sourceSpeciesCompatible(25, 26));
    static_assert(sourceSpeciesCompatible(52, 53));
    static_assert(sourceSpeciesCompatible(58, 59));
    static_assert(sourceSpeciesCompatible(69, 70));
    static_assert(sourceSpeciesCompatible(69, 71));
    static_assert(sourceSpeciesCompatible(79, 80));
    static_assert(sourceSpeciesCompatible(79, 199)); // Slowking branch
    static_assert(sourceSpeciesCompatible(90, 91));
    static_assert(sourceSpeciesCompatible(113, 242));
    static_assert(sourceSpeciesCompatible(123, 212));

    // Fourth distribution: Kanto starter descendants.
    static_assert(sourceSpeciesCompatible(1, 2));
    static_assert(sourceSpeciesCompatible(1, 3));
    static_assert(sourceSpeciesCompatible(4, 5));
    static_assert(sourceSpeciesCompatible(4, 6));
    static_assert(sourceSpeciesCompatible(7, 8));
    static_assert(sourceSpeciesCompatible(7, 9));

    // Fifth distribution: Hoenn lines that can evolve within a PK3.
    static_assert(sourceSpeciesCompatible(270, 271));
    static_assert(sourceSpeciesCompatible(270, 272));
    static_assert(sourceSpeciesCompatible(273, 274));
    static_assert(sourceSpeciesCompatible(273, 275));
    static_assert(sourceSpeciesCompatible(283, 284));
    static_assert(sourceSpeciesCompatible(300, 301));
    static_assert(sourceSpeciesCompatible(307, 308));

    // Sixth distribution: remaining Gen III-reachable descendants.
    static_assert(sourceSpeciesCompatible(163, 164));
    static_assert(sourceSpeciesCompatible(179, 180));
    static_assert(sourceSpeciesCompatible(179, 181));
    static_assert(sourceSpeciesCompatible(191, 192));
    static_assert(sourceSpeciesCompatible(204, 205));
    static_assert(sourceSpeciesCompatible(209, 210));
    static_assert(sourceSpeciesCompatible(216, 217));
    static_assert(sourceSpeciesCompatible(228, 229));

    // Directionality and generation boundaries stay strict. These evolutions
    // either run backwards or were introduced after Generation III and cannot
    // exist in a surviving PK3.
    static_assert(!sourceSpeciesCompatible(254, 252));
    static_assert(!sourceSpeciesCompatible(125, 466)); // Electivire
    static_assert(!sourceSpeciesCompatible(126, 467)); // Magmortar
    static_assert(!sourceSpeciesCompatible(198, 430)); // Honchkrow
    static_assert(!sourceSpeciesCompatible(200, 429)); // Mismagius
    static_assert(!sourceSpeciesCompatible(215, 461)); // Weavile
    static_assert(!sourceSpeciesCompatible(207, 472)); // Gliscor
    static_assert(!sourceSpeciesCompatible(190, 424)); // Ambipom
    static_assert(!sourceSpeciesCompatible(315, 407)); // Roserade

    static_assert(speciesAllowed(Distribution::First, 254));
    static_assert(speciesAllowed(Distribution::Second, 157));
    static_assert(speciesAllowed(Distribution::Third, 199));
    static_assert(speciesAllowed(Distribution::Fourth, 6));
    static_assert(speciesAllowed(Distribution::Fifth, 272));
    static_assert(speciesAllowed(Distribution::Sixth, 181));
    static_assert(!speciesAllowed(Distribution::First, 199));
    static_assert(!speciesAllowed(Distribution::Fifth, 407));

    constexpr uint32_t seed = 0x12345678u;
    const Result regular{Variant::Regular, seed, true};

    // One evolved positive vector from every distribution preserves all other
    // source-backed PCJP invariants: TID, Ruby origin, Japanese language,
    // location/ball, city OT, OT-gender reconstruction and BA-CD class.
    assert(matches(candidateFor(Distribution::First, 254, seed), regular));
    assert(matches(candidateFor(Distribution::Second, 157, seed), regular));
    assert(matches(candidateFor(Distribution::Third, 199, seed), regular));
    assert(matches(candidateFor(Distribution::Fourth, 6, seed), regular));
    assert(matches(candidateFor(Distribution::Fifth, 272, seed), regular));
    assert(matches(candidateFor(Distribution::Sixth, 181, seed), regular));

    // A descendant is only valid if its source species belongs to that exact
    // TID/distribution table.
    assert(!matches(candidateFor(Distribution::First, 199, seed), regular));

    // Sapporo is excluded from the sixth distribution by the pinned source.
    assert(!matches(candidateFor(Distribution::Sixth, 181, seed, u"サッポロ"), regular));

    // Keep the existing event RNG class boundary unchanged.
    const Result antiShiny{Variant::RegularAntiShiny, seed, true};
    assert(!matches(candidateFor(Distribution::First, 254, seed), antiShiny));

    std::cout << "Gen III PCJP evolved machine-gift provenance: PASS\n";
}
