#include "Legality/Gen3EggFullHistoryAlternatives.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen3EggFullHistoryAlternatives;
namespace J=Legality::Gen3JointEggPIDIVMoveHistory;
namespace P=Legality::Gen3EggPIDCreationHistory;
namespace V=Legality::Gen3EggIVInheritanceStage;
namespace M=Legality::Gen34SameParentEggHistory;
constexpr std::array<uint8_t,6> before{0,3,6,9,12,15};
constexpr std::array<uint8_t,6> fatherIVs{7,12,17,22,27,0};
constexpr std::array<uint8_t,6> motherIVs{13,24,3,14,25,4};
A::Input fixture(const char* game,uint32_t seed) {
    A::Input a{};
    a.game=game;a.originalEggIdentity=0xABCD0123;
    a.originalUnhatchedEggEvidenceIndependentlyVerified=true;
    a.originalEggMovesIndependentlyVerified=true;
    a.originalEggSpecies=1;
    a.allOrdinaryDaycareAlternativesIndependentlyEnumerated=true;
    a.count=1;
    auto& route=a.alternatives[0];
    route.routeIdentity=1;
    route.claimedOriginalEggIdentity=a.originalEggIdentity;
    route.originalSourceHistoryIndependentlyAttested=true;
    route.thisCandidateMethodEvidenceFullyAttested=true;
    auto& x=route.history;
    x.originalEggEventIdentity=0x7722;
    x.sameOriginalEggAndEventIndependentlyVerified=true;
    x.originalEggSpeciesIndependentlyVerified=true;
    x.originalEggSpecies=1;
    x.originalUnhatchedEggMovesIndependentlyVerified=true;
    x.moveAndPIDSameParentSnapshotsIndependentlyVerified=true;
    x.moveAndPIDSamePhysicalPairIndependentlyVerified=true;
    auto& p=x.pid;
    p.game=game;
    p.originalUnhatchedEggIndependentlyVerified=true;
    p.ordinaryDaycarePairIndependentlyVerified=true;
    p.ordinaryDaycarePairEligible=true;
    p.originalPhysicalDepositOrderIndependentlyVerified=true;
    p.physicalSlots[0]={101,1,0,7,false,true,true};
    p.physicalSlots[1]={202,1,1,11,false,true,true};
    p.originalCreationStageIndependentlyVerified=true;
    p.originalCollectionStageIndependentlyVerified=true;
    p.originalParentStageIdentity=x.originalEggEventIdentity;
    p.originalCreationStageIdentity=x.originalEggEventIdentity;
    p.originalCollectionStageIdentity=x.originalEggEventIdentity;
    p.pendingLCRNGStateIndependentlyVerified=true;
    p.pendingLCRNGState=seed;
    p.collectionLCRNGStateIndependentlyVerified=true;
    p.collectionLCRNGState=seed^0x13459874u;
    p.originalEmeraldVBlankCounterIndependentlyVerified=true;
    p.originalEmeraldVBlankCounter=static_cast<uint16_t>(seed);
    a.originalEggPID=P::evaluate(p).finalPersonality;
    p.originalUnhatchedEggPID=a.originalEggPID;
    auto& mv=x.moveAndIV;
    auto& m=mv.moveHistory;
    m.game=game;m.species=1;m.fatherId=101;m.motherId=202;
    m.fatherSpecies=1;m.motherSpecies=1;
    m.snapshotsKnown=true;
    m.pairProof=M::PairProof::IndependentlyCompatible;
    m.lightBallKnown=true;
    m.base={33,45,0,0};m.observed=m.base;
    a.originalEggMoves=m.observed;
    mv.father={101,true,fatherIVs};mv.mother={202,true,motherIVs};
    mv.daycareSlotOrderIndependentlyVerified=true;
    mv.daycareSlot0ParentId=101;mv.daycareSlot1ParentId=202;
    mv.sharedParentStageIndependentlyVerified=true;
    mv.moveParentStageToken=x.originalEggEventIdentity;
    mv.ivParentStageToken=x.originalEggEventIdentity;
    mv.independentlyKnownFinalIVStageState=true;
    mv.initialChildIVs=before;
    const auto originalIV=V::replay(game,seed,before,fatherIVs,motherIVs,true);
    assert(originalIV.status==V::Status::ReplayedStage);
    mv.finalIVStageRNGState=originalIV.finalSeed;
    a.originalEggIVs=originalIV.childIVs;
    mv.observedUnhatchedIVs=a.originalEggIVs;
    return a;
}
int main() {
    unsigned seedControls=0,edgeCases=0;
    for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba",
                          "firered_gba","leafgreen_gba"}) {
        for(uint32_t seed=0;seed<512;++seed) {
            auto x=fixture(game,seed);
            assert(A::evaluate(x).status==A::Status::OneConditionallyConsistentHistory);
            auto denied=x;
            denied.alternatives[0].history.pid.originalUnhatchedEggPID^=1;
            // Original egg projection must match for a candidate before
            // source-stage replay; a fabricated egg PID stays Unverified.
            assert(A::evaluate(denied).status==A::Status::UnverifiedCandidateProvenance);
            denied=x;
            denied.alternatives[0].history.moveAndIV.moveHistory.father={15,0,0,0};
            denied.alternatives[0].history.moveAndIV.moveHistory.verifiedMachine=
                {true,false,false,false};
            // One separately attested inherited TM move unconditionally
            // changes this exact four-slot history. The original observed
            // move bytes remain unchanged; no parent/egg projection forgery.
            const auto local=A::evaluate(denied);
            assert(local.status==A::Status::AllAttestedOrdinaryDaycareRoutesContradicted);
            assert(local.contradicted==1 && !local.consistent);
            denied.allOrdinaryDaycareAlternativesIndependentlyEnumerated=false;
            assert(A::evaluate(denied).status==A::Status::UnknownAlternativeEnumeration);
            // New independent source stage restores a viable explanation
            // even when another individually described history disagrees.
            denied.count=2;denied.alternatives[1]=x.alternatives[0];
            denied.alternatives[1].routeIdentity=2;
            assert(A::evaluate(denied).status==A::Status::OneConditionallyConsistentHistory);
            ++seedControls;
        }
        auto x=fixture(game,42);
        x.originalUnhatchedEggEvidenceIndependentlyVerified=false;
        assert(A::evaluate(x).status==A::Status::UnknownOriginalEgg);
        x=fixture(game,42);x.originalEggMovesIndependentlyVerified=false;
        assert(A::evaluate(x).status==A::Status::UnknownOriginalEgg);
        x=fixture(game,42);x.alternatives[0].routeIdentity=0;
        // An independently attested matching history may appear AFTER
        // a contradictory hypothesis with the very same archive ID.
        // Order cannot manufacture a false-negative candidate universe.
        x=fixture(game,42);x.count=2;
        x.alternatives[1]=x.alternatives[0];
        x.alternatives[0].history.moveAndIV.moveHistory.father={15,0,0,0};
        x.alternatives[0].history.moveAndIV.moveHistory.verifiedMachine=
            {true,false,false,false};
        const auto reversed=A::evaluate(x);
        assert(reversed.status==A::Status::OneConditionallyConsistentHistory);
        assert(reversed.consistent==1 && reversed.unverified==1);
        x=fixture(game,42);x.count=2;x.alternatives[1]=x.alternatives[0];
        // A duplicated row is not an independent history, but may not
        // suppress the first independently sourced matching parent pair.
        const auto duplicateWithMatch=A::evaluate(x);
        assert(duplicateWithMatch.status==A::Status::OneConditionallyConsistentHistory);
        assert(duplicateWithMatch.consistent==1 && duplicateWithMatch.unverified==1);
        // With no valid matching source route, duplicate/zero identity
        // cannot fabricate exhaustive rejection.
        x.alternatives[0].history.moveAndIV.moveHistory.father={15,0,0,0};
        x.alternatives[0].history.moveAndIV.moveHistory.verifiedMachine=
            {true,false,false,false};
        assert(A::evaluate(x).status==A::Status::DuplicateOrMissingRoute);
        x=fixture(game,42);x.count=2;
        x.alternatives[1]=x.alternatives[0];
        x.alternatives[0].routeIdentity=0;
        assert(A::evaluate(x).status==A::Status::OneConditionallyConsistentHistory);
        x=fixture(game,42);x.alternatives[0].originalSourceHistoryIndependentlyAttested=false;
        assert(A::evaluate(x).status==A::Status::UnverifiedCandidateProvenance);
        x=fixture(game,42);x.alternatives[0].claimedOriginalEggIdentity=2;
        assert(A::evaluate(x).status==A::Status::UnverifiedCandidateProvenance);
        x=fixture(game,42);x.alternatives[0].history.moveAndPIDSamePhysicalPairIndependentlyVerified=false;
        assert(A::evaluate(x).status==A::Status::UnresolvedCandidateHistory);
        x=fixture(game,42);x.alternatives[0].history.pid.pendingLCRNGStateIndependentlyVerified=false;
        assert(A::evaluate(x).status==A::Status::UnresolvedCandidateHistory);
        x=fixture(game,42);x.originalEggSpecies++;
        assert(A::evaluate(x).status==A::Status::UnverifiedCandidateProvenance);
        x=fixture(game,42);x.originalEggPID++;
        assert(A::evaluate(x).status==A::Status::UnverifiedCandidateProvenance);
        x=fixture(game,42);x.alternatives[0].thisCandidateMethodEvidenceFullyAttested=false;
        x.alternatives[0].history.moveAndIV.moveHistory.observed={33,45,0,0};
        x.alternatives[0].history.moveAndIV.moveHistory.father={15,0,0,0};
        x.alternatives[0].history.moveAndIV.moveHistory.verifiedMachine=
            {true,false,false,false};
        assert(A::evaluate(x).status==A::Status::UnresolvedCandidateHistory);
        x=fixture(game,42);x.count=9;
        assert(A::evaluate(x).status==A::Status::CapacityExceededUnknown);
        x=fixture(game,42);x.count=0;
        assert(A::evaluate(x).status==A::Status::NoCandidates);
        edgeCases+=13;
    }
    auto x=fixture("ruby_gba",0);x.game="platinum_nds";
    assert(A::evaluate(x).status==A::Status::UnsupportedGame);
    std::cout<<"L2 B18 original Gen III PID/IV/move alternatives: "
             <<seedControls<<" source-stage seeds and "<<edgeCases
             <<" missing history/cross-parent controls PASS\n";
}
