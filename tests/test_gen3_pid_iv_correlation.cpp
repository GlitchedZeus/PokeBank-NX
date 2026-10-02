#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen3WondercardEggEventTemplate.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3PidIv::Method;
    using Legality::Gen3PidIv::analyze;

    // Cross-check vectors from PKHeX Tests/PKHeX.Core.Tests/Entity/PIDIVTests.cs.
    assert(analyze(0xE97E0000u, {17,19,20,16,13,12}).method == Method::Method1);
    assert(analyze(0x5271E97Eu, {2,18,3,12,22,24}).method == Method::Method2);
    assert(analyze(0x3DD1BB49u, {23,12,31,9,3,3}).method == Method::Method3);
    assert(analyze(0x31B05271u, {2,18,3,5,30,11}).method == Method::Method4);

    assert(analyze(0x815549A2u, {2,26,30,30,11,26}, true).method == Method::Method1Unown);
    assert(analyze(0x8A7B5190u, {14,2,21,30,29,15}, true).method == Method::Method2Unown);
    assert(analyze(0xBB493DD1u, {23,12,31,9,3,3}, true).method == Method::Method3Unown);
    assert(analyze(0x5FA80D70u, {2,6,3,26,4,19}, true).method == Method::Method4Unown);

    // Pinned Gen III Wondercard event eggs accept their native Method 2
    // correlation and the documented Method 1 / Method 4 VBlank outcomes.
    namespace WcEgg = Legality::Gen3WondercardEggEvent;
    const auto m1 = analyze(0xE97E0000u, {17,19,20,16,13,12});
    const auto m2 = analyze(0x5271E97Eu, {2,18,3,12,22,24});
    const auto m4 = analyze(0x31B05271u, {2,18,3,5,30,11});

    WcEgg::Candidate farfetchd{
        83, 4, 0, 255, 4, true, true, {281, 273, 0, 0}
    };
    assert(WcEgg::matches(farfetchd, m1));
    assert(WcEgg::matches(farfetchd, m2));
    assert(WcEgg::matches(farfetchd, m4));

    WcEgg::Candidate pokeParkPichu{
        172, 3, 0, 255, 4, true, true, {84, 204, 266, 0}
    };
    assert(WcEgg::matches(pokeParkPichu, m2));

    auto wrongGame = farfetchd;
    wrongGame.originGame = 3; // PCNY/PCJP Wish eggs are FR/LG only.
    assert(!WcEgg::matches(wrongGame, m2));
    auto wrongMoves = pokeParkPichu;
    wrongMoves.moves = {84, 204, 273, 0};
    assert(!WcEgg::matches(wrongMoves, m2));
    auto notFateful = pokeParkPichu;
    notFateful.fateful = false;
    assert(!WcEgg::matches(notFateful, m2));

    // One IV changed from the known Method 1 vector: no handheld 1/2/3/4 correlation.
    assert(analyze(0xE97E0000u, {17,19,21,16,13,12}).method == Method::None);

    // Deterministic Gen III truncated-roamer vector.
    // Origin seed 0x12345678 => PID 0x84EA0B71, generated IV32 low byte 0x8A.
    const auto roamer =
        Legality::Gen3PidIv::analyzeRoamer(0x84EA0B71u, {10,4,0,0,0,0});
    assert(roamer.method == Method::Method1Roamer);
    assert(roamer.originSeed == 0x12345678u);
    assert(Legality::Gen3PidIv::analyzeRoamer(
               0x84EA0B71u, {10,4,1,0,0,0}).method == Method::None);
    assert(Legality::Gen3PidIv::isRoamerSpecies(243));
    assert(Legality::Gen3PidIv::isRoamerSpecies(381));
    assert(!Legality::Gen3PidIv::isRoamerSpecies(150));

    std::cout << "Gen III PID/IV handheld + roamer LCRNG correlation: PASS\n";
}
