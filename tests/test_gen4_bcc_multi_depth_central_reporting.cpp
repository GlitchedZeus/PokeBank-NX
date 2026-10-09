#include "Legality/Gen4WildRngCorrelation.h"
#include "Legality/Gen4LeadReportingEvidence.h"
#include "Legality/Gen4BugContestMixedDepthEvidence.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    using namespace Legality;
    using Gen4LeadHistory::Path;

    // Real pinned encounter_hg.pkl Kakuna: HeartGold #14, location 207,
    // Bug Catching Contest slot 3, levels 9..18 and actual rate 25.
    constexpr uint64_t row = 0x64D80412139E0EULL;
    static_assert(Gen4Wild::game(row) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(row) == 14);
    static_assert(Gen4Wild::location(row) == 207);
    static_assert(Gen4Wild::slot(row) == 3);
    static_assert(Gen4Wild::rate(row) == 25);

    constexpr uint16_t bit =
        static_cast<uint16_t>(Path::SynchronizeMixedMultipleRerolls);

    // Depth2: failed / success / failed, two rejected IV attempts.
    // Depth3: failed / failed / failed / success, the fourth retained
    // attempt does NOT have a 31 IV (legal after all prior rejections).
    constexpr uint32_t depth2Seed=0x000E11BEu, depth2Pid=0x9C2C4659u;
    constexpr uint32_t depth3Seed=0x0014FEAAu, depth3Pid=0x8EE19004u;
    static_assert(Gen4BugContestMixedDepth::match(
        row,depth2Seed,depth2Pid,16,2,0b010u).matched());
    static_assert(Gen4BugContestMixedDepth::match(
        row,depth3Seed,depth3Pid,10,3,0b1000u).matched());

    const auto depth2 = Gen4WildRng::analyzeSupported(
        "heartgold_nds",14,207,16,0,0,depth2Seed,depth2Pid);
    const auto depth3 = Gen4WildRng::analyzeSupported(
        "heartgold_nds",14,207,10,0,0,depth3Seed,depth3Pid);

    // Positive proof must survive actual species/game/met source lookup,
    // central Method K selection and lead-history OR mask, not only helper.
    assert(depth2.method==Gen4WildRng::Method::MethodKExtendedLead);
    assert(depth3.method==Gen4WildRng::Method::MethodKExtendedLead);
    assert((depth2.leadHistoryMask & bit)!=0);
    assert((depth3.leadHistoryMask & bit)!=0);
    assert(depth2.slot==0xFF && depth3.slot==0xFF);

    const std::string message2 = Gen4LeadReporting::describeWildResult(
        depth2.leadHistoryMask, Gen4WildRng::methodName(depth2.method));
    const std::string message3 = Gen4LeadReporting::describeWildResult(
        depth3.leadHistoryMask, Gen4WildRng::methodName(depth3.method));
    assert(message2.find("Synchronize mixed two/three rerolls (BCC)")!=
           std::string::npos);
    assert(message3.find("Synchronize mixed two/three rerolls (BCC)")!=
           std::string::npos);
    assert(message2.find("other unproven lead histories remain incomplete")!=
           std::string::npos);
    assert(message3.find("other unproven lead histories remain incomplete")!=
           std::string::npos);

    // Unsupported identity/level does not gain the positive evidence.
    const auto wrongGame=Gen4WildRng::analyzeSupported(
        "diamond_nds",14,207,16,0,0,depth2Seed,depth2Pid);
    const auto wrongLevel=Gen4WildRng::analyzeSupported(
        "heartgold_nds",14,207,1,0,0,depth2Seed,depth2Pid);
    const auto wrongSpecies=Gen4WildRng::analyzeSupported(
        "heartgold_nds",1,207,16,0,0,depth2Seed,depth2Pid);
    assert((wrongGame.leadHistoryMask & bit)==0);
    assert((wrongLevel.leadHistoryMask & bit)==0);
    assert((wrongSpecies.leadHistoryMask & bit)==0);

    // A missing Method K positive history is still Incomplete / Unresolved,
    // never proof of an impossible Pokemon or a mandate to modify it.
    std::cout << "Gen IV BCC multi-reroll central report bridge: PASS\n";
}
