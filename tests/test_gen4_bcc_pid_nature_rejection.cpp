#include "Legality/Gen4BugContestNoLeadEvidence.h"
#include "Legality/Gen4WildRngCorrelation.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t kRealHeartGoldKakuna = 0x64D80412139E0EULL;
}

int main() {
    using namespace Legality;
    using Gen4LeadFrame::sequentialPid;
    using Gen3PidIv::Detail::prev;

    static_assert(Gen4Wild::game(kRealHeartGoldKakuna) ==
                  Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(kRealHeartGoldKakuna) == 14);
    static_assert(Gen4Wild::location(kRealHeartGoldKakuna) == 207);
    static_assert(Gen4Wild::method(kRealHeartGoldKakuna) == 8);
    static_assert(Gen4Wild::slot(kRealHeartGoldKakuna) == 3);
    static_assert(Gen4Wild::minLevel(kRealHeartGoldKakuna) == 9);
    static_assert(Gen4Wild::maxLevel(kRealHeartGoldKakuna) == 18);
    static_assert(Gen4Wild::rate(kRealHeartGoldKakuna) == 25);

    // Pinned Method K GenerateMethodK.SetRandomK: an ordinary nature roll
    // is followed by repeated 2-frame PIDs until pid % 25 == nature.
    // One rejected PID before retention: nature =23, rejected 0x00000312
    // (wrong nature), retained 0x8D4BE8AF (nature23). The pre-nature source
    // frames yield genuine slot 3, level 10 in the source-packed BCC row.
    constexpr uint32_t oneSeed = 0x00001659u;
    constexpr uint32_t onePid = 0x8D4BE8AFu;
    constexpr uint32_t oneNatureSeed = prev(prev(oneSeed));
    static_assert(oneNatureSeed == 0x43DD6F97u);
    static_assert((oneNatureSeed >> 16) % 25u == onePid % 25u);
    static_assert(sequentialPid(oneNatureSeed) == 0x00000312u);
    static_assert(sequentialPid(oneNatureSeed) % 25u != onePid % 25u);
    static_assert(sequentialPid(oneSeed) == onePid);
    static_assert(Gen4LeadFrame::directMinimum31Satisfied(oneSeed));
    static_assert(Gen4LeadFrame::reversalWindow(oneSeed,
        static_cast<uint8_t>(onePid % 25u)) == 31);
    constexpr auto one=Gen4BugContestNoLead::match(
        kRealHeartGoldKakuna,oneSeed,onePid,10);
    static_assert(one.matched());
    static_assert(one.slot==3 && one.level==10);

    // Two rejected PID nature attempts: q first produces 0x40D24416,
    // second produces 0x0000BEAC, then retained 0x36373E13 (nature10).
    // Actual source BCC slot 3, level15, minimum-31 IV requirement met.
    constexpr uint32_t twoSeed = 0x00000264u;
    constexpr uint32_t twoPid = 0x36373E13u;
    constexpr uint32_t twoNatureSeed =
        prev(prev(prev(prev(twoSeed))));
    static_assert(twoNatureSeed == 0xA20DDC80u);
    static_assert((twoNatureSeed >> 16) % 25u == twoPid % 25u);
    static_assert(sequentialPid(twoNatureSeed) == 0x40D24416u);
    static_assert(sequentialPid(twoNatureSeed) % 25u != twoPid % 25u);
    constexpr uint32_t afterFirstRejected =
        Gen3PidIv::Detail::next(Gen3PidIv::Detail::next(twoNatureSeed));
    static_assert(sequentialPid(afterFirstRejected) == 0x0000BEACu);
    static_assert(sequentialPid(afterFirstRejected) % 25u != twoPid % 25u);
    static_assert(sequentialPid(twoSeed) == twoPid);
    static_assert(Gen4LeadFrame::directMinimum31Satisfied(twoSeed));
    static_assert(Gen4LeadFrame::reversalWindow(twoSeed,
        static_cast<uint8_t>(twoPid % 25u)) == 18);
    constexpr auto two=Gen4BugContestNoLead::match(
        kRealHeartGoldKakuna,twoSeed,twoPid,15);
    static_assert(two.matched());
    static_assert(two.slot==3 && two.level==15);

    // Source-indexed central analyzer preserves each positive Method K
    // history; an unknown or other-game encounter must NOT be falsely
    // identified as this native HeartGold Bug Contest encounter.
    const auto sourceOne=Gen4WildRng::analyzeSupported(
        "heartgold_nds",14,207,10,0,0,oneSeed,onePid);
    const auto sourceTwo=Gen4WildRng::analyzeSupported(
        "heartgold_nds",14,207,15,0,0,twoSeed,twoPid);
    assert(sourceOne.method==Gen4WildRng::Method::MethodKBugContestNoLead);
    assert(sourceTwo.method==Gen4WildRng::Method::MethodKBugContestNoLead);
    assert(!Gen4WildRng::analyzeSupported(
        "diamond_nds",14,207,10,0,0,oneSeed,onePid).matched());
    assert(!Gen4WildRng::analyzeSupported(
        "heartgold_nds",14,999,10,0,0,oneSeed,onePid).matched());
    assert(!Gen4BugContestNoLead::match(
        kRealHeartGoldKakuna,oneSeed,onePid,1).matched());

    // Positive path only. Other unknown nature rejection histories remain
    // incomplete; a non-match is never an automatic invalid verdict.
    std::cout << "Gen IV HGSS BCC 1/2 PID nature-rejection loops: PASS\n";
}
