#include "Legality/Gen4EvolvedSpecialBallEvidence.h"
#include <cassert>

int main() {
    namespace E=Legality::Gen4EvolvedSpecialBall;
    using A=Legality::Gen4SpecialBallEvidence::Affinity;
    using Legality::Gen34EggMove::preEvolution;

    // Pinned real HeartGold Safari Zone Pidgey at location202, level15.
    // Captured Pidgey can later become Pidgeotto/Pidgeot while keeping
    // its original Safari Ball, source game and met-location fields.
    static_assert(preEvolution("heartgold_nds",17)==16);
    static_assert(preEvolution("heartgold_nds",18)==17);
    const auto pidgeotto=E::analyzeSupported(
        "heartgold_nds",17,202,15,0,5);
    const auto pidgeot=E::analyzeSupported(
        "heartgold_nds",18,202,15,0,5);
    assert(pidgeotto.matched() && pidgeotto.affinity==A::Safari);
    assert(pidgeotto.sourceSpecies==16 && pidgeotto.matchingAncestors==1);
    assert(pidgeot.matched() && pidgeot.affinity==A::Safari);
    assert(pidgeot.sourceSpecies==16 && pidgeot.matchingAncestors==1);

    // Pinned HeartGold grass Pidgey location149/level2:
    // same retained Apricorn capture after two evolutions.
    for(uint8_t ball=17;ball<=23;++ball) {
        const auto outcome=E::analyzeSupported(
            "heartgold_nds",18,149,2,0,ball);
        assert(outcome.matched() && outcome.affinity==A::ApricornWild);
        assert(outcome.sourceSpecies==16);
    }

    // The same evidence cannot be claimed for mismatched persistent
    // fields, unsupported formats, special Shedinja or wrong ball.
    assert(!E::analyzeSupported("diamond_nds",18,202,15,0,5).matched());
    assert(!E::analyzeSupported("heartgold_nds",18,202,1,0,5).matched());
    assert(!E::analyzeSupported("heartgold_nds",18,206,15,0,5).matched());
    assert(!E::analyzeSupported("heartgold_nds",18,202,15,0,24).matched());
    assert(!E::analyzeSupported("heartgold_nds",18,202,15,1,5).matched());
    assert(!E::analyzeSupported("heartgold_nds",292,207,26,0,24).matched());
    assert(!E::analyzeSupported("unknown",18,202,15,0,5).matched());
    assert(!E::analyzeSupported("heartgold_nds",0,202,15,0,5).matched());
    assert(!E::analyzeSupported("heartgold_nds",16,202,15,0,5).matched());
}
