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
        "heartgold_nds",17,202,15,0,5,0x00010000u);
    const auto pidgeot=E::analyzeSupported(
        "heartgold_nds",18,202,15,0,5,0x00010000u);
    assert(pidgeotto.matched() && pidgeotto.affinity==A::Safari);
    assert(pidgeotto.sourceSpecies==16 && pidgeotto.matchingAncestors==1);
    assert(pidgeot.matched() && pidgeot.affinity==A::Safari);
    assert(pidgeot.sourceSpecies==16 && pidgeot.matchingAncestors==1);

    // Pinned HeartGold grass Pidgey location149/level2:
    // same retained Apricorn capture after two evolutions.
    for(uint8_t ball=17;ball<=23;++ball) {
        const auto outcome=E::analyzeSupported(
            "heartgold_nds",18,149,2,0,ball,0x00010000u);
        assert(outcome.matched() && outcome.affinity==A::ApricornWild);
        assert(outcome.sourceSpecies==16);
    }

    // Pinned HeartGold Wurmple #265 Headbutt row (0xD80303051509)
    // location138, level2-3, exact species and HGSS game. A Beautifly
    // evolved via Wurmple may retain the Apricorn Ball only when the
    // PID selects the Silcoon/Beautifly branch. PID upper half 1 -> 0;
    // PID upper half 6 -> 1 (Cascoon/Dustox).
    constexpr uint64_t wurmple=0xD80303051509ULL;
    static_assert(Gen4Wild::game(wurmple)==Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(wurmple)==265);
    static_assert(Gen4Wild::location(wurmple)==138);
    static_assert(Gen4Wild::minLevel(wurmple)==2);
    static_assert(Gen4Wild::maxLevel(wurmple)==3);
    static_assert(Gen4Wild::method(wurmple)==6);
    assert(Gen4SpecialBallEvidence::analyzeSupported(
        "heartgold_nds",265,138,2,0,17)==A::ApricornWild);
    const auto beautyAllowed=E::analyzeSupported(
        "heartgold_nds",267,138,2,0,17,0x00010000u);
    const auto beautyWrong=E::analyzeSupported(
        "heartgold_nds",267,138,2,0,17,0x00060000u);
    const auto dustoxAllowed=E::analyzeSupported(
        "heartgold_nds",269,138,2,0,17,0x00060000u);
    const auto dustoxWrong=E::analyzeSupported(
        "heartgold_nds",269,138,2,0,17,0x00010000u);
    assert(beautyAllowed.matched() && beautyAllowed.sourceSpecies==265);
    assert(!beautyWrong.matched());
    assert(dustoxAllowed.matched() && dustoxAllowed.sourceSpecies==265);
    assert(!dustoxWrong.matched());

    // The same evidence cannot be claimed for mismatched persistent
    // fields, unsupported formats, special Shedinja or wrong ball.
    assert(!E::analyzeSupported("diamond_nds",18,202,15,0,5,0x00010000u).matched());
    assert(!E::analyzeSupported("heartgold_nds",18,202,1,0,5,0x00010000u).matched());
    assert(!E::analyzeSupported("heartgold_nds",18,206,15,0,5,0x00010000u).matched());
    assert(!E::analyzeSupported("heartgold_nds",18,202,15,0,24,0x00010000u).matched());
    assert(!E::analyzeSupported("heartgold_nds",18,202,15,1,5,0x00010000u).matched());
    assert(!E::analyzeSupported("heartgold_nds",292,207,26,0,24,0x00010000u).matched());
    assert(!E::analyzeSupported("unknown",18,202,15,0,5,0x00010000u).matched());
    assert(!E::analyzeSupported("heartgold_nds",0,202,15,0,5,0x00010000u).matched());
    assert(!E::analyzeSupported("heartgold_nds",16,202,15,0,5,0x00010000u).matched());
}
