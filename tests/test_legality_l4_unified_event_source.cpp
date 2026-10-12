#include "Legality/Gen4UnifiedEventCandidateEvidence.h"
#include <cassert>
#include <iostream>
namespace U=Legality::Gen4UnifiedEventCandidateEvidence;
namespace A=Legality::Gen4EventArchiveAttestation;
namespace H=Legality::Gen14CandidateMoveHistory;
int main() {
    // Synthetic field values conditioned on a real projected Charmander
    // identity at index 0. NOT an authenticated original event gift.
    U::Input in{};
    in.moves.stored={4,7200,58364,0x0f199b50u,40,3053,16,0,0,1,7,0,true};
    in.moves.claimedCardId=67;
    in.moves.history={7001,60,40,true,5};
    in.moves.initialMovesIndependentlyVerified=true;
    in.moves.initialMoves={10,52,0,0};
    in.moves.observedMoves={10,52,53,0};
    in.moves.laterRoutes[2].move=53;
    in.moves.laterRoutes[2].count=1;
    in.moves.laterRoutes[2].options[0]=
        {53,7001,H::Method::Tutor,40,60,2,0,0,true};
    in.nativeRibbonCapability=true;
    in.originalEventRibbonRules={true,{1u,2u,4u},{8u,16u,32u}};
    in.observedRibbonWords={1u,2u,4u};
    in.nativeRawTextCapability=true;
    in.rawNickname[2]=std::byte{0xff};
    in.rawNickname[3]=std::byte{0xff};
    in.rawOT[2]=std::byte{0xff};
    in.rawOT[3]=std::byte{0xff};
    in.originalEventRawTextVerified=true;
    in.originalNicknameRawBytesStillComparable=true;
    in.originalOTRawBytesStillComparable=true;
    in.originalEventNickname=in.rawNickname;
    in.originalEventOT=in.rawOT;
    in.archiveDigestsIndependentlyVerified=true;
    in.moveSourceArchiveSha256[0]=std::byte{0x4a};
    in.ribbonSourceArchiveSha256=in.moveSourceArchiveSha256;
    in.rawTextSourceArchiveSha256=in.moveSourceArchiveSha256;
    A::Source s{};
    s.projectionIndex=0;s.cardId=67;s.species=4;
    s.grade=A::ArchiveGrade::OriginalPCD;
    s.originalPayloadSha256=in.moveSourceArchiveSha256;
    s.digestIndependentlyChecked=true;
    s.releaseIndependentlyCorroborated=true;
    for(auto& f:s.fields) {
        f.origin=A::FieldOrigin::OriginalPayload;
        f.payloadSha256=s.originalPayloadSha256;
    }
    // Hypothetical affirmative witness; attestation must be supplied by L5
    // from actual per-card original archive records before use in production.
    assert(U::evaluateAttested(in,s).status==U::AttestedStatus::ConditionalWitness);
    auto noSource=s;noSource.grade=A::ArchiveGrade::Unverified;
    assert(U::evaluateAttested(in,noSource).status==U::AttestedStatus::UnknownArchive);
    noSource=s;noSource.grade=A::ArchiveGrade::ReconstructedFromPGT;
    assert(U::evaluateAttested(in,noSource).status==U::AttestedStatus::UntrustedArchive);
    noSource=s;noSource.fields[0].origin=A::FieldOrigin::Unavailable;
    assert(U::evaluateAttested(in,noSource).status==U::AttestedStatus::UnknownOriginalFields);
    noSource=s;noSource.projectionIndex=4;
    assert(U::evaluateAttested(in,noSource).status==U::AttestedStatus::IdentityMismatch);
    auto swapped=in;swapped.ribbonSourceArchiveSha256[1]=std::byte{0xdd};
    assert(U::evaluateAttested(swapped,s).status==U::AttestedStatus::IncoherentArchive);
    auto unknown=in;unknown.originalEventRawTextVerified=false;
    assert(U::evaluateAttested(unknown,s).status==U::AttestedStatus::UnresolvedCandidate);
    auto noMoves=in;noMoves.moves.initialMovesIndependentlyVerified=false;
    assert(U::evaluateAttested(noMoves,s).status==U::AttestedStatus::UnresolvedCandidate);
    auto candidateWrong=in;candidateWrong.observedRibbonWords[1]=0;
    assert(U::evaluateAttested(candidateWrong,s).status==U::AttestedStatus::CandidateContradicted);
    auto moved=in;moved.moves.stored.species=5;
    assert(U::evaluateAttested(moved,s).status==U::AttestedStatus::UnresolvedCandidate);
    // A caller who falsely asserts identical bytes after evolution must
    // never force an event nickname mismatch into a global assertion.
    moved.rawNickname[6]=std::byte{0x58};
    assert(U::evaluateAttested(moved,s).status==U::AttestedStatus::UnresolvedCandidate);
    assert(in.moves.stored.species==4 && in.observedRibbonWords[1]==2u);
    std::cout<<"L4 single-row attested event joint candidate: PASS\n";
}
