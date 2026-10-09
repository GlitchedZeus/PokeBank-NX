#include "Legality/Gen4SpecialBallEvidence.h"
#include <cassert>
#include <iostream>

int main() {
    namespace Proof = Legality::Gen4SpecialBallEvidence;
    using Proof::Affinity;
    using namespace Legality;
    // Original pinned HeartGold Bug-Catching Contest source: Kakuna #14.
    constexpr uint64_t bcc = 0x64D80412139E0EULL;
    static_assert(Gen4Wild::game(bcc) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(bcc) == 14);
    static_assert(Gen4Wild::location(bcc) == 207);
    static_assert(Gen4Wild::method(bcc) == 8);
    static_assert(Gen4Wild::rate(bcc) == 25);
    static_assert(Proof::matchDirectRow(bcc,24) == Affinity::Sport);
    static_assert(Proof::matchDirectRow(bcc,4) == Affinity::None);

    // Pinned encounter_hg.pkl HeartGold Safari Grass: Pidgey #16, location
    // 202, fixed level15. Safari Ball ID5 has a compatible direct source.
    constexpr uint64_t safari = 0x1858050F1F9410ULL;
    static_assert(Gen4Wild::game(safari) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(safari) == 16);
    static_assert(Gen4Wild::location(safari) == 202);
    static_assert(Gen4Wild::method(safari) == 10);
    static_assert(Gen4Wild::minLevel(safari) == 15);
    static_assert(Gen4Wild::maxLevel(safari) == 15);
    static_assert(Proof::matchDirectRow(safari,5) == Affinity::Safari);
    static_assert(Proof::matchDirectRow(safari,24) == Affinity::None);

    assert(Proof::analyzeSupported(
        "heartgold_nds",14,207,15,0,24) == Affinity::Sport);
    assert(Proof::analyzeSupported(
        "heartgold_nds",16,202,15,0,5) == Affinity::Safari);
    // Pinned HGSS source also contains an authentic level-16 Pidgey
    // Safari slot at location 202. Do NOT claim it impossible!
    assert(Proof::analyzeSupported(
        "heartgold_nds",16,202,16,0,5) == Affinity::Safari);

    // Unknown, wrong generation, unwired ball, nonmatching ball/level,
    // alternate species, and non-native encounter are all simply no PROOF.
    assert(Proof::analyzeSupported(
        "platinum_nds",14,207,15,0,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",14,207,15,0,4) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",14,207,15,0,0) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",14,207,1,0,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",14,999,15,0,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",16,202,15,0,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",16,202,1,0,5) == Affinity::None);
    assert(Proof::analyzeSupported(
        "unknown",14,207,15,0,24) == Affinity::None);

    std::cout << "Gen IV source-specific Sport/Safari positive ball evidence: PASS\n";
}
