#include "Legality/Gen4PidIvCorrelation.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen4PidIv::Method;
    using Legality::Gen4PidIv::analyze;

    // Same sequential Method-1 correlation used by normal Gen IV static encounters.
    const auto method1 = analyze(0xE97E0000u, {17,19,20,16,13,12});
    assert(method1.method == Method::Method1);
    // This canonical PKHeX Method-1 vector genuinely originates from seed 0.
    // Match state is carried by the method enum, so zero is a valid recovered seed.
    assert(method1.originSeed == 0);

    // A Gen III Method-2 vector must not be accepted as normal Gen IV Method 1.
    assert(analyze(0x5271E97Eu, {2,18,3,12,22,24}).method == Method::None);

    // One IV changed from the Method-1 vector.
    assert(analyze(0xE97E0000u, {17,19,21,16,13,12}).method == Method::None);

    std::cout << "Gen IV normal Method-1 PID/IV correlation: PASS\n";
}
