#include "Legality/Gen4WildRngCorrelation.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t makeRow(uint8_t type, uint8_t slot,
                           uint8_t minimum, uint8_t maximum,
                           uint8_t rate = 0) {
    return (static_cast<uint64_t>(minimum) << 17) |
           (static_cast<uint64_t>(maximum) << 24) |
           (static_cast<uint64_t>(type) << 31) |
           (static_cast<uint64_t>(slot) << 46) |
           (static_cast<uint64_t>(rate) << 50);
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

    // D/P/Pt Old Rod positive vector: pre-PID seed 331 rolls slot 0, level 7,
    // and passes the 25% rod activation frame.
    constexpr uint32_t fishingJSeed = 331u;
    constexpr uint32_t fishingJPid = sequentialPid(fishingJSeed);
    constexpr uint64_t fishingJ = makeRow(2, 0, 5, 10);
    constexpr auto jf = matchNoLeadRow(false, fishingJ, fishingJSeed, fishingJPid, 7);
    static_assert(jf.method == Method::MethodJFishingNoLead);
    static_assert(jf.slot == 0);
    static_assert(!matchNoLeadRow(false, fishingJ, fishingJSeed, fishingJPid, 8).matched());

    // HG/SS Old Rod positive vector: pre-PID seed 26 rolls slot 0, level 8,
    // and passes the normal/following-Pokemon rod activation path without a lead ability.
    constexpr uint32_t fishingKSeed = 26u;
    constexpr uint32_t fishingKPid = sequentialPid(fishingKSeed);
    constexpr uint64_t fishingK = makeRow(2, 0, 5, 10);
    constexpr auto kf = matchNoLeadRow(true, fishingK, fishingKSeed, fishingKPid, 8);
    static_assert(kf.method == Method::MethodKFishingNoLead);
    static_assert(kf.slot == 0);

    // The same deterministic Method K seed maps Headbutt's 23% slot roll to slot 0
    // and the preceding level roll to level 8.
    constexpr uint64_t headbutt = makeRow(6, 0, 5, 10);
    constexpr auto kh = matchNoLeadRow(true, headbutt, fishingKSeed, fishingKPid, 8);
    static_assert(kh.method == Method::MethodKHeadbuttNoLead);
    static_assert(kh.slot == 0);
    constexpr uint64_t headbuttSpecial = makeRow(7, 0, 5, 10);
    static_assert(matchNoLeadRow(true, headbuttSpecial,
                                 fishingKSeed, fishingKPid, 8).matched());
    static_assert(!matchNoLeadRow(false, headbutt,
                                  fishingKSeed, fishingKPid, 8).matched());

    // HG/SS Rock Smash requires the area encounter rate in addition to slot + level.
    // Seed 26 yields slot 0 and level 8; rate 100 guarantees the normal no-lead trigger.
    constexpr uint64_t rockSmash = makeRow(5, 0, 5, 10, 100);
    constexpr auto kr = matchNoLeadRow(true, rockSmash,
                                       fishingKSeed, fishingKPid, 8);
    static_assert(kr.method == Method::MethodKRockSmashNoLead);
    static_assert(kr.slot == 0);
    constexpr uint64_t impossibleRockSmashRate = makeRow(5, 0, 5, 10, 0);
    static_assert(!matchNoLeadRow(true, impossibleRockSmashRate,
                                  fishingKSeed, fishingKPid, 8).matched());
    static_assert(!matchNoLeadRow(false, rockSmash,
                                  fishingKSeed, fishingKPid, 8).matched());

    // D/P/Pt Honey Trees do not use the ordinary encounter-slot roll.
    // Seed 331 has Prev1 high16=46232, producing level 12 via 5 + rand/0x1745.
    constexpr uint64_t honeyTree = makeRow(9, 0, 5, 15);
    constexpr auto jh = matchNoLeadRow(false, honeyTree,
                                       fishingJSeed, fishingJPid, 12);
    static_assert(jh.method == Method::MethodJHoneyTreeNoLead);
    static_assert(!matchNoLeadRow(false, honeyTree,
                                  fishingJSeed, fishingJPid, 11).matched());
    static_assert(!matchNoLeadRow(true, honeyTree,
                                  fishingJSeed, fishingJPid, 12).matched());

    // Feebas fishing remains fail-closed until the Mt. Coronet tile branch is modeled.
    constexpr uint64_t feebasFishing =
        fishingJ | static_cast<uint64_t>(349u);
    static_assert(!matchNoLeadRow(false, feebasFishing,
                                  fishingJSeed, fishingJPid, 7).matched());

    // The packed wild table reader must retain slot bits emitted by the generator.
    constexpr uint64_t packedSample = 0x20000060c260aULL;
    static_assert(Legality::Gen4Wild::slot(packedSample) == 8);
    constexpr uint64_t rateSample = packedSample | (static_cast<uint64_t>(25) << 50);
    static_assert(Legality::Gen4Wild::rate(rateSample) == 25);

    std::cout << "Gen IV Method J/K no-lead wild RNG evidence: PASS\n";
}
