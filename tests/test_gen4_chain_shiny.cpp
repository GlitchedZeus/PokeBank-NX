#include "Legality/Gen4ChainShiny.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen4ChainShiny::analyze;

    // PKHeX pinned regression vector:
    // Chain Shiny, TID16=SID16=0.
    const auto chain = analyze(
        0xA9C1A9C6u, 0u, {22,14,23,24,11,4});
    assert(chain.matched);
    assert(chain.originSeed == 0u);

    // Same PID but one IV changed cannot use the same Chain Shiny correlation.
    assert(!analyze(
        0xA9C1A9C6u, 0u, {22,14,23,24,11,5}).matched);

    // The RNG class itself necessarily requires shiny trainer/PID relation.
    assert(!analyze(
        0xA9C1A9C7u, 0u, {22,14,23,24,11,4}).matched);

    std::cout << "Gen IV Chain Shiny PID/IV correlation: PASS\n";
}
