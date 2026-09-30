#include "Legality/Gen4RangerManaphy.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4RangerManaphy;

    Candidate egg{
        490, 2, 2, true,
        LocationRanger4, 0, BallPoke, true
    };
    static_assert(matches(egg));

    // Pinned Method-1 regression vector also used by the Gen IV RNG checker.
    constexpr uint32_t pid = 0x5271E97Eu;
    constexpr std::array<uint8_t, 6> ivs{16,13,12,2,18,3};
    constexpr auto normal = analyzePidIv(egg, pid, ivs, 12345, 54321);
    static_assert(normal.evidence == PidEvidence::Method1);

    // Pinned anti-shiny ARNG vector: final PID 0x07578CB7 reverses once to
    // original Method-1 PID 0x5271E97E. Choose trainer IDs for which the
    // original PID is shiny, proving the recipient anti-shiny path.
    constexpr uint16_t tid = 0;
    constexpr uint16_t sid =
        static_cast<uint16_t>(0x5271u ^ 0xE97Eu);
    constexpr auto anti = analyzePidIv(
        egg, 0x07578CB7u, ivs, tid, sid);
    static_assert(anti.evidence == PidEvidence::AntiShinyRecipient);
    static_assert(anti.arngRerolls == 1);

    auto wrongBall = egg;
    wrongBall.ball = 16;
    static_assert(!analyzePidIv(wrongBall, pid, ivs, 12345, 54321).matched());

    std::cout << "Gen IV Ranger Manaphy PID/IV evidence: PASS\n";
}
