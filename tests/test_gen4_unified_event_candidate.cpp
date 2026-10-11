#include "Legality/Gen4UnifiedEventCandidateEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
namespace U=Legality::Gen4UnifiedEventCandidateEvidence;
namespace H=Legality::Gen14CandidateMoveHistory;
void runCases() {
    // Card #67 identity is pinned in the structural Gen IV template
    // index; all starting moves, ribbon masks and OT text below are
    // independent SYNTHETIC witness inputs, NOT historical card #67 data.
    U::Input in{};
    in.moves.stored={
        4,7200,58364,0x0f199b50u,40,3053,16,0,0,1,7,0,true
    };
    in.moves.claimedCardId=67;
    in.moves.history={7001,60,40,true,5};
    in.moves.initialMovesIndependentlyVerified=true;
    in.moves.initialMoves={10,52,0,0};
    in.moves.observedMoves={10,52,53,0};
    in.moves.laterRoutes[2].move=53;
    in.moves.laterRoutes[2].count=1;
    in.moves.laterRoutes[2].exhaustive=true;
    in.moves.laterRoutes[2].options[0]={
        53,7001,H::Method::Tutor,40,60,2,0,0,true
    };
    in.nativeRibbonCapability=true;
    in.observedRibbonWords={1u,2u,4u};
    in.originalEventRibbonRules={true,{1u,2u,4u},{8u,16u,32u}};
    in.nativeRawTextCapability=true;
    in.rawNickname[0]=std::byte{0x41};
    in.rawNickname[2]=std::byte{0xFF};
    in.rawNickname[3]=std::byte{0xFF};
    in.rawOT[0]=std::byte{0x52};
    in.rawOT[2]=std::byte{0xFF};
    in.rawOT[3]=std::byte{0xFF};
    in.originalEventRawTextVerified=true;
    in.originalEventNickname=in.rawNickname;
    in.originalEventOT=in.rawOT;
    // These hashes are a synthetic equality key ONLY; not a real
    // fetched WC4/PCD SHA. Original archive identity must be verified
    // independently in a real application.
    in.archiveDigestsIndependentlyVerified=true;
    in.moveSourceArchiveSha256[0]=std::byte{0x7A};
    in.ribbonSourceArchiveSha256=in.moveSourceArchiveSha256;
    in.rawTextSourceArchiveSha256=in.moveSourceArchiveSha256;
    auto supported=U::evaluate(in);
    assert(supported.status==U::Status::KnownConditionalCandidate);
    assert(supported.moveEvidence.retainedInitialMoves==2);
    assert(supported.rawTextEvidence.fullyTerminated());
    // One single-card conditional history cannot borrow a later event
    // gift move from a different card, even with matching current moves.
    auto splitEvent=in;
    splitEvent.moves.laterRoutes[2].options[0]={
        53,7001,H::Method::Event,40,60,2,68,0,true
    };
    assert(U::evaluate(splitEvent).status==U::Status::CandidateContradicted);
    auto noStartingMoves=in;
    noStartingMoves.moves.initialMovesIndependentlyVerified=false;
    assert(U::evaluate(noStartingMoves).status==U::Status::UnknownOriginalMoves);
    // Even an incomplete search may positively establish one route if
    // all used methods are already verified. Force a truly UNVERIFIED
    // source method to exercise Unknown, not a false negative.
    auto noRoutes=in;
    noRoutes.moves.history.allRelevantRoutesEnumerated=false;
    noRoutes.moves.laterRoutes[2].options[0]={
        53,7001,H::Method::BroadPool,40,60,2,0,0,false
    };
    assert(U::evaluate(noRoutes).status==U::Status::UnknownMoveRoutes);
    auto incompleteButPositive=in;
    incompleteButPositive.moves.history.allRelevantRoutesEnumerated=false;
    assert(U::evaluate(incompleteButPositive).status==
           U::Status::KnownConditionalCandidate);
    auto wrongCard=in;wrongCard.moves.claimedCardId=68;
    assert(U::evaluate(wrongCard).status==U::Status::IdentityMismatch);
    auto missingRibbons=in;missingRibbons.nativeRibbonCapability=false;
    assert(U::evaluate(missingRibbons).status==U::Status::UnknownNativeRibbons);
    auto unknownRibbonSource=in;
    unknownRibbonSource.originalEventRibbonRules.independentlyVerified=false;
    assert(U::evaluate(unknownRibbonSource).status==U::Status::UnknownRibbonTemplate);
    auto ungroundedBadRibbon=in;
    ungroundedBadRibbon.observedRibbonWords[0]=0;
    ungroundedBadRibbon.originalEventRibbonRules.independentlyVerified=false;
    assert(U::evaluate(ungroundedBadRibbon).status==U::Status::UnknownRibbonTemplate);
    auto contradictedRibbon=in;contradictedRibbon.observedRibbonWords[1]=0;
    assert(U::evaluate(contradictedRibbon).status==U::Status::CandidateContradicted);
    auto invalidTemplate=in;invalidTemplate.originalEventRibbonRules.mustNotHave[0]=1u;
    assert(U::evaluate(invalidTemplate).status==U::Status::InconsistentRibbonTemplate);
    auto noNativeText=in;noNativeText.nativeRawTextCapability=false;
    assert(U::evaluate(noNativeText).status==U::Status::UnknownNativeRawText);
    auto noOriginalText=in;noOriginalText.originalEventRawTextVerified=false;
    assert(U::evaluate(noOriginalText).status==U::Status::UnknownOriginalRawText);
    auto differentText=in;differentText.rawOT[6]=std::byte{0xFA};
    assert(U::evaluate(differentText).status==U::Status::CandidateContradicted);
    auto unverifiedTextMismatch=in;
    unverifiedTextMismatch.rawOT[6]=std::byte{0xFA};
    unverifiedTextMismatch.originalEventRawTextVerified=false;
    assert(U::evaluate(unverifiedTextMismatch).status==
           U::Status::UnknownOriginalRawText);
    auto missingArchive=in;
    missingArchive.archiveDigestsIndependentlyVerified=false;
    assert(U::evaluate(missingArchive).status==
           U::Status::UnknownArchiveProvenance);
    auto emptyArchive=in;emptyArchive.moveSourceArchiveSha256={};
    assert(U::evaluate(emptyArchive).status==
           U::Status::UnknownArchiveProvenance);
    auto differentArchive=in;
    differentArchive.rawTextSourceArchiveSha256[0]=std::byte{0x14};
    assert(U::evaluate(differentArchive).status==
           U::Status::IncoherentArchiveProvenance);
    auto evolved=in;evolved.moves.stored.species=5;
    assert(U::evaluate(evolved).status==U::Status::KnownConditionalCandidate);
    auto noId=in;noId.moves.history.id=0;
    assert(U::evaluate(noId).status==U::Status::UnknownCardIdentity);
}
int main() {
    runCases();
    std::cout<<"Gen IV single-card event moves, ribbons, raw-text joint evidence: PASS\n";
}
