#include "Legality/Gen3CxdPidIvCorrelation.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3CxdPidIv::analyze;

    const auto cxd = analyze(0x0985A297u, {6, 1, 0, 7, 17, 7});
    assert(cxd.matched);

    uint32_t seed = cxd.originSeed;
    const auto next = [](uint32_t s) constexpr {
        return s * 0x000343FDu + 0x00269EC3u;
    };
    seed = next(seed);
    const uint32_t iv1 = (seed >> 16) & 0x7FFFu;
    seed = next(seed);
    const uint32_t iv2 = (seed >> 16) & 0x7FFFu;
    seed = next(seed); // ability
    seed = next(seed);
    const uint32_t pidHigh = seed >> 16;
    seed = next(seed);
    const uint32_t pidLow = seed >> 16;

    assert(iv1 == (6u | (1u << 5) | (0u << 10)));
    assert(iv2 == (7u | (17u << 5) | (7u << 10)));
    assert(((pidHigh << 16) | pidLow) == 0x0985A297u);

    assert(!analyze(0x0985A297u, {6, 1, 0, 7, 18, 7}).matched);

    std::cout << "Gen III Colosseum/XD PID/IV correlation: PASS\n";
}
