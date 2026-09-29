#include "Legality/Gen4MysteryGiftPid.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen4MysteryGiftPid::analyze;

    // Pinned PKHeX PIDIV regression vector.
    // Final anti-shiny PID 0x07578CB7 is one ARNG reroll from
    // Method-1 original PID 0x5271E97E.
    const auto result = analyze(
        0x07578CB7u, {16,13,12,2,18,3});
    assert(result.matched);
    assert(result.rerolls == 1);
    assert(result.originalPid == 0x5271E97Eu);
    assert(result.originSeed == 0x00006073u);

    // A changed IV breaks the original PID/IV correlation.
    assert(!analyze(
        0x07578CB7u, {16,13,12,2,18,4}).matched);

    // An ordinary Method-1 PID is not itself evidence of an anti-shiny reroll.
    assert(!analyze(
        0x5271E97Eu, {16,13,12,2,18,3}).matched);

    std::cout << "Gen IV Mystery Gift anti-shiny PID correlation: PASS\n";
}
