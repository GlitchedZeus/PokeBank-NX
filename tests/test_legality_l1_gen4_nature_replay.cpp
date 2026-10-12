#include "Legality/Gen4NatureReversalReplay.h"
#include <cassert>
#include <iostream>
int main() {
    namespace N=Legality::Gen4NatureReversalReplay;
    using S=N::Status;
    // RNG-002, pinned HGSS CreateMonWithNature stage, independently compared
    // with unchanged retail C. Seed 0x12: nature 20, 129 rejected PID pairs.
    N::Candidate hg{};
    hg.method=N::Method::K;
    hg.originalNatureStateIndependentlyAttested=true;
    hg.originalPidSnapshotIndependentlyAttested=true;
    hg.preNatureState=0x00000012u;
    hg.recordedPid=0xEA0705DBu;
    hg.targetNature=20;
    hg.maxPidPairs=128;
    auto result=N::replay(hg);
    assert(result.status==S::SearchExhausted);
    assert(result.inspectedPairs==128u);
    hg.maxPidPairs=129;
    assert(N::replay(hg).status==S::SearchExhausted);
    hg.maxPidPairs=130;
    result=N::replay(hg);
    assert(result.status==S::AttestedStageMatched);
    assert(result.rejectedPairs==129u);
    assert(result.inspectedPairs==130u);
    assert(result.acceptedPid==0xEA0705DBu);
    assert(result.preAcceptedPidState==0xE0D14A4Fu);
    // Prove legacy reversal was a *budget*, not an impossible-history verdict.
    const auto old=Legality::Gen4NatureReversal::search(0xE0D14A4Fu,20,128);
    assert(old.exhausted());
    const auto deep=Legality::Gen4NatureReversal::search(0xE0D14A4Fu,20,130);
    assert(deep.matched() && deep.rejectedPidPairs==129u);
    auto wrong=hg;wrong.recordedPid^=1u;
    assert(N::replay(wrong).status==S::AttestedStagePidContradicted);
    auto absent=hg;absent.originalNatureStateIndependentlyAttested=false;
    assert(N::replay(absent).status==S::UnknownHistoricalStage);
    absent=hg;absent.originalPidSnapshotIndependentlyAttested=false;
    assert(N::replay(absent).status==S::UnknownHistoricalStage);
    auto bad=hg;bad.targetNature=25;
    assert(N::replay(bad).status==S::InvalidTargetNature);
    // Separate Method J / DPPt nature division stage (not HGSS modulo).
    N::Candidate j{};
    j.method=N::Method::J;
    j.originalNatureStateIndependentlyAttested=true;
    j.originalPidSnapshotIndependentlyAttested=true;
    j.preNatureState=0x35u;
    j.targetNature=15;
    j.recordedPid=0xEC793528u;
    j.maxPidPairs=1;
    const auto direct=N::replay(j);
    assert(direct.matched());
    assert(direct.rejectedPairs==0u);
    auto wrongNature=j;wrongNature.targetNature=14;
    assert(N::replay(wrongNature).status==S::AttestedStagePidContradicted);
    auto unknown=j;unknown.method=N::Method::Unknown;
    assert(N::replay(unknown).status==S::UnsupportedMethod);
    // A maliciously huge caller budget is capped, while the real 129-pair
    // fixture still succeeds within the implementation resource ceiling.
    auto gigantic=hg;gigantic.maxPidPairs=UINT32_MAX;
    assert(N::replay(gigantic).matched());
    static_assert(N::kMaximumReplayPairs>=130u);
    // Zero samples proves nothing, for either method.
    j.maxPidPairs=0;
    assert(N::replay(j).status==S::SearchExhausted);
    std::cout<<"L1 E15 source-attested J/K long nature stage: PASS\n";
}
