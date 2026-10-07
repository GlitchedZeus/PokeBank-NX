#include "Legality/Gen3HoennSafariRngEvidence.h"

#include <array>
#include <cassert>
#include <cstdint>

using namespace Legality::Gen3HoennSafariRng;

namespace {

constexpr uint32_t rewindIterative(uint32_t seed, int count) noexcept {
    for (int i = 0; i < count; ++i)
        seed = prev(seed);
    return seed;
}

} // namespace

int main() {
    static_assert(kPrev300Mult == 0xC048C851u);
    static_assert(kPrev300Add == 0x196302B4u);

    // Lock the pinned PKHeX affine shortcut against the underlying Gen III
    // reverse LCRNG for several unrelated seeds.
    constexpr std::array<uint32_t, 8> seeds{{
        0x00000000u,
        0x00000001u,
        0x12345678u,
        0xDEADBEEFu,
        0x80000000u,
        0xFFFFFFFFu,
        0xA5A5A5A5u,
        0x5A5A5A5Au,
    }};
    for (uint32_t seed : seeds)
        assert(rewindNaturePreferenceBlock(seed) == rewindIterative(seed, 300));

    // PKHeX's Safari block activation is exactly an 80/100 upper-half test.
    for (uint32_t roll = 0; roll < 100; ++roll) {
        const uint32_t seed = roll << 16;
        assert(safariBlockProc(seed) == (roll < 80));
    }

    // Values above 99 still reduce the raw upper half modulo 100.
    assert(safariBlockProc(100u << 16));
    assert(!safariBlockProc(180u << 16));
    assert(!safariBlockProc(199u << 16));

    // The original nature roll is captured before the Hoenn Safari no-block
    // one-call rewind, matching pinned MethodH's p0 ordering.
    constexpr uint32_t natureSeed = 0x12345678u;
    assert(natureRoll(natureSeed) == 0x1234u);
    assert(nature(natureSeed) == (0x1234u % 25u));
    assert(noBlockFrameSeed(natureSeed) == prev(natureSeed));

    // Only R/S/E at the pinned Hoenn Safari location are eligible for this
    // special frame transformation. FR/LG Safari does not use this branch.
    for (const auto game : {"ruby_gba", "sapphire_gba", "emerald_gba"}) {
        const auto ev = evidence(game, 57, natureSeed);
        assert(ev.applicable);
        assert(ev.natureSeed == natureSeed);
        assert(ev.blockSeed == rewindNaturePreferenceBlock(natureSeed));
        assert(ev.blockProc == safariBlockProc(ev.blockSeed));
        assert(ev.noBlockSeed == prev(natureSeed));
        assert(ev.originalNatureRoll == natureRoll(natureSeed));
        assert(ev.originalNature == nature(natureSeed));
    }

    assert(!evidence("firered_gba", 136, natureSeed).applicable);
    assert(!evidence("leafgreen_gba", 136, natureSeed).applicable);
    assert(!evidence("ruby_gba", 136, natureSeed).applicable);
    assert(!evidence("unknown", 57, natureSeed).applicable);

    return 0;
}
