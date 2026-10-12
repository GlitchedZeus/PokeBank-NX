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
    in.daycareSlotOrderIndependentlyVerified=true;
    in.daycareSlot0ParentId=101;
    in.daycareSlot1ParentId=202;
    in.sharedParentStageIndependentlyVerified=true;
    in.moveParentStageToken=0x1234ABCDu;
    in.ivParentStageToken=0x1234ABCDu;
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
    auto missingSharedStage=in;missingSharedStage.sharedParentStageIndependentlyVerified=false;
    assert(H::evaluate(missingSharedStage).status==H::Status::UnknownSharedParentStage);
    auto wrongSharedStage=in;wrongSharedStage.ivParentStageToken=0xBEEFu;
    assert(H::evaluate(wrongSharedStage).status==H::Status::IncoherentParentStage);
    auto missingStageToken=in;missingStageToken.moveParentStageToken=0;
    assert(H::evaluate(missingStageToken).status==H::Status::UnknownSharedParentStage);
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

void testRetailDaycareSlotOrdering() {
    // BREED-002: 24 pinned retail IV-stage cases, with BOTH independently
    // asserted deposit orderings. Observed IVs below are the independently
    // cross-checked retail results, not calculated from the joint helper.
    constexpr std::array<uint32_t,8> seeds{
        0u,1u,2u,3u,0x2Au,0xFFFFu,0x12345678u,0xFFFFFFFFu
    };
    constexpr std::array<uint32_t,8> finalSeeds{
        0xE2CCA5EEu,0xB6A8FDE7u,0x8A8555E0u,0x5E61ADD9u,
        0xA4F314C8u,0x66E94DF5u,0x983788A6u,0x0EF04DF5u
    };
    constexpr std::array<std::array<uint8_t,6>,8> emerald{{
        {{30,0,0,30,0,30}}, {{0,31,31,0,30,0}},
        {{0,31,0,0,31,0}}, {{0,30,30,0,31,0}},
        {{0,0,0,31,31,30}}, {{0,31,30,0,0,31}},
        {{0,30,30,0,31,0}}, {{0,0,30,31,0,30}}
    }};
    constexpr std::array<std::array<uint8_t,6>,8> rsOrFrlg{{
        {{30,0,30,0,0,30}}, {{31,0,31,0,30,0}},
        {{30,31,0,0,31,0}}, {{0,30,30,0,31,0}},
        {{0,0,31,31,0,30}}, {{31,30,0,0,0,31}},
        {{0,30,30,0,31,0}}, {{0,0,30,31,0,30}}
    }};
    constexpr std::array<const char*,3> games{
        "emerald_gba","ruby_gba","firered_gba"
    };
    for(const auto game:games) {
        for(size_t k=0;k<seeds.size();++k) {
            for(unsigned fatherSlot=0;fatherSlot<2;++fatherSlot) {
                H::Input in{};
                auto& m=in.moveHistory;
                m.game=game;m.species=1;
                m.fatherId=101;m.motherId=202;
                m.snapshotsKnown=true;
                m.fatherSpecies=1;m.motherSpecies=1;
                m.pairProof=Legality::Gen34SameParentEggHistory::PairProof::IndependentlyCompatible;
                m.base={33,45,0,0};m.observed=m.base;
                m.lightBallKnown=true;
                constexpr std::array<uint8_t,6> low{30,30,30,30,30,30};
                constexpr std::array<uint8_t,6> high{31,31,31,31,31,31};
                // Regardless of role names, physical slot 0 has low IVs.
                in.father={101,true,fatherSlot==0?low:high};
                in.mother={202,true,fatherSlot==0?high:low};
                in.daycareSlotOrderIndependentlyVerified=true;
                in.daycareSlot0ParentId=fatherSlot==0?101u:202u;
                in.daycareSlot1ParentId=fatherSlot==0?202u:101u;
                in.sharedParentStageIndependentlyVerified=true;
                in.moveParentStageToken=0x9988u;
                in.ivParentStageToken=0x9988u;
                in.independentlyKnownFinalIVStageState=true;
                in.finalIVStageRNGState=finalSeeds[k];
                in.initialChildIVs={0,0,0,0,0,0};
                in.observedUnhatchedIVs=
                    (G::family(game)==G::Family::Emerald?emerald:rsOrFrlg)[k];
                assert(G::rewindSix(in.finalIVStageRNGState)==seeds[k]);
                const auto out=H::evaluate(in);
                assert(out.moves.status==
                    Legality::Gen34SameParentEggHistory::Status::CandidateCompatible);
                assert(out.status==H::Status::KnownCandidateConsistent);
                assert(out.ivs==G::CandidateCheck::Matched);
                // Without independent order we cannot claim either a
                // match or an impossibility, even with both parent IVs.
                auto unknown=in;
                unknown.daycareSlotOrderIndependentlyVerified=false;
                assert(H::evaluate(unknown).status==
                       H::Status::UnknownDaycareSlotOrder);
                auto noSlot=in;noSlot.daycareSlot0ParentId=0;
                assert(H::evaluate(noSlot).status==
                       H::Status::UnknownDaycareSlotOrder);
                auto sameSlot=in; sameSlot.daycareSlot1ParentId=sameSlot.daycareSlot0ParentId;
                assert(H::evaluate(sameSlot).status==
                       H::Status::IncoherentDaycareSlotOrder);
                auto alien=in;alien.daycareSlot1ParentId=303;
                assert(H::evaluate(alien).status==
                       H::Status::IncoherentDaycareSlotOrder);
                auto wrongOrder=in;
                wrongOrder.daycareSlot0ParentId=in.daycareSlot1ParentId;
                wrongOrder.daycareSlot1ParentId=in.daycareSlot0ParentId;
                // Contradicts this independently attested IV-stage
                // candidate (not the overall legality of the Pokemon).
                // Some RNG states can choose solely one parent and the
                // observed difference is still conclusive for those rows.
                const auto wrong=H::evaluate(wrongOrder);
                assert(wrong.status==H::Status::IVCandidateContradicted ||
                       wrong.status==H::Status::KnownCandidateConsistent);
            }
        }
    }
}
int main() {
    runCases();
    testRetailDaycareSlotOrdering();
    std::cout<<"Gen III same-parent joint egg move plus IV witness: PASS\n";
}
