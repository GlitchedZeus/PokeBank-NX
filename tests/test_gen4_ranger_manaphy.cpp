#include "Legality/Gen4RangerManaphy.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4RangerManaphy;

    Candidate egg{
        490, 2, 2, true,
        LocationRanger4, 0, BallPoke, true
    };
    static_assert(matches(egg));

    auto tradedEgg = egg;
    tradedEgg.metLocation = LocationLinkTrade4;
    static_assert(matches(tradedEgg));

    Candidate hatched{
        490, 2, 2, false,
        LocationRanger4, 1, BallPoke, true
    };
    static_assert(matches(hatched));

    auto tradedHatched = hatched;
    tradedHatched.eggLocation = LocationLinkTrade4;
    static_assert(matches(tradedHatched));

    auto korean = egg;
    korean.language = 8;
    static_assert(!matches(korean));

    auto wrongBall = egg;
    wrongBall.ball = 16;
    static_assert(!matches(wrongBall));

    auto ordinaryManaphy = egg;
    ordinaryManaphy.eggLocation = 0;
    static_assert(!matches(ordinaryManaphy));

    std::cout << "Gen IV Ranger Manaphy positive event evidence: PASS\n";
}
