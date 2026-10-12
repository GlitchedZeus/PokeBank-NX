#include "Legality/Gen14OriginalEvidenceBridge.h"
#include <cassert>
#include <array>
#include <cstdint>
#include <iostream>
using namespace Legality::Gen14OriginalEvidenceBridge;
// No enum's unrecognized integer value may be interpreted as a compatible
// source or contradiction. Control every helper's whole contiguous status
// domain plus 256 corrupt/unknown byte values.
template<class Status>
void cover(Status positive,Status negative){
 assert(map(positive)!=Class::Unknown);
 assert(map(negative)==Class::CandidateLocalContradiction);
 for(unsigned i=128;i<256;++i)
   assert(map(static_cast<Status>(i))==Class::Unknown);
}
int main(){
 cover(G1::Status::SourceSlotCompatibleRateAndHistoryUnverified,G1::Status::CandidateOriginalLevelContradicted);
 cover(F2::Status::SourceSlotCompatibleWorldAndRngUnverified,F2::Status::CandidateRodContradicted);
 cover(T2::Status::SourceRowCompatibleHistoryUnverified,T2::Status::CandidateRarityContradicted);
 cover(S2::Status::SourceSwarmRowCompatibleOriginalWorldUnverified,S2::Status::CandidateMapContradicted);
 cover(R2::Status::ConditionalSourceTransitionOnly,R2::Status::CandidateOriginalTransitionContradicted);
 cover(W3::Status::SourceTemplateCompatibleHistoryUnverified,W3::Status::CandidateRodContradicted);
 cover(RNG3::Status::StageOnlyCompatibleHistoryUnverified,RNG3::Status::CandidateNatureContradicted);
 cover(Roam3::Status::OriginalFrameCompatibleHistoryUnverified,Roam3::Status::CandidateIvsContradicted);
 cover(GC3::Status::ConditionalGameCubeOriginHistory,GC3::Status::CandidateNativeOriginContradicted);
 cover(Tree4::Status::ConditionalOriginalTreeGroup,Tree4::Status::CandidateTreeEmpty);
 cover(Honey4::Status::ConditionalOriginalHoneyHistory,Honey4::Status::CandidateTimerExpired);
 cover(JK4::Status::ConditionalOriginalJkNatureStage,JK4::Status::CandidatePidContradicted);
 static_assert(map(G1::Status::ForeignGameRowNotEvidence)==Class::Unknown);
 static_assert(map(S2::Status::ForeignGameRowNotEvidence)==Class::Unknown);
 static_assert(map(W3::Status::ForeignSource)==Class::Unknown);
 static_assert(map(RNG3::Status::UnknownSafariPokeblock)==Class::Unknown);
 static_assert(map(Honey4::Status::UnknownHistoricalTimer)==Class::Unknown);
 static_assert(map(GC3::Status::UnknownRecipientHistory)==Class::Unknown);
 static_assert(map(GC3::Status::ConditionalGameCubeOriginHistory)==Class::ConditionalNative);
 static_assert(map(JK4::Status::ConditionalOriginalJkNatureStage)==Class::RngStageOnly);
 static_assert(map(Honey4::Status::ConditionalOriginalHoneyHistory)==Class::SourceOnly);
 std::cout<<"L1 S5 all 12 source status adapters and 1536 invalid status controls: PASS\n";
}
