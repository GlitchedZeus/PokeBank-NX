#include "Legality/Gen4StaticMagnetLeadEvidence.h"

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
constexpr uint32_t makeMeta(uint8_t magnetIndex, uint8_t magnetCount,
                            uint8_t staticIndex, uint8_t staticCount) {
    return static_cast<uint32_t>(magnetIndex) |
           (static_cast<uint32_t>(magnetCount) << 8) |
           (static_cast<uint32_t>(staticIndex) << 16) |
           (static_cast<uint32_t>(staticCount) << 24);
}
}

int main() {
    using namespace Legality::Gen4StaticMagnetLead;
    using Legality::Gen4WildRng::sequentialPid;

    static_assert(magnetIndex(makeMeta(2, 4, 1, 3)) == 2);
    static_assert(magnetCount(makeMeta(2, 4, 1, 3)) == 4);
    static_assert(staticIndex(makeMeta(2, 4, 1, 3)) == 1);
    static_assert(staticCount(makeMeta(2, 4, 1, 3)) == 3);

    // Static is checked before Magnet Pull when both metadata predicates happen
    // to match the same roll, mirroring pinned IMagnetStatic exactly.
    static_assert(attractedLead(makeMeta(0, 2, 0, 2), 4) == Lead::Static);
    static_assert(attractedLead(makeMeta(1, 2, 0, 0), 5) == Lead::MagnetPull);
    static_assert(attractedLead(makeMeta(0, 0, 0, 0), 5) == Lead::None);

    constexpr uint64_t grass = makeRow(0, 0, 5, 5);

    // Method J fixed-level Grass is deliberately a discriminating vector.
    // Seed 53 has Prev3=14714 (high bit clear => proc passes) but Prev2=61367
    // (high bit set => would fail). Prev1=30387 selects Static index 0 of 3
    // and Magnet Pull index 3 of 4. This catches the Method-J-only Prev3 proc.
    constexpr uint32_t jGrassSeed = 53u;
    constexpr uint32_t jGrassPid = sequentialPid(jGrassSeed);
    constexpr auto jStatic = matchRow(
        false, grass, makeMeta(0, 0, 0, 3),
        jGrassSeed, jGrassPid, 5);
    static_assert(jStatic.lead == Lead::Static);
    constexpr auto jMagnet = matchRow(
        false, grass, makeMeta(3, 4, 0, 0),
        jGrassSeed, jGrassPid, 5);
    static_assert(jMagnet.lead == Lead::MagnetPull);

    // If Method J incorrectly used Prev2 like Method K, seed 53 would not match.
    static_assert((61367u >> 15) == 1u);
    static_assert((14714u >> 15) == 0u);

    // Method K fixed-level Grass uses Prev2 for the proc and Prev1 for the
    // attracted-slot roll. Seed 13 keeps an independent low-bit-parity vector.
    constexpr uint32_t kGrassSeed = 13u;
    constexpr uint32_t kGrassPid = sequentialPid(kGrassSeed);
    constexpr auto kStatic = matchRow(
        true, grass, makeMeta(0, 0, 0, 3),
        kGrassSeed, kGrassPid, 5);
    static_assert(kStatic.lead == Lead::Static);
    constexpr auto kMagnet = matchRow(
        true, grass, makeMeta(2, 4, 0, 0),
        kGrassSeed, kGrassPid, 5);
    static_assert(kMagnet.lead == Lead::MagnetPull);

    // Wrong source-area eligible-slot metadata cannot become positive evidence.
    static_assert(!matchRow(
        false, grass, makeMeta(0, 0, 1, 3),
        jGrassSeed, jGrassPid, 5).matched());
    static_assert(!matchRow(
        true, grass, makeMeta(1, 4, 0, 0),
        kGrassSeed, kGrassPid, 5).matched());

    // Random-level Method J Old Rod ordering: seed 13 gives Prev1 level 5,
    // Prev2 attracted-slot roll 32448 (mod 3 == 0), Prev3 passing proc, and a
    // normal 17% hook on the subsequent activation frame.
    constexpr uint32_t jFishSeed = 13u;
    constexpr uint32_t jFishPid = sequentialPid(jFishSeed);
    constexpr uint64_t jOldRod = makeRow(2, 0, 5, 10);
    constexpr auto jFishingStatic = matchRow(
        false, jOldRod, makeMeta(0, 0, 0, 3),
        jFishSeed, jFishPid, 5);
    static_assert(jFishingStatic.lead == Lead::Static);
    static_assert(!matchRow(
        false, jOldRod, makeMeta(0, 0, 0, 3),
        jFishSeed, jFishPid, 6).matched());

    // Method K seed 53 gives Prev1 level 8, Prev2 roll 61367 (mod 3 == 2),
    // Prev3 even/passing proc and a normal Old Rod activation roll of 4.
    constexpr uint32_t kFishSeed = 53u;
    constexpr uint32_t kFishPid = sequentialPid(kFishSeed);
    constexpr uint64_t kOldRod = makeRow(2, 0, 5, 10);
    constexpr auto kFishingStatic = matchRow(
        true, kOldRod, makeMeta(0, 0, 2, 3),
        kFishSeed, kFishPid, 8);
    static_assert(kFishingStatic.lead == Lead::Static);

    // Bug Contest/Safari remain outside this tranche until their reroll/deadlock
    // rules are explicitly reconstructed.
    constexpr uint64_t contest = makeRow(8, 0, 7, 18, 25);
    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchRow(
        true, contest, makeMeta(0, 0, 0, 3),
        kGrassSeed, kGrassPid, 7).matched());
    static_assert(!matchRow(
        true, safari, makeMeta(0, 0, 0, 3),
        kGrassSeed, kGrassPid, 15).matched());

    assert(matchRow(false, grass, makeMeta(3, 4, 0, 0),
                    jGrassSeed, jGrassPid, 5).matched());
    assert(matchRow(true, kOldRod, makeMeta(0, 0, 2, 3),
                    kFishSeed, kFishPid, 8).matched());

    std::cout << "Gen IV Static/Magnet Pull lead evidence: PASS\n";
}
