#include "Legality/Gen4TradeEvidence.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Trade;

    // D/P/Pt Abra NPC trade.
    assert(matches("diamond_nds", 63, 0x0000008Eu, 25643u, 0, 1, 1,
                   std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 1));
    assert(matches("platinum_nds", 63, 0x0000008Eu, 25643u, 0, 1, 1,
                   std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 50));
    assert(!matches("heartgold_nds", 63, 0x0000008Eu, 25643u, 0, 1, 1,
                    std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 1));

    // Evolution must not erase fixed NPC-trade provenance.
    const auto evolvedKadabra = matchEvolutionLine(
        "diamond_nds", 64, 0x0000008Eu, 25643u, 0, 1, 1,
        std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 16);
    assert(evolvedKadabra.matched);
    assert(evolvedKadabra.evolved);
    assert(evolvedKadabra.sourceSpecies == 63);
    const auto evolvedAlakazam = matchEvolutionLine(
        "platinum_nds", 65, 0x0000008Eu, 25643u, 0, 1, 1,
        std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 50);
    assert(evolvedAlakazam.matched);
    assert(evolvedAlakazam.sourceSpecies == 63);

    // HG/SS Webster's Spearow has an explicit met location and level.
    assert(matches("heartgold_nds", 21, 0x00006B5Eu, 1001u, 1, 0, 1,
                   std::array<uint8_t,6>{15,20,15,20,20,20}, 183, 20));
    assert(!matches("heartgold_nds", 21, 0x00006B5Eu, 1001u, 1, 0, 1,
                    std::array<uint8_t,6>{15,20,15,20,20,20}, 2001, 20));
    const auto evolvedFearow = matchEvolutionLine(
        "heartgold_nds", 22, 0x00006B5Eu, 1001u, 1, 0, 1,
        std::array<uint8_t,6>{15,20,15,20,20,20}, 183, 20);
    assert(evolvedFearow.matched);
    assert(evolvedFearow.evolved);
    assert(evolvedFearow.sourceSpecies == 21);

    // Kirk's Shuckle uses SID=1; ID32 must preserve it.
    assert(matches("soulsilver_nds", 213, 0x000214D7u, 0x000110F0u, 0, 0, 2,
                   std::array<uint8_t,6>{15,20,15,20,20,20}, 130, 20));

    auto wrong = std::array<uint8_t,6>{15,20,15,20,20,19};
    assert(!matches("soulsilver_nds", 213, 0x000214D7u, 0x000110F0u, 0, 0, 2,
                    wrong, 130, 20));

    // Ability slot is fixed by the source trade and cannot be changed in Gen IV.
    assert(!matches("diamond_nds", 63, 0x0000008Eu, 25643u, 0, 1, 2,
                    std::array<uint8_t,6>{15,15,15,20,25,25}, 2001, 1));
    assert(!matches("soulsilver_nds", 213, 0x000214D7u, 0x000110F0u, 0, 0, 1,
                    std::array<uint8_t,6>{15,20,15,20,20,20}, 130, 20));

    assert(hasSpecies("platinum_nds", 441));
    assert(hasSpecies("heartgold_nds", 25));
    assert(!hasSpecies("diamond_nds", 25));

    std::cout << "Gen IV fixed in-game trade evidence: PASS\n";
}
