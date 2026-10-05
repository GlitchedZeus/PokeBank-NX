#include "Legality/Gen4LeadFailureEvidence.h"

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
    using namespace Legality::Gen4LeadFailure;
    using Legality::Gen4WildRng::sequentialPid;

    // D/P/Pt fixed-level Grass vectors for the post-nature failure branches.
    // Seeds were independently derived from the pinned LCRNG ordering.
    constexpr uint32_t jOddSeed = 13u;
    constexpr uint32_t jOddPid = sequentialPid(jOddSeed);
    constexpr uint64_t jSlot2 = makeRow(0, 2, 5, 5);
    static_assert(matchRow(false, jSlot2, jOddSeed, jOddPid, 5,
                           Lead::CuteCharm).lead == Lead::CuteCharm);
    static_assert(matchRow(false, jSlot2, jOddSeed, jOddPid, 5,
                           Lead::PressureHustleVitalSpirit).lead ==
                  Lead::PressureHustleVitalSpirit);
    static_assert(matchRow(false, jSlot2, jOddSeed, jOddPid, 5,
                           Lead::IntimidateKeenEye).lead ==
                  Lead::IntimidateKeenEye);

    constexpr uint32_t jHighSeed = 81u;
    constexpr uint32_t jHighPid = sequentialPid(jHighSeed);
    constexpr uint64_t jSlot0 = makeRow(0, 0, 5, 5);
    static_assert(matchRow(false, jSlot0, jHighSeed, jHighPid, 5,
                           Lead::Synchronize).lead == Lead::Synchronize);

    // Static/Magnet Pull failure has different ordering: for fixed-level Grass,
    // Prev2 is the failed proc and Prev1 is the ordinary ESV. Method J seed 53
    // therefore reaches slot 2, not the post-nature slot used above.
    constexpr uint32_t staticSeed = 53u;
    constexpr uint32_t staticPid = sequentialPid(staticSeed);
    constexpr uint64_t jStaticSlot2 = makeRow(0, 2, 5, 5);
    static_assert(matchRow(false, jStaticSlot2, staticSeed, staticPid, 5,
                           Lead::StaticMagnetPull).lead ==
                  Lead::StaticMagnetPull);

    // HG/SS uses low-bit parity for the post-nature predicates.
    constexpr uint32_t kOddSeed = 13u;
    constexpr uint32_t kOddPid = sequentialPid(kOddSeed);
    constexpr uint64_t kSlot2 = makeRow(0, 2, 5, 5);
    static_assert(matchRow(true, kSlot2, kOddSeed, kOddPid, 5,
                           Lead::CuteCharm).matched());
    static_assert(matchRow(true, kSlot2, kOddSeed, kOddPid, 5,
                           Lead::PressureHustleVitalSpirit).matched());
    static_assert(matchRow(true, kSlot2, kOddSeed, kOddPid, 5,
                           Lead::IntimidateKeenEye).matched());

    constexpr uint32_t kEvenSeed = 53u;
    constexpr uint32_t kEvenPid = sequentialPid(kEvenSeed);
    constexpr uint64_t kSlot4 = makeRow(0, 4, 5, 5);
    static_assert(matchRow(true, kSlot4, kEvenSeed, kEvenPid, 5,
                           Lead::Synchronize).matched());

    // The same seed's Method K Static/Magnet failure ordering uses Prev1 as ESV,
    // which maps to slot 7 after the odd Prev2 proc fails to activate.
    constexpr uint64_t kStaticSlot7 = makeRow(0, 7, 5, 5);
    static_assert(matchRow(true, kStaticSlot7, staticSeed, staticPid, 5,
                           Lead::StaticMagnetPull).matched());

    // Post-nature failure fishing shifts lead, level, slot and activation one frame.
    // Method J seed 492 reaches Old Rod slot 0, level 7, and a normal 19% hook.
    constexpr uint32_t jFishSeed = 492u;
    constexpr uint32_t jFishPid = sequentialPid(jFishSeed);
    constexpr uint64_t jOldRod = makeRow(2, 0, 5, 10);
    constexpr auto jFish = matchRow(false, jOldRod, jFishSeed, jFishPid, 7,
                                    Lead::Synchronize);
    static_assert(jFish.matched());
    static_assert(jFish.slot == 0);
    static_assert(!matchRow(false, jOldRod, jFishSeed, jFishPid, 8,
                            Lead::Synchronize).matched());

    // Static/Magnet Pull failure around a random-level Method J encounter is
    // Prev3=proc, Prev2=ESV, Prev1=level, then activation. Seed 162 proves the
    // independently derived Old Rod slot 1 / level 9 / normal 23% hook vector.
    constexpr uint32_t staticFishSeed = 162u;
    constexpr uint32_t staticFishPid = sequentialPid(staticFishSeed);
    constexpr uint64_t jStaticOldRod = makeRow(2, 1, 5, 10);
    constexpr auto staticFish = matchRow(
        false, jStaticOldRod, staticFishSeed, staticFishPid, 9,
        Lead::StaticMagnetPull);
    static_assert(staticFish.matched());
    static_assert(staticFish.slot == 1);

    // HG/SS Rock Smash similarly uses a shifted normal activation frame. The
    // failed Synchronize lead cannot simultaneously claim Illuminate.
    constexpr uint32_t kRockSeed = 53u;
    constexpr uint32_t kRockPid = sequentialPid(kRockSeed);
    constexpr uint64_t kRock = makeRow(5, 0, 5, 10, 20);
    constexpr auto rock = matchRow(true, kRock, kRockSeed, kRockPid, 10,
                                   Lead::Synchronize);
    static_assert(rock.matched());
    static_assert(rock.slot == 0);

    // Keep the complicated reroll/deadlock families unresolved in this tranche.
    constexpr uint64_t contest = makeRow(8, 0, 7, 18, 25);
    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchRow(true, contest, kEvenSeed, kEvenPid, 7,
                            Lead::PressureHustleVitalSpirit).matched());
    static_assert(!matchRow(true, safari, kEvenSeed, kEvenPid, 15,
                            Lead::Synchronize).matched());

    static_assert(leadName(Lead::Synchronize)[0] == 'S');
    static_assert(leadName(Lead::IntimidateKeenEye)[0] == 'I');

    // Runtime asserts keep both ordering families exercised under ASan/UBSan.
    assert(matchRow(false, jSlot2, jOddSeed, jOddPid, 5,
                    Lead::PressureHustleVitalSpirit).matched());
    assert(matchRow(true, kStaticSlot7, staticSeed, staticPid, 5,
                    Lead::StaticMagnetPull).matched());

    std::cout << "Gen IV failed lead-effect frame evidence: PASS\n";
}
