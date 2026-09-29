#include "Legality/Gen4TradeEvidence.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Trade;

    // D/P/Pt Abra NPC trade.
    assert(matches("diamond_nds", 63, 0x0000008Eu, 25643u, 0, 1,
                   std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 1));
    assert(matches("platinum_nds", 63, 0x0000008Eu, 25643u, 0, 1,
                   std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 50));
    assert(!matches("heartgold_nds", 63, 0x0000008Eu, 25643u, 0, 1,
                    std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 1));

    // HG/SS Webster's Spearow has an explicit met location and level.
    assert(matches("heartgold_nds", 21, 0x00006B5Eu, 1001u, 1, 0,
                   std::array<uint8_t,6>{15,20,15,20,20,20}, 183, 20));
    assert(!matches("heartgold_nds", 21, 0x00006B5Eu, 1001u, 1, 0,
                    std::array<uint8_t,6>{15,20,15,20,20,20}, 2001, 20));

    // Kirk's Shuckle uses SID=1; ID32 must preserve it.
    assert(matches("soulsilver_nds", 213, 0x000214D7u, 0x000110F0u, 0, 0,
                   std::array<uint8_t,6>{15,20,15,20,20,20}, 130, 20));

    auto wrong = std::array<uint8_t,6>{15,20,15,20,20,19};
    assert(!matches("soulsilver_nds", 213, 0x000214D7u, 0x000110F0u, 0, 0,
                    wrong, 130, 20));

    assert(hasSpecies("platinum_nds", 441));
    assert(hasSpecies("heartgold_nds", 25));
    assert(!hasSpecies("diamond_nds", 25));

    std::cout << "Gen IV fixed in-game trade evidence: PASS\n";
}
