#include "Legality/Gen12TimeCapsuleEvidence.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen12TimeCapsule;

    assert(canVisitGen1Species(1));
    assert(canVisitGen1Species(151));
    assert(canVisitGen1Species(169)); // Crobat can devolve to Golbat.
    assert(canVisitGen1Species(242)); // Blissey can devolve to Chansey.
    assert(!canVisitGen1Species(152)); // Chikorita is Gen II-only.

    assert(couldOriginateGen1(25, false, 0));
    assert(couldOriginateGen1(169, false, 0));
    assert(!couldOriginateGen1(25, true, 0));
    assert(!couldOriginateGen1(25, false, 1));
    assert(!couldOriginateGen1(152, false, 0));

    assert(canCurrentlyTradeToGen1(25, false, std::array<uint16_t,4>{85, 98, 129, 0}));
    assert(!canCurrentlyTradeToGen1(152, false, std::array<uint16_t,4>{33, 0, 0, 0}));
    assert(!canCurrentlyTradeToGen1(25, false, std::array<uint16_t,4>{166, 0, 0, 0}));
    assert(!canCurrentlyTradeToGen1(25, true, std::array<uint16_t,4>{85, 0, 0, 0}));

    // Current Time Capsule eligibility is separate from a possible earlier
    // Gen I origin. A returned Pokemon may evolve or learn Gen II moves.
    constexpr std::array<uint16_t,4> gen1Moves{85,98,129,0};
    constexpr std::array<uint16_t,4> gen2Move{85,166,0,0};
    assert(classifyCurrentTrade(25,false,gen1Moves)==CurrentTrade::Compatible);
    assert(classifyCurrentTrade(25,false,gen2Move)==CurrentTrade::MoveNotInGen1);
    assert(classifyCurrentTrade(169,false,gen1Moves)==CurrentTrade::SpeciesNotInGen1);
    assert(classifyCurrentTrade(152,false,gen1Moves)==CurrentTrade::SpeciesNotInGen1);
    assert(classifyCurrentTrade(25,true,gen1Moves)==CurrentTrade::EggNotTradeable);
    assert(classifyCurrentTrade(0,false,gen1Moves)==CurrentTrade::SpeciesNotInGen1);
    assert(couldOriginateGen1(169,false,0));
    assert(couldOriginateGen1(25,false,0));
    assert(!canCurrentlyTradeToGen1(25,false,gen2Move));
    assert(!canCurrentlyTradeToGen1(169,false,gen1Moves));

    std::cout << "Gen I/II Time Capsule compatibility evidence: PASS\n";
}
