#include "Legality/Gen3CxdPidIvCorrelation.h"
#include "Legality/Gen3XdShadowEncounter.h"

#include <cassert>
#include <iostream>

int main() {
    namespace XD = Legality::Gen3XdShadowEncounter;

    static_assert(XD::kEncounterCount == 90);

    std::size_t fixedBallRows = 0;
    for (const auto& row : XD::kEntries)
        if (row.fixedBall != 0)
            ++fixedBallRows;
    assert(fixedBallRows == 2);

    // Teddiursa is one of the two source-fixed Poke Ball shadows.
    XD::Candidate teddiursa{
        216, 15, 11, 143, 4, false, true, false
    };
    const auto teddiursaMatch = XD::match(teddiursa);
    assert(teddiursaMatch.matched);
    assert(teddiursaMatch.index == 1);
    assert(teddiursaMatch.fixedBall);
    assert(!teddiursaMatch.rebattleLocation);

    auto teddiursaWrongBall = teddiursa;
    teddiursaWrongBall.ball = 3;
    assert(!XD::match(teddiursaWrongBall).matched);

    // Ordinary shadow rows do not have a fixed post-snag ball restriction.
    XD::Candidate mawile{
        303, 15, 22, 111, 3, false, true, false
    };
    assert(XD::match(mawile).matched);

    // The pinned template explicitly permits Miror B. rebattle locations.
    auto mawileRebattle = mawile;
    mawileRebattle.metLocation = 90;
    const auto rebattleMatch = XD::match(mawileRebattle);
    assert(rebattleMatch.matched);
    assert(rebattleMatch.rebattleLocation);

    auto wrongLevel = mawile;
    wrongLevel.metLevel = 21;
    assert(!XD::match(wrongLevel).matched);

    auto wrongOrigin = mawile;
    wrongOrigin.originGame = 2;
    assert(!XD::match(wrongOrigin).matched);

    auto notFateful = mawile;
    notFateful.fateful = false;
    assert(!XD::match(notFateful).matched);

    auto egg = mawile;
    egg.isEgg = true;
    assert(!XD::match(egg).matched);

    auto shiny = mawile;
    shiny.shiny = true;
    assert(!XD::match(shiny).matched);

    // Pinned PKHeX ShadowTests vector: XD Poochyena with a standard CXD spread.
    const auto poochyenaRng = Legality::Gen3CxdPidIv::analyze(
        0xAF4E3161u, {11, 29, 25, 6, 23, 10});
    assert(poochyenaRng.matched);

    XD::Candidate poochyena{
        261, 15, 10, 162, 2, false, true, false
    };
    const auto poochyenaMatch = XD::match(poochyena);
    assert(poochyenaMatch.matched);
    assert(poochyenaMatch.index == 82);

    std::cout << "Gen III XD shadow encounter identity evidence: PASS\n";
}
