#include "Legality/Gen3BacdPidIvCorrelation.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3BacdPidIv::Variant;
    using Legality::Gen3BacdPidIv::analyze;
    using Legality::Gen3BacdPidIv::analyzeWithTrainer;

    // Canonical PKHeX unrestricted regular BACD event vector.
    const auto bacd = analyze(0x67DBFC33u, {12, 25, 27, 30, 2, 31});
    assert(bacd.matched());
    assert(bacd.variant == Variant::Regular);

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

    assert(!analyze(0x67DBFC33u, {12, 25, 27, 30, 3, 31}).matched());

    // Canonical PKHeX unrestricted regular-antishiny event vector.
    const auto anti = analyzeWithTrainer(
        0x67DBFC38u, {12, 25, 27, 30, 2, 31}, 1337, 40657);
    assert(anti.matched());
    assert(anti.variant == Variant::RegularAntiShiny);

    // Canonical PKHeX unrestricted force-antishiny event vector.
    const auto forced = analyzeWithTrainer(
        0xBD3DF676u, {0, 15, 5, 4, 21, 5}, 80, 0);
    assert(forced.matched());
    assert(forced.variant == Variant::ForceAntiShiny);

    assert(!analyzeWithTrainer(
        0xBD3DF676u, {0, 15, 5, 4, 21, 6}, 80, 0).matched());

    std::cout << "Gen III BA-CD event PID/IV correlation variants: PASS\n";
}
