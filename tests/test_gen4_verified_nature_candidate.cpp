#include "Legality/Gen4NatureReversalEvidence.h"
#include <cassert>
#include <iostream>
namespace N=Legality::Gen4NatureReversal;
void runCases(){
    // Independently pinned synthetic 129 rejected PID pairs: 128
    // bounded searches MUST remain Unknown/Exhausted, 256 reaches the
    // nature-compatible predecessor without inventing full encounter
    // method/lead/area proof.
    constexpr N::Candidate longHistory{true,0xE0D14A4Fu,0xEA0705DBu,20,128};
    static_assert(N::retainedPidFromStage(longHistory.prePidState)==
                  longHistory.observedRetainedPid);
    constexpr auto shortBudget=N::verifyKnownCandidate(longHistory);
    static_assert(shortBudget.status==N::CandidateStatus::SearchExhausted);
    static_assert(shortBudget.reverse.inspectedPidPairs==128);
    constexpr auto fullBudget=[]{
        auto c=longHistory;c.pairBudget=256;
        return N::verifyKnownCandidate(c);
    }();
    static_assert(fullBudget.status==N::CandidateStatus::CandidateConsistent);
    static_assert(fullBudget.reverse.rejectedPidPairs==129);
    static_assert(fullBudget.reverse.inspectedPidPairs==130);
    const auto actual=N::verifyKnownCandidate(longHistory);
    assert(actual.status==N::CandidateStatus::SearchExhausted);
    N::Candidate one{true,0x00001659u,0x8D4BE8AFu,23,128};
    assert(N::verifyKnownCandidate(one).status==
           N::CandidateStatus::CandidateConsistent);
    N::Candidate two{true,0x00000264u,0x36373E13u,10,128};
    assert(N::verifyKnownCandidate(two).status==
           N::CandidateStatus::CandidateConsistent);
    auto unknown=longHistory;unknown.independentlyKnownPrePidState=false;
    assert(N::verifyKnownCandidate(unknown).status==
           N::CandidateStatus::UnknownOriginalStage);
    auto wrongPid=longHistory;wrongPid.observedRetainedPid^=1u;
    assert(N::verifyKnownCandidate(wrongPid).status==
           N::CandidateStatus::IncompatibleRetainedPid);
    auto wrongNature=longHistory;wrongNature.targetNature=19;
    assert(N::verifyKnownCandidate(wrongNature).status==
           N::CandidateStatus::IncompatibleRetainedPid);
    auto invalidNature=longHistory;invalidNature.targetNature=25;
    assert(N::verifyKnownCandidate(invalidNature).status==
           N::CandidateStatus::InvalidNature);
    auto noAttempts=longHistory;noAttempts.pairBudget=0;
    assert(N::verifyKnownCandidate(noAttempts).status==
           N::CandidateStatus::SearchExhausted);
    assert(N::verifyKnownCandidate(noAttempts).reverse.inspectedPidPairs==0);
}
int main(){runCases();std::cout<<"Gen IV attested PID/nature reversal with 129-rejection exhaustion: PASS\n";}
