#include "Legality/Gen3MystryMewEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality;
    using namespace Legality::Gen3MystryMew;

    static_assert(kBaseSeeds.size() == 86);
    static_assert(validSeedProvenance(0x0652u));
    static_assert(!validSeedProvenance(0x6065u));
    static_assert(validSeedProvenance(0xE8D1A0BFu)); // released sibling index 2
    static_assert(seedInfo(0xE8D1A0BFu).baseSeed == 0x6065u);
    static_assert(seedInfo(0xE8D1A0BFu).subIndex == 2);

    const auto rng = Gen3BacdPidIv::analyzeWithTrainer(
        0xB75C1132u, {3, 9, 27, 31, 21, 5}, 6930, 0);
    assert(rng.matched());
    assert(rng.variant == Gen3BacdPidIv::Variant::Regular);
    assert(rng.originSeed == 0x0652u);
    assert(expectedOtGender(rng.originSeed) == 0);

    Candidate mew{
        151, 6930, 0,
        2, 2, 0,
        10, 255, 4,
        false, true, false,
        u"MYSTRY"
    };
    assert(matches(mew, rng));

    auto wrongOt = mew;
    wrongOt.otName = u"WISHMKR";
    assert(!matches(wrongOt, rng));

    auto wrongFateful = mew;
    wrongFateful.fateful = false;
    assert(!matches(wrongFateful, rng));

    auto wrongShiny = mew;
    wrongShiny.shiny = true;
    assert(!matches(wrongShiny, rng));

    auto wrongLanguage = mew;
    wrongLanguage.language = 1;
    assert(!matches(wrongLanguage, rng));

    auto unreleased = rng;
    unreleased.originSeed = 0x6065u;
    assert(!matches(mew, unreleased));

    std::cout << "Gen III MYSTRY Mew seed/template evidence: PASS\n";
}
