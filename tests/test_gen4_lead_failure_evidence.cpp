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
    using Legality::Gen4LeadFrame::sequentialPid;

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

    constexpr uint32_t staticSeed = 53u;
    constexpr uint32_t staticPid = sequentialPid(staticSeed);
    constexpr uint64_t jStaticSlot2 = makeRow(0, 2, 5, 5);
    static_assert(matchRow(false, jStaticSlot2, staticSeed, staticPid, 5,
                           Lead::StaticMagnetPull).lead ==
                  Lead::StaticMagnetPull);

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

    constexpr uint64_t kStaticSlot7 = makeRow(0, 7, 5, 5);
    static_assert(matchRow(true, kStaticSlot7, staticSeed, staticPid, 5,
                           Lead::StaticMagnetPull).matched());

    constexpr uint32_t jFishSeed = 492u;
    constexpr uint32_t jFishPid = sequentialPid(jFishSeed);
    constexpr uint64_t jOldRod = makeRow(2, 0, 5, 10);
    constexpr auto jFish = matchRow(false, jOldRod, jFishSeed, jFishPid, 7,
                                    Lead::Synchronize);
    static_assert(jFish.matched());
    static_assert(jFish.slot == 0);
    static_assert(!matchRow(false, jOldRod, jFishSeed, jFishPid, 8,
                            Lead::Synchronize).matched());

    constexpr uint32_t staticFishSeed = 162u;
    constexpr uint32_t staticFishPid = sequentialPid(staticFishSeed);
    constexpr uint64_t jStaticOldRod = makeRow(2, 1, 5, 10);
    constexpr auto staticFish = matchRow(
        false, jStaticOldRod, staticFishSeed, staticFishPid, 9,
        Lead::StaticMagnetPull);
    static_assert(staticFish.matched());
    static_assert(staticFish.slot == 1);

    constexpr uint32_t kRockSeed = 53u;
    constexpr uint32_t kRockPid = sequentialPid(kRockSeed);
    constexpr uint64_t kRock = makeRow(5, 0, 5, 10, 20);
    constexpr auto rock = matchRow(true, kRock, kRockSeed, kRockPid, 10,
                                   Lead::Synchronize);
    static_assert(rock.matched());
    static_assert(rock.slot == 0);

    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchRow(true, safari, kEvenSeed, kEvenPid, 15,
                            Lead::Synchronize).matched());

    // Non-Synchronize Method K reroll histories for HG/SS Bug Catching Contest.
    // Each vector is chosen so the direct origin does not prove the requested
    // lead history; only the earlier origin reached through the minimum-31
    // rejection chain does. Synchronize remains excluded because it requires
    // the separate pinned forceSyncLead nature lock.
    constexpr uint32_t cuteSeed = 280u;
    constexpr uint32_t cutePid = sequentialPid(cuteSeed);
    constexpr uint64_t cuteRow = makeRow(8, 2, 7, 18, 25);
    static_assert(cutePid == 0xCB57F0E6u);
    static_assert(!matchAttempt(
        true, cuteRow, cuteSeed, cutePid, 18, Lead::CuteCharm).matched());
    constexpr auto cuteReroll = matchBugContestReroll(
        true, cuteRow, cuteSeed, cutePid, 18, Lead::CuteCharm, 1);
    static_assert(cuteReroll.lead == Lead::CuteCharm);
    static_assert(cuteReroll.rerollDepth == 1);
    static_assert(!matchBugContestReroll(
        true, cuteRow, cuteSeed, cutePid, 18, Lead::Synchronize, 1).matched());

    constexpr uint32_t pressureFailSeed = 622u;
    constexpr uint32_t pressureFailPid = sequentialPid(pressureFailSeed);
    constexpr uint64_t pressureFailRow = makeRow(8, 6, 7, 18, 25);
    static_assert(pressureFailPid == 0xD091CFD2u);
    static_assert(!matchAttempt(
        true, pressureFailRow, pressureFailSeed, pressureFailPid, 18,
        Lead::PressureHustleVitalSpirit).matched());
    constexpr auto pressureFailReroll = matchBugContestReroll(
        true, pressureFailRow, pressureFailSeed, pressureFailPid, 18,
        Lead::PressureHustleVitalSpirit, 1);
    static_assert(pressureFailReroll.lead ==
                  Lead::PressureHustleVitalSpirit);
    static_assert(pressureFailReroll.rerollDepth == 1);
    constexpr auto intimidateReroll = matchBugContestReroll(
        true, pressureFailRow, pressureFailSeed, pressureFailPid, 18,
        Lead::IntimidateKeenEye, 1);
    static_assert(intimidateReroll.lead == Lead::IntimidateKeenEye);
    static_assert(intimidateReroll.rerollDepth == 1);

    constexpr uint32_t staticFailSeed = 2924u;
    constexpr uint32_t staticFailPid = sequentialPid(staticFailSeed);
    constexpr uint64_t staticFailRow = makeRow(8, 8, 7, 18, 25);
    static_assert(staticFailPid == 0x02BA4508u);
    static_assert(!matchAttempt(
        true, staticFailRow, staticFailSeed, staticFailPid, 13,
        Lead::StaticMagnetPull).matched());
    constexpr auto staticFailReroll = matchBugContestReroll(
        true, staticFailRow, staticFailSeed, staticFailPid, 13,
        Lead::StaticMagnetPull, 1);
    static_assert(staticFailReroll.lead == Lead::StaticMagnetPull);
    static_assert(staticFailReroll.rerollDepth == 1);

    constexpr uint32_t depth2Seed = 32038u;
    constexpr uint32_t depth2Pid = sequentialPid(depth2Seed);
    constexpr uint64_t depth2Row = makeRow(8, 2, 7, 18, 25);
    static_assert(depth2Pid == 0x330697BBu);
    static_assert(!matchBugContestReroll(
        true, depth2Row, depth2Seed, depth2Pid, 18,
        Lead::PressureHustleVitalSpirit, 1).matched());
    constexpr auto depth2 = matchBugContestReroll(
        true, depth2Row, depth2Seed, depth2Pid, 18,
        Lead::PressureHustleVitalSpirit, 2);
    static_assert(depth2.lead == Lead::PressureHustleVitalSpirit);
    static_assert(depth2.rerollDepth == 2);

    constexpr uint32_t depth3Seed = 58747u;
    constexpr uint32_t depth3Pid = sequentialPid(depth3Seed);
    constexpr uint64_t depth3Row = makeRow(8, 1, 7, 18, 25);
    static_assert(depth3Pid == 0xE1BEFE6Fu);
    static_assert(!Legality::Gen4LeadFrame::directMinimum31Satisfied(depth3Seed));
    static_assert(!matchBugContestReroll(
        true, depth3Row, depth3Seed, depth3Pid, 10,
        Lead::PressureHustleVitalSpirit, 1).matched());
    static_assert(!matchBugContestReroll(
        true, depth3Row, depth3Seed, depth3Pid, 10,
        Lead::PressureHustleVitalSpirit, 2).matched());
    constexpr auto depth3 = matchBugContestReroll(
        true, depth3Row, depth3Seed, depth3Pid, 10,
        Lead::PressureHustleVitalSpirit, 3);
    static_assert(depth3.lead == Lead::PressureHustleVitalSpirit);
    static_assert(depth3.rerollDepth == 3);
    constexpr auto depth3Recovered = matchRow(
        true, depth3Row, depth3Seed, depth3Pid, 10,
        Lead::PressureHustleVitalSpirit);
    static_assert(depth3Recovered.lead == Lead::PressureHustleVitalSpirit);
    static_assert(depth3Recovered.rerollDepth == 3);

    static_assert(leadName(Lead::Synchronize)[0] == 'S');
    static_assert(leadName(Lead::IntimidateKeenEye)[0] == 'I');

    assert(matchRow(false, jSlot2, jOddSeed, jOddPid, 5,
                    Lead::PressureHustleVitalSpirit).matched());
    assert(matchRow(true, kStaticSlot7, staticSeed, staticPid, 5,
                    Lead::StaticMagnetPull).matched());

    std::cout << "Gen IV failed lead-effect frame evidence: PASS\n";
}
