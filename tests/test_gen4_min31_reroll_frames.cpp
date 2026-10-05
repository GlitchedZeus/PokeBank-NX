#include "Legality/Gen4BugContestNoLeadEvidence.h"
#include "Legality/Gen4BugContestSynchronizeEvidence.h"
#include "Legality/Gen4LeadFrameEvidence.h"
#include "Legality/Gen4SafariNoLeadEvidence.h"

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
    using namespace Legality::Gen4LeadFrame;
    using Legality::Gen3PidIv::Detail::prev;

    constexpr uint32_t seed = 0x12345678u;
    constexpr uint32_t p1 = prev(seed);
    constexpr uint32_t p2 = prev(p1);
    constexpr uint32_t p3 = prev(p2);
    constexpr uint32_t p4 = prev(p3);
    constexpr uint32_t p5 = prev(p4);
    static_assert(previousRerollIv2Word(seed) ==
                  static_cast<uint16_t>((p1 >> 16) & 0x7FFFu));
    static_assert(previousRerollIv1Word(seed) ==
                  static_cast<uint16_t>((p2 >> 16) & 0x7FFFu));
    static_assert(previousRerollNatureSeed(seed) == p5);

    static_assert(directMinimum31Satisfied(3u));
    static_assert(minimum31IvChainAllows(3u, 0));
    static_assert(minimum31IvChainAllows(3u, 1));
    static_assert(minimum31IvChainAllows(3u, 2));

    static_assert(!directMinimum31Satisfied(2u));
    static_assert(!minimum31IvChainAllows(2u, 0));
    static_assert(!minimum31IvChainAllows(2u, 1));
    static_assert(!minimum31IvChainAllows(2u, 2));
    static_assert(minimum31IvChainAllows(2u, 3));
    static_assert(!minimum31IvChainAllows(2u, 4));

    uint32_t cursor = 2u;
    for (int i = 0; i < 3; ++i) {
        assert(previousRerollAttemptRejected(cursor));
        cursor = previousRerollNatureSeed(cursor);
    }

    using namespace Legality::Gen4BugContestNoLead;

    constexpr uint32_t depth1Seed = 280u;
    constexpr uint32_t depth1Pid = sequentialPid(depth1Seed);
    constexpr uint64_t depth1Row = makeRow(8, 7, 7, 18, 25);
    static_assert(depth1Pid == 0xCB57F0E6u);
    static_assert(directMinimum31Satisfied(depth1Seed));
    static_assert(!matchAttempt(depth1Row, depth1Seed, depth1Pid, 16).matched());
    constexpr auto depth1 = matchReroll(
        depth1Row, depth1Seed, depth1Pid, 16, 1);
    static_assert(depth1.matched());
    static_assert(depth1.rerollDepth == 1);
    static_assert(depth1.slot == 7);
    static_assert(depth1.level == 16);
    static_assert(match(depth1Row, depth1Seed, depth1Pid, 16).rerollDepth == 1);

    constexpr uint32_t depth2Seed = 280u;
    constexpr uint32_t depth2Pid = sequentialPid(depth2Seed);
    constexpr uint64_t depth2Row = makeRow(8, 3, 7, 18, 25);
    static_assert(depth2Pid == 0xCB57F0E6u);
    static_assert(directMinimum31Satisfied(depth2Seed));
    static_assert(!matchAttempt(depth2Row, depth2Seed, depth2Pid, 14).matched());
    static_assert(!matchReroll(
        depth2Row, depth2Seed, depth2Pid, 14, 1).matched());
    constexpr auto depth2 = matchReroll(
        depth2Row, depth2Seed, depth2Pid, 14, 2);
    static_assert(depth2.matched());
    static_assert(depth2.rerollDepth == 2);
    static_assert(depth2.slot == 3);
    static_assert(depth2.level == 14);
    static_assert(match(depth2Row, depth2Seed, depth2Pid, 14).rerollDepth == 2);

    constexpr uint32_t depth3Seed = 24094u;
    constexpr uint32_t depth3Pid = sequentialPid(depth3Seed);
    constexpr uint64_t depth3Row = makeRow(8, 5, 7, 18, 25);
    static_assert(depth3Pid == 0x6D3F8609u);
    static_assert(!directMinimum31Satisfied(depth3Seed));
    static_assert(!matchReroll(
        depth3Row, depth3Seed, depth3Pid, 18, 1).matched());
    static_assert(!matchReroll(
        depth3Row, depth3Seed, depth3Pid, 18, 2).matched());
    constexpr auto depth3 = matchReroll(
        depth3Row, depth3Seed, depth3Pid, 18, 3);
    static_assert(depth3.matched());
    static_assert(depth3.rerollDepth == 3);
    static_assert(depth3.slot == 5);
    static_assert(depth3.level == 18);
    static_assert(match(depth3Row, depth3Seed, depth3Pid, 18).rerollDepth == 3);

    constexpr uint32_t syncDepth1Seed = 1469u;
    constexpr uint32_t syncDepth1Pid = sequentialPid(syncDepth1Seed);
    constexpr uint64_t syncDepth1Row = makeRow(8, 5, 7, 18, 25);
    static_assert(syncDepth1Pid == 0xC88E6EF0u);
    static_assert((syncDepth1Pid % 25u) == 12u);
    static_assert(!Legality::Gen4BugContestSynchronize::matchSuccessfulAttempt(
        syncDepth1Row, syncDepth1Seed, syncDepth1Pid, 9, 12).matched());
    static_assert(!Legality::Gen4BugContestSynchronize::matchSuccessfulAttempt(
        syncDepth1Row, syncDepth1Seed, syncDepth1Pid, 9, 11).matched());
    static_assert(!Legality::Gen4BugContestSynchronize::matchReroll(
        syncDepth1Row, syncDepth1Seed, syncDepth1Pid, 9, 0).matched());
    static_assert(!Legality::Gen4BugContestSynchronize::matchReroll(
        syncDepth1Row, syncDepth1Seed, syncDepth1Pid, 9, 4).matched());
    constexpr auto syncDepth1 =
        Legality::Gen4BugContestSynchronize::matchReroll(
            syncDepth1Row, syncDepth1Seed, syncDepth1Pid, 9, 1);
    static_assert(syncDepth1.matched());
    static_assert(syncDepth1.lockedNature == 12);
    static_assert(syncDepth1.rerollDepth == 1);
    static_assert(syncDepth1.slot == 5);
    static_assert(syncDepth1.level == 9);

    constexpr uint64_t syncZeroRateRow = makeRow(8, 5, 7, 18, 0);
    static_assert(!Legality::Gen4BugContestSynchronize::matchReroll(
        syncZeroRateRow, syncDepth1Seed, syncDepth1Pid, 9, 1).matched());
    static_assert(!Legality::Gen4BugContestSynchronize::matchReroll(
        syncDepth1Row, syncDepth1Seed, syncDepth1Pid, 9, 3).matched());

    constexpr uint32_t syncDepth2Seed = 47914u;
    constexpr uint32_t syncDepth2Pid = sequentialPid(syncDepth2Seed);
    constexpr uint64_t syncDepth2Row = makeRow(8, 1, 7, 18, 25);
    static_assert(syncDepth2Pid == 0x9EF6A5D2u);
    static_assert((syncDepth2Pid % 25u) == 9u);
    static_assert(!Legality::Gen4BugContestSynchronize::matchReroll(
        syncDepth2Row, syncDepth2Seed, syncDepth2Pid, 10, 1).matched());
    constexpr auto syncDepth2 =
        Legality::Gen4BugContestSynchronize::matchReroll(
            syncDepth2Row, syncDepth2Seed, syncDepth2Pid, 10, 2);
    static_assert(syncDepth2.matched());
    static_assert(syncDepth2.lockedNature == 9);
    static_assert(syncDepth2.rerollDepth == 2);
    static_assert(syncDepth2.slot == 1);
    static_assert(syncDepth2.level == 10);

    constexpr uint32_t syncDepth3Seed = 6712803u;
    constexpr uint32_t syncDepth3Pid = sequentialPid(syncDepth3Seed);
    constexpr uint64_t syncDepth3Row = makeRow(8, 8, 7, 18, 25);
    static_assert(syncDepth3Pid == 0x414201AAu);
    static_assert((syncDepth3Pid % 25u) == 17u);
    static_assert(!directMinimum31Satisfied(syncDepth3Seed));
    static_assert(!Legality::Gen4BugContestSynchronize::matchReroll(
        syncDepth3Row, syncDepth3Seed, syncDepth3Pid, 9, 1).matched());
    static_assert(!Legality::Gen4BugContestSynchronize::matchReroll(
        syncDepth3Row, syncDepth3Seed, syncDepth3Pid, 9, 2).matched());
    constexpr auto syncDepth3 =
        Legality::Gen4BugContestSynchronize::matchReroll(
            syncDepth3Row, syncDepth3Seed, syncDepth3Pid, 9, 3);
    static_assert(syncDepth3.matched());
    static_assert(syncDepth3.lockedNature == 17);
    static_assert(syncDepth3.rerollDepth == 3);
    static_assert(syncDepth3.slot == 8);
    static_assert(syncDepth3.level == 9);

    // HG/SS Safari is fixed-level Method K with rand%10 slot selection and the
    // same four-attempt minimum-31 reroll rule. The grass vectors prove each
    // reroll depth without relying on Safari fishing activation.
    constexpr uint64_t safariDepth1Row = makeRow(10, 7, 15, 15, 6);
    static_assert(!Legality::Gen4SafariNoLead::matchAttempt(
        safariDepth1Row, depth1Seed, depth1Pid, 15).matched());
    constexpr auto safariDepth1 = Legality::Gen4SafariNoLead::matchReroll(
        safariDepth1Row, depth1Seed, depth1Pid, 15, 1);
    static_assert(safariDepth1.matched());
    static_assert(safariDepth1.slot == 7);
    static_assert(safariDepth1.rerollDepth == 1);
    static_assert(!safariDepth1.suctionCups);

    constexpr uint64_t safariDepth2Row = makeRow(10, 9, 15, 15, 6);
    static_assert(!Legality::Gen4SafariNoLead::matchAttempt(
        safariDepth2Row, depth2Seed, depth2Pid, 15).matched());
    static_assert(!Legality::Gen4SafariNoLead::matchReroll(
        safariDepth2Row, depth2Seed, depth2Pid, 15, 1).matched());
    constexpr auto safariDepth2 = Legality::Gen4SafariNoLead::matchReroll(
        safariDepth2Row, depth2Seed, depth2Pid, 15, 2);
    static_assert(safariDepth2.matched());
    static_assert(safariDepth2.slot == 9);
    static_assert(safariDepth2.rerollDepth == 2);

    constexpr uint64_t safariDepth3Row = makeRow(10, 1, 15, 15, 6);
    static_assert(!Legality::Gen4SafariNoLead::matchReroll(
        safariDepth3Row, depth3Seed, depth3Pid, 15, 1).matched());
    static_assert(!Legality::Gen4SafariNoLead::matchReroll(
        safariDepth3Row, depth3Seed, depth3Pid, 15, 2).matched());
    constexpr auto safariDepth3 = Legality::Gen4SafariNoLead::matchReroll(
        safariDepth3Row, depth3Seed, depth3Pid, 15, 3);
    static_assert(safariDepth3.matched());
    static_assert(safariDepth3.slot == 1);
    static_assert(safariDepth3.rerollDepth == 3);

    // Safari Old Rod has a real activation distinction: at the original attempt
    // for this depth-1 vector, the rod roll is 95. With the HG/SS +50 following
    // Pokemon bonus, normal Old Rod activation ends at 74 and no-lead reaches
    // this frame only through the Suction Cups / Sticky Hold fallback.
    constexpr uint32_t safariFishSeed = 44388u;
    constexpr uint32_t safariFishPid = sequentialPid(safariFishSeed);
    constexpr uint64_t safariOldRodRow = makeRow(12, 7, 20, 20, 6);
    static_assert(safariFishPid == 0xD35BB476u);
    static_assert(!Legality::Gen4SafariNoLead::matchAttempt(
        safariOldRodRow, safariFishSeed, safariFishPid, 20).matched());
    constexpr auto safariFish = Legality::Gen4SafariNoLead::matchReroll(
        safariOldRodRow, safariFishSeed, safariFishPid, 20, 1);
    static_assert(safariFish.matched());
    static_assert(safariFish.slot == 7);
    static_assert(safariFish.rerollDepth == 1);
    static_assert(safariFish.suctionCups);

    static_assert(!Legality::Gen4SafariNoLead::matchReroll(
        safariDepth1Row, depth1Seed, depth1Pid, 14, 1).matched());
    static_assert(!Legality::Gen4SafariNoLead::matchReroll(
        safariDepth1Row, depth1Seed, depth1Pid, 15, 0).matched());
    static_assert(!Legality::Gen4SafariNoLead::matchReroll(
        safariDepth1Row, depth1Seed, depth1Pid, 15, 4).matched());

    std::cout << "Gen IV minimum-31 reroll frame geometry: PASS\n";
}
