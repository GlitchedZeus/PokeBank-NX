#include "Legality/Gen3JointEggCandidateEvidence.h"
#include <array>
#include <cassert>
#include <iostream>

namespace H=Legality::Gen3JointEggCandidateEvidence;
namespace G=Legality::Gen3EggIVInheritanceStage;

void runCases() {
    H::Input in{};
    auto& m=in.moveHistory;
    m.game="firered_gba";m.species=1;m.fatherId=101;m.motherId=202;
    m.snapshotsKnown=true;m.fatherSpecies=1;m.motherSpecies=1;
    m.pairProof=Legality::Gen34SameParentEggHistory::PairProof::IndependentlyCompatible;
    m.base={33,45,0,0};m.father={80,345,15,33};m.mother={33,45,0,0};
    m.verifiedMachine={false,false,true,false};
    m.verifiedSharedLevel={false,false,false,true};
    m.lightBallKnown=true;m.observed={80,345,15,33};
    in.father={101,true,{30,30,30,30,30,30}};
    in.mother={202,true,{31,31,31,31,31,31}};
    in.initialChildIVs={0,0,0,0,0,0};
    constexpr uint32_t entrance=0x00000002u;
    auto staged=G::replay("firered_gba",entrance,in.initialChildIVs,
                          in.father.originalIVs,in.mother.originalIVs,true);
    assert(staged.status==G::Status::ReplayedStage);
    in.independentlyKnownFinalIVStageState=true;
    in.finalIVStageRNGState=staged.finalSeed;
    in.observedUnhatchedIVs=staged.childIVs;

    auto r=H::evaluate(in);
    assert(r.status==H::Status::KnownCandidateConsistent);
    assert(r.moves.eggSources==2 && r.moves.sharedSources==1);
    assert(r.ivs==G::CandidateCheck::Matched);
    // A different "father" in the IV calculation does not make a
    // mutually exclusive independent father history jointly possible.
    auto differentDad=in;differentDad.father.id=303;
    assert(H::evaluate(differentDad).status==H::Status::IncoherentParentWitness);
    auto differentMom=in;differentMom.mother.id=909;
    assert(H::evaluate(differentMom).status==H::Status::IncoherentParentWitness);
    auto unknownParent=in;unknownParent.father.originalIVsKnown=false;
    assert(H::evaluate(unknownParent).status==H::Status::UnknownIVParentSnapshot);
    auto missingFinalState=in;missingFinalState.independentlyKnownFinalIVStageState=false;
    assert(H::evaluate(missingFinalState).status==H::Status::UnknownIVRNGHistory);
    auto oppositeDonor=in;oppositeDonor.mother.originalIVs={0,0,0,0,0,0};
    assert(H::evaluate(oppositeDonor).status==H::Status::IVCandidateContradicted);
    auto malformedObserved=in;malformedObserved.observedUnhatchedIVs[2]=32;
    assert(H::evaluate(malformedObserved).status==H::Status::InvalidIVCandidateInput);
    auto malformedParent=in;malformedParent.father.originalIVs[1]=32;
    assert(H::evaluate(malformedParent).status==H::Status::InvalidIVCandidateInput);
    auto changedFatherMoves=in;changedFatherMoves.moveHistory.father={80,15,33,0};
    changedFatherMoves.moveHistory.verifiedMachine={false,true,false,false};
    changedFatherMoves.moveHistory.verifiedSharedLevel={false,false,true,false};
    assert(H::evaluate(changedFatherMoves).status==H::Status::MoveCandidateContradicted);
    auto unverifiedPair=in;unverifiedPair.moveHistory.pairProof=
        Legality::Gen34SameParentEggHistory::PairProof::Unknown;
    assert(H::evaluate(unverifiedPair).status==H::Status::UnknownPairEligibility);
    auto sourceUnknown=in;sourceUnknown.moveHistory.game="platinum_nds";
    assert(H::evaluate(sourceUnknown).status==H::Status::UnsupportedGame);
    auto missingParentMove=in;missingParentMove.moveHistory.snapshotsKnown=false;
    assert(H::evaluate(missingParentMove).status==H::Status::UnknownMoveParentHistory);
    auto rs=in;rs.moveHistory.game="ruby_gba";
    auto rRS=H::evaluate(rs);
    assert(rRS.status==H::Status::IVCandidateContradicted ||
           rRS.status==H::Status::KnownCandidateConsistent);
    // Separate retail RS vs Emerald stage behavior is already covered by
    // the independent 24-vector source-pinned forward/reverse test.
}
int main() {
    runCases();
    std::cout<<"Gen III same-parent joint egg move plus IV witness: PASS\n";
}
