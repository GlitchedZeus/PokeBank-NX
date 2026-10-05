#include "Legality/Gen34WurmpleEvolutionEvidence.h"

#include <cassert>
#include <cstdint>

using namespace Legality::Gen34WurmpleEvolution;

namespace {

constexpr uint32_t pidWithHigh(uint16_t high) noexcept {
    return static_cast<uint32_t>(high) << 16;
}

void testPidModuloBoundaries() {
    // Pinned PKHeX rule: ((PID >> 16) % 10) / 5.
    assert(branchForPid(pidWithHigh(0)) == Branch::SilcoonBeautifly);
    assert(branchForPid(pidWithHigh(4)) == Branch::SilcoonBeautifly);
    assert(branchForPid(pidWithHigh(5)) == Branch::CascoonDustox);
    assert(branchForPid(pidWithHigh(9)) == Branch::CascoonDustox);
    assert(branchForPid(pidWithHigh(10)) == Branch::SilcoonBeautifly);
    assert(branchForPid(pidWithHigh(14)) == Branch::SilcoonBeautifly);
    assert(branchForPid(pidWithHigh(15)) == Branch::CascoonDustox);
    assert(branchForPid(pidWithHigh(19)) == Branch::CascoonDustox);

    // Exercise every possible 16-bit high word, not only the named boundaries.
    for (uint32_t high = 0; high <= 0xFFFFu; ++high) {
        const auto expected = ((high % 10u) / 5u) == 0
            ? Branch::SilcoonBeautifly
            : Branch::CascoonDustox;
        assert(branchForPid(high << 16) == expected);
    }
}

void testEvolutionGroups() {
    assert(isWurmpleEvolution(266));
    assert(isWurmpleEvolution(267));
    assert(isWurmpleEvolution(268));
    assert(isWurmpleEvolution(269));
    assert(!isWurmpleEvolution(265));
    assert(!isWurmpleEvolution(270));

    assert(branchForEvolution(266) == Branch::SilcoonBeautifly);
    assert(branchForEvolution(267) == Branch::SilcoonBeautifly);
    assert(branchForEvolution(268) == Branch::CascoonDustox);
    assert(branchForEvolution(269) == Branch::CascoonDustox);
}

void testEncounterConditioning() {
    const uint32_t silcoonPid = pidWithHigh(4);
    const uint32_t cascoonPid = pidWithHigh(5);

    // Wurmple-origin descendants are constrained by the PID branch.
    assert(evaluate(265, 266, silcoonPid).status == Status::Compatible);
    assert(evaluate(265, 267, silcoonPid).status == Status::Compatible);
    assert(evaluate(265, 268, silcoonPid).status == Status::Incompatible);
    assert(evaluate(265, 269, silcoonPid).status == Status::Incompatible);

    assert(evaluate(265, 268, cascoonPid).status == Status::Compatible);
    assert(evaluate(265, 269, cascoonPid).status == Status::Compatible);
    assert(evaluate(265, 266, cascoonPid).status == Status::Incompatible);
    assert(evaluate(265, 267, cascoonPid).status == Status::Incompatible);

    // Surviving Wurmple gets only a future-branch prediction, never an invalid result.
    const auto unevolved = evaluate(265, 265, cascoonPid);
    assert(unevolved.status == Status::Compatible);
    assert(unevolved.pidBranch == Branch::CascoonDustox);

    // Directly encountered cocoons are deliberate counterexamples: the pinned
    // verifier does not apply the Wurmple-origin branch rule to them.
    assert(evaluate(266, 266, cascoonPid).status == Status::NotApplicable);
    assert(evaluate(268, 268, silcoonPid).status == Status::NotApplicable);
    assert(evaluate(266, 267, cascoonPid).status == Status::NotApplicable);
    assert(evaluate(268, 269, silcoonPid).status == Status::NotApplicable);

    // Unknown or unrelated encounter histories must stay outside this rule.
    assert(evaluate(0, 267, cascoonPid).status == Status::NotApplicable);
    assert(evaluate(265, 270, cascoonPid).status == Status::NotApplicable);
}

void testBranchNames() {
    assert(branchName(Branch::SilcoonBeautifly)[0] == 'S');
    assert(branchName(Branch::CascoonDustox)[0] == 'C');
}

} // namespace

int main() {
    testPidModuloBoundaries();
    testEvolutionGroups();
    testEncounterConditioning();
    testBranchNames();
    return 0;
}
