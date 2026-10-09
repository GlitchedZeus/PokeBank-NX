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
    // Real pinned HGSS grass source: HeartGold Pidgey #16, location149,
    // grass slot0, level2, rate20. Kurt Apricorn IDs17..23 are native
    // HGSS wild capture options, not D/P/Pt or fixed BCC/Safari balls.
    constexpr uint64_t pidgey=0x50180002052A10ULL;
    static_assert(Gen4Wild::game(pidgey)==Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(pidgey)==16);
    static_assert(Gen4Wild::location(pidgey)==149);
    static_assert(Gen4Wild::method(pidgey)==0);
    static_assert(Gen4Wild::minLevel(pidgey)==2);
    static_assert(Gen4Wild::maxLevel(pidgey)==2);
    static_assert(Gen4Wild::slot(pidgey)==0);
    static_assert(Gen4Wild::rate(pidgey)==20);
    static_assert(Proof::matchDirectRow(pidgey,17)==Affinity::ApricornWild);
    static_assert(Proof::matchDirectRow(pidgey,23)==Affinity::ApricornWild);
    static_assert(Proof::matchDirectRow(pidgey,16)==Affinity::None);
    static_assert(Proof::matchDirectRow(bcc,17)==Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",16,149,2,0,17)==Affinity::ApricornWild);
    assert(Proof::analyzeSupported(
        "heartgold_nds",16,149,2,0,23)==Affinity::ApricornWild);
    assert(Proof::analyzeSupported(
        "heartgold_nds",16,149,1,0,17)==Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",16,149,2,0,16)==Affinity::None);
    assert(Proof::analyzeSupported(
        "diamond_nds",16,149,2,0,17)==Affinity::None);
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
    // Pinned Locations4: D/P/Pt Great Marsh = location 52, fixed Safari.
    // Real Diamond Arbok #24 encounter row; level20, grass slot 9.
    constexpr uint64_t marsh = 0x8E400014286818ULL;
    static_assert(Gen4Wild::game(marsh) == Gen4Wild::Game::Diamond);
    static_assert(Gen4Wild::species(marsh) == 24);
    static_assert(Gen4Wild::location(marsh) == 52);
    static_assert(Gen4Wild::minLevel(marsh) == 20);
    static_assert(Gen4Wild::maxLevel(marsh) == 20);
    static_assert(Gen4Wild::method(marsh) == 0);
    static_assert(Gen4Wild::slot(marsh) == 9);
    static_assert(Proof::matchDirectRow(marsh,5) == Affinity::GreatMarsh);
    static_assert(Proof::matchDirectRow(marsh,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "diamond_nds",24,52,20,0,5) == Affinity::GreatMarsh);
    // Pinned real HeartGold Nincada #290 BCC row: source for an
    // evolved Shedinja #292. Both Sport ID24 and Poké ID4 are compatible
    // according to pinned BallVerifier.VerifyEvolvedShedinja.
    constexpr uint64_t nincada = 0x64980424359F22ULL;
    static_assert(Gen4Wild::game(nincada) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(nincada) == 290);
    static_assert(Gen4Wild::location(nincada) == 207);
    static_assert(Gen4Wild::method(nincada) == 8);
    static_assert(Gen4Wild::minLevel(nincada) == 26);
    static_assert(Gen4Wild::maxLevel(nincada) == 36);
    static_assert(Gen4Wild::slot(nincada) == 2);
    static_assert(Gen4Wild::rate(nincada) == 25);
    assert(Proof::analyzeSupported(
        "heartgold_nds",292,207,26,0,24) == Affinity::ShedinjaBugContest);
    assert(Proof::analyzeSupported(
        "heartgold_nds",292,207,36,0,4) == Affinity::ShedinjaBugContest);
    assert(Proof::analyzeSupported(
        "soulsilver_nds",292,207,26,0,24) == Affinity::ShedinjaBugContest);
    assert(Proof::analyzeSupported(
        "heartgold_nds",292,207,25,0,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",292,206,26,0,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",292,207,26,0,5) == Affinity::None);
    assert(Proof::analyzeSupported(
        "diamond_nds",292,207,26,0,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",292,207,26,1,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",291,207,26,0,4) == Affinity::None);
    assert(Proof::analyzeSupported(
        "diamond_nds",24,52,20,0,24) == Affinity::None);
    assert(Proof::analyzeSupported(
        "diamond_nds",24,52,1,0,5) == Affinity::None);
    assert(Proof::analyzeSupported(
        "heartgold_nds",24,52,20,0,5) == Affinity::None);
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
