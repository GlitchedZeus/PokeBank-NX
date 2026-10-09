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
    using Legality::Gen4LeadFrame::sequentialPid;

    static_assert(magnetIndex(makeMeta(2, 4, 1, 3)) == 2);
    static_assert(magnetCount(makeMeta(2, 4, 1, 3)) == 4);
    static_assert(staticIndex(makeMeta(2, 4, 1, 3)) == 1);
    static_assert(staticCount(makeMeta(2, 4, 1, 3)) == 3);
    static_assert(attractedLead(makeMeta(0, 2, 0, 2), 4) == Lead::Static);
    static_assert(attractedLead(makeMeta(1, 2, 0, 0), 5) == Lead::MagnetPull);
    static_assert(attractedLead(makeMeta(0, 0, 0, 0), 5) == Lead::None);

    constexpr uint64_t grass = makeRow(0, 0, 5, 5);
    constexpr uint32_t jGrassSeed = 53u;
    constexpr uint32_t jGrassPid = sequentialPid(jGrassSeed);
    constexpr auto jStatic = matchRow(
        false, grass, makeMeta(0, 0, 0, 3),
        jGrassSeed, jGrassPid, 5);
    static_assert(jStatic.lead == Lead::Static);
    static_assert(jStatic.rerollDepth == 0);
    constexpr auto jMagnet = matchRow(
        false, grass, makeMeta(3, 4, 0, 0),
        jGrassSeed, jGrassPid, 5);
    static_assert(jMagnet.lead == Lead::MagnetPull);
    static_assert((61367u >> 15) == 1u);
    static_assert((14714u >> 15) == 0u);

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

    static_assert(attractedLead(makeMeta(0, 0, 1, 3), 30387u) == Lead::None);
    static_assert(attractedLead(makeMeta(1, 4, 0, 0), 10662u) == Lead::None);

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

    constexpr uint32_t kFishSeed = 53u;
    constexpr uint32_t kFishPid = sequentialPid(kFishSeed);
    constexpr uint64_t kOldRod = makeRow(2, 0, 5, 10);
    constexpr auto kFishingStatic = matchRow(
        true, kOldRod, makeMeta(0, 0, 2, 3),
        kFishSeed, kFishPid, 8);
    static_assert(kFishingStatic.lead == Lead::Static);

    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchRow(
        true, safari, makeMeta(0, 0, 0, 3),
        kGrassSeed, kGrassPid, 15).matched());

    // Rerolled HG/SS Bug Catching Contest Static/Magnet Pull histories. These
    // vectors discriminate the new recursion path from direct origin evidence.
    // The lead proc, attracted source row, level and BCC movement/rate check are
    // proven only at the earliest origin; intervening attempts prove nature plus
    // minimum-31 rejection exactly as pinned Method K RecurseReject does.
    constexpr uint32_t staticMeta = makeMeta(0, 0, 0, 1);
    constexpr uint32_t magnetMeta = makeMeta(0, 1, 0, 0);

    constexpr uint32_t bccDepth1Seed = 280u;
    constexpr uint32_t bccDepth1Pid = sequentialPid(bccDepth1Seed);
    constexpr uint64_t bccDepth1Row = makeRow(8, 2, 7, 18, 25);
    static_assert(bccDepth1Pid == 0xCB57F0E6u);
    static_assert(Legality::Gen4LeadFrame::directMinimum31Satisfied(
        bccDepth1Seed));
    static_assert(Legality::Gen4LeadFrame::previousRerollAttemptRejected(
        bccDepth1Seed));
    static_assert(!matchAttempt(
        true, bccDepth1Row, staticMeta,
        bccDepth1Seed, bccDepth1Pid, 16).matched());
    constexpr auto bccStaticDepth1 = matchBugContestReroll(
        true, bccDepth1Row, staticMeta,
        bccDepth1Seed, bccDepth1Pid, 16, 1);
    static_assert(bccStaticDepth1.lead == Lead::Static);
    static_assert(bccStaticDepth1.rerollDepth == 1);
    constexpr auto bccMagnetDepth1 = matchBugContestReroll(
        true, bccDepth1Row, magnetMeta,
        bccDepth1Seed, bccDepth1Pid, 16, 1);
    static_assert(bccMagnetDepth1.lead == Lead::MagnetPull);
    static_assert(bccMagnetDepth1.rerollDepth == 1);
    constexpr auto bccDepth1Recovered = matchRow(
        true, bccDepth1Row, staticMeta,
        bccDepth1Seed, bccDepth1Pid, 16);
    static_assert(bccDepth1Recovered.lead == Lead::Static);
    static_assert(bccDepth1Recovered.rerollDepth == 1);

    constexpr uint32_t bccDepth2Seed = 480461u;
    constexpr uint32_t bccDepth2Pid = sequentialPid(bccDepth2Seed);
    constexpr uint64_t bccDepth2Row = makeRow(8, 0, 7, 18, 25);
    static_assert(bccDepth2Pid == 0x593DE283u);
    static_assert(Legality::Gen4LeadFrame::directMinimum31Satisfied(
        bccDepth2Seed));
    static_assert(!matchAttempt(
        true, bccDepth2Row, staticMeta,
        bccDepth2Seed, bccDepth2Pid, 17).matched());
    static_assert(!matchBugContestReroll(
        true, bccDepth2Row, staticMeta,
        bccDepth2Seed, bccDepth2Pid, 17, 1).matched());
    constexpr auto bccStaticDepth2 = matchBugContestReroll(
        true, bccDepth2Row, staticMeta,
        bccDepth2Seed, bccDepth2Pid, 17, 2);
    static_assert(bccStaticDepth2.lead == Lead::Static);
    static_assert(bccStaticDepth2.rerollDepth == 2);

    constexpr uint32_t bccDepth3Seed = 271501u;
    constexpr uint32_t bccDepth3Pid = sequentialPid(bccDepth3Seed);
    constexpr uint64_t bccDepth3Row = makeRow(8, 1, 7, 18, 25);
    static_assert(bccDepth3Pid == 0x646856F4u);
    static_assert(!Legality::Gen4LeadFrame::directMinimum31Satisfied(
        bccDepth3Seed));
    static_assert(!matchBugContestReroll(
        true, bccDepth3Row, staticMeta,
        bccDepth3Seed, bccDepth3Pid, 8, 1).matched());
    static_assert(!matchBugContestReroll(
        true, bccDepth3Row, staticMeta,
        bccDepth3Seed, bccDepth3Pid, 8, 2).matched());
    constexpr auto bccStaticDepth3 = matchBugContestReroll(
        true, bccDepth3Row, staticMeta,
        bccDepth3Seed, bccDepth3Pid, 8, 3);
    static_assert(bccStaticDepth3.lead == Lead::Static);
    static_assert(bccStaticDepth3.rerollDepth == 3);
    constexpr auto bccDepth3Recovered = matchRow(
        true, bccDepth3Row, staticMeta,
        bccDepth3Seed, bccDepth3Pid, 8);
    static_assert(bccDepth3Recovered.lead == Lead::Static);
    static_assert(bccDepth3Recovered.rerollDepth == 3);

    assert(matchRow(false, grass, makeMeta(3, 4, 0, 0),
                    jGrassSeed, jGrassPid, 5).matched());
    assert(matchRow(true, kOldRod, makeMeta(0, 0, 2, 3),
                    kFishSeed, kFishPid, 8).matched());

    std::cout << "Gen IV Static/Magnet Pull lead evidence: PASS\n";
}
