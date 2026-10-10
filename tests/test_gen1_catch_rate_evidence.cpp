#include "Legality/Gen1CatchRateEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen1CatchRate;

    assert(expectedRate("red_gb", 1) == 45);
    assert(expectedRate("blue_gb", 1) == 45);
    assert(expectedRate("yellow_gb", 1) == 45);

    // Exact game differences in the pinned Gen I personal tables.
    assert(expectedRate("red_gb", 25) == 190);
    assert(expectedRate("yellow_gb", 25) == 163);
    assert(expectedRate("red_gb", 64) == 100);
    assert(expectedRate("yellow_gb", 64) == 96);
    assert(expectedRate("red_gb", 148) == 45);
    assert(expectedRate("yellow_gb", 148) == 27);
    assert(expectedRate("red_gb", 149) == 45);
    assert(expectedRate("yellow_gb", 149) == 9);

    assert(classify("red_gb", 1, 45) == Evidence::NativeSpeciesRate);

    // Caterpie -> Metapod -> Butterfree is contiguous in Gen I. A Butterfree can retain
    // Caterpie's 255 catch rate after evolution.
    assert(evolutionStage(12) == 2);
    assert(matchesGen1SpeciesOrPreEvolutionRate(12, 255));
    assert(classify("red_gb", 12, 255) == Evidence::Gen1SpeciesOrPreEvolutionRate);

    // Branched Eevee evolutions are graph edges, NOT adjacent species IDs.
    // Pinned Yellow source rates: Eevee 45, Vaporeon 27, Jolteon 9,
    // Flareon 3. Jolteon/Flareon may retain Eevee's original 45,
    // but cannot inherit the rate of their mutually exclusive siblings.
    static_assert(expectedRate("yellow_gb", 133) == 45);
    static_assert(expectedRate("yellow_gb", 134) == 27);
    static_assert(expectedRate("yellow_gb", 135) == 9);
    static_assert(expectedRate("yellow_gb", 136) == 3);
    for (uint16_t target : {134u, 135u, 136u}) {
        assert(matchesGen1SpeciesOrPreEvolutionRate(target, 45));
    }
    assert(matchesGen1SpeciesOrPreEvolutionRate(134, 27));
    assert(matchesGen1SpeciesOrPreEvolutionRate(135, 9));
    assert(matchesGen1SpeciesOrPreEvolutionRate(136, 3));
    assert(!matchesGen1SpeciesOrPreEvolutionRate(134, 9));
    assert(!matchesGen1SpeciesOrPreEvolutionRate(134, 3));
    assert(!matchesGen1SpeciesOrPreEvolutionRate(135, 27));
    assert(!matchesGen1SpeciesOrPreEvolutionRate(135, 3));
    assert(!matchesGen1SpeciesOrPreEvolutionRate(136, 27));
    assert(!matchesGen1SpeciesOrPreEvolutionRate(136, 9));
    // Independent linear families must retain their genuine chains.
    assert(matchesGen1SpeciesOrPreEvolutionRate(12, 255));
    assert(matchesGen1SpeciesOrPreEvolutionRate(26, 190));

    // Yellow Pikachu's 0xA3 catch-rate byte is also a valid Gen II held-item byte.
    // The byte alone therefore cannot prove whether the Pokemon visited Gen II.
    assert(expectedRate("yellow_gb", 25) == 163);
    assert(isPossibleTimeCapsuleHeldItem(163));
    assert(classify("yellow_gb", 25, 163) ==
           Evidence::AmbiguousGen1OrTimeCapsuleHeldItem);

    // 0x01 is a valid Gen II item byte but not a Bulbasaur R/B/Y/pre-evolution rate.
    assert(isPossibleTimeCapsuleHeldItem(0x01));
    assert(classify("red_gb", 1, 0x01) == Evidence::PossibleTimeCapsuleHeldItem);

    assert(classify("unknown", 1, 45) == Evidence::Unknown);
    assert(expectedRate("red_gb", 0) == 0);

    std::cout << "Gen I catch-rate legality evidence: PASS\n";
}
