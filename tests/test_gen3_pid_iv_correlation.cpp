#include "Legality/Gen3PidIvCorrelation.h"

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

    // One IV changed from the known Method 1 vector: no handheld 1/2/3/4 correlation.
    assert(analyze(0xE97E0000u, {17,19,21,16,13,12}).method == Method::None);

    std::cout << "Gen III PID/IV handheld LCRNG correlation: PASS\n";
}
