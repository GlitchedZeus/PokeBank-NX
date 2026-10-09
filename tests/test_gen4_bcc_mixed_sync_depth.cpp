#include "Legality/Gen4BugContestMixedDepthEvidence.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace Legality;
    namespace M = Gen4BugContestMixedDepth;
    // Pinned encounter_hg.pkl HeartGold Kakuna BCC row, slot 3, location
    // 207, level 9..18, actual encounter rate 25.
    constexpr uint64_t row = 0x64D80412139E0EULL;
    static_assert(Gen4Wild::game(row) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(row) == 14);
    static_assert(Gen4Wild::location(row) == 207);
    static_assert(Gen4Wild::slot(row) == 3);
    static_assert(Gen4Wild::rate(row) == 25);

    // Two rejects: original failed Sync, middle successful Sync, retained
    // failed Sync. Real activation roll 13, met level 16.
    constexpr auto twice=M::match(row,0x000E11BEu,0x9C2C4659u,16,2,0b010u);
    static_assert(twice.matched());
    static_assert(twice.originalPid==0x3B865496u);
    static_assert(twice.lockedNature==7 && twice.depth==2);
    static_assert(twice.slot==3 && twice.level==16);

    // Three rejects: F-F-F-S, persisted fourth Pokémon has no 31 IV.
    // This is legal ONLY at depth 3 after all earlier attempts rejected.
    constexpr auto thrice=M::match(row,0x0014FEAAu,0x8EE19004u,10,3,0b1000u);
    static_assert(thrice.matched());
    static_assert(thrice.originalPid==0xC56F98A2u);
    static_assert(thrice.lockedNature==15 && thrice.depth==3);
    static_assert(!Gen4LeadFrame::directMinimum31Satisfied(0x0014FEAAu));

    // Independent source-valid variants: S-F-F, and F-F-S-F.
    constexpr auto twiceAlt=M::match(row,0x001FF1EEu,0xA2BD5564u,16,2,0b001u);
    static_assert(twiceAlt.matched() && twiceAlt.lockedNature==16);
    constexpr auto thriceAlt=M::match(row,0x002B16DEu,0x9B684104u,12,3,0b0100u);
    static_assert(thriceAlt.matched() && thriceAlt.lockedNature==19);

    // Missing proof stays unresolved; no 'Invalid' transitions here.
    static_assert(!M::match(row,0x000E11BEu,0x9C2C4659u,15,2,0b010u).matched());
    static_assert(!M::match(row,0x000E11BEu,0x9C2C4658u,16,2,0b010u).matched());
    static_assert(!M::match(row,0x000E11BEu,0x9C2C4659u,16,2,0b100u).matched());
    static_assert(!M::match(row,0x000E11BEu,0x9C2C4659u,16,1,0b010u).matched());
    static_assert(!M::match(row,0x000E11BEu,0x9C2C4659u,16,4,0b010u).matched());
    static_assert(!M::match(row,0x000E11BEu,0x9C2C4659u,16,2,0b000u).matched());
    static_assert(!M::match(row,0x000E11BEu,0x9C2C4659u,16,2,0b111u).matched());
    static_assert(!M::match(row & ~(0xffULL<<50),0x000E11BEu,0x9C2C4659u,16,2,0b010u).matched());
    static_assert(!M::match(row & ~(0xfULL<<31),0x000E11BEu,0x9C2C4659u,16,2,0b010u).matched());

    assert(twice.matched() && thrice.matched() &&
           twiceAlt.matched() && thriceAlt.matched());
    std::cout<<"Gen IV BCC mixed Synchronize 2/3 rerolls: PASS\n";
}
