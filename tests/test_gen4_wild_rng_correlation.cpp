#include "Legality/Gen4WildRngCorrelation.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t makeRow(uint8_t type, uint8_t slot,
                           uint8_t minimum, uint8_t maximum) {
    return (static_cast<uint64_t>(minimum) << 17) |
           (static_cast<uint64_t>(maximum) << 24) |
           (static_cast<uint64_t>(type) << 31) |
           (static_cast<uint64_t>(slot) << 46);
}
}

int main() {
    using namespace Legality::Gen4WildRng;

    // PKHeX MethodJChecks: ModestFlawless (TimidFlawless ^ 0x80000000)
    // can reach Grass slot 6 with LeadRequired.None.
    constexpr uint32_t methodJSeed = 0x469FB838u;
    constexpr uint32_t methodJPid = sequentialPid(methodJSeed);
    constexpr uint64_t jSlot6 = makeRow(0, 6, 5, 5);
    constexpr auto j = matchNoLeadRow(false, jSlot6, methodJSeed, methodJPid, 5);
    static_assert(j.method == Method::MethodJNoLead);
    static_assert(j.slot == 6);

    // PKHeX marks slot 3 impossible for the same Method J pre-PID seed.
    constexpr uint64_t jSlot3 = makeRow(0, 3, 5, 5);
    static_assert(!matchNoLeadRow(false, jSlot3, methodJSeed, methodJPid, 5).matched());

    // Deterministic HG/SS Method K no-lead vector. Seed 0x0000000D has
    // nature roll 0 and maps the preceding Grass slot roll to slot 4.
    constexpr uint32_t methodKSeed = 0x0000000Du;
    constexpr uint32_t methodKPid = sequentialPid(methodKSeed);
    static_assert((methodKPid % 25u) == 0);
    constexpr uint64_t kSlot4 = makeRow(0, 4, 5, 5);
    constexpr auto k = matchNoLeadRow(true, kSlot4, methodKSeed, methodKPid, 5);
    static_assert(k.method == Method::MethodKNoLead);
    static_assert(k.slot == 4);

    constexpr uint64_t kWrong = makeRow(0, 5, 5, 5);
    static_assert(!matchNoLeadRow(true, kWrong, methodKSeed, methodKPid, 5).matched());

    // The conservative first tranche deliberately refuses unsupported encounter types.
    constexpr uint64_t fishing = makeRow(2, 0, 5, 10);
    static_assert(!matchNoLeadRow(false, fishing, methodJSeed, methodJPid, 5).matched());

    // The packed wild table reader must retain slot bits emitted by the generator.
    constexpr uint64_t packedSample = 0x20000060c260aULL;
    static_assert(Legality::Gen4Wild::slot(packedSample) == 8);

    std::cout << "Gen IV Method J/K no-lead wild RNG evidence: PASS\n";
}
