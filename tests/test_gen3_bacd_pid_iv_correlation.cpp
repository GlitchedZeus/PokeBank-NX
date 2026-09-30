#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3BacdPidIv::analyze;

    // Canonical PKHeX unrestricted regular BACD event vector.
    const auto bacd = analyze(0x67DBFC33u, {12, 25, 27, 30, 2, 31});
    assert(bacd.matched);

    // Replay from the recovered origin: A/B generate PID, C/D generate IV halves.
    uint32_t seed = bacd.originSeed;
    const auto next = [](uint32_t s) constexpr {
        return s * 0x41C64E6Du + 0x00006073u;
    };
    seed = next(seed);
    const uint32_t a16 = seed >> 16;
    seed = next(seed);
    const uint32_t b16 = seed >> 16;
    seed = next(seed);
    const uint32_t iv1 = (seed >> 16) & 0x7FFFu;
    seed = next(seed);
    const uint32_t iv2 = (seed >> 16) & 0x7FFFu;

    assert(((a16 << 16) | b16) == 0x67DBFC33u);
    assert(iv1 == (12u | (25u << 5) | (27u << 10)));
    assert(iv2 == (30u | (2u << 5) | (31u << 10)));

    assert(!analyze(0x67DBFC33u, {12, 25, 27, 30, 3, 31}).matched);

    std::cout << "Gen III regular BACD event PID/IV correlation: PASS\n";
}
