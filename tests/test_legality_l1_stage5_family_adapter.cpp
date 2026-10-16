#include "Legality/Gen4OriginalEvidenceFamilyAdapter.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen4OriginalEvidenceFamilyAdapter;
namespace C=Legality::Gen4OriginalAcquisitionReconciliation;
int main(){
 using E=C::Stage;
 static_assert(A::map(A::Sinnoh::Status::ConditionalOriginalHoneySource)==E::SourceTemplate);
 static_assert(A::map(A::Sinnoh::Status::ConditionalRadarSourceAwaitingPidReplay)==E::SourceTemplate);
 static_assert(A::map(A::Honey::Status::ConditionalOriginalHoneyHistory)==E::SourceTemplate);
 static_assert(A::map(A::Tree::Status::ConditionalOriginalTreeGroup)==E::SourceTemplate);
 static_assert(A::map(A::Special::Status::KnownConditionalSpecialEncounter)==E::OriginalRngMatched);
 static_assert(A::map(A::Jk::Status::ConditionalOriginalJkNatureStage)==E::OriginalRngMatched);
 static_assert(A::map(A::Static::Status::ConditionalOriginalStaticHistory)==E::SourceTemplate);
 static_assert(A::map(A::Static::Status::ConditionalOriginalGiftEggSourceAwaitingBreedingVerification)==E::SourceTemplate);
 static_assert(A::map(A::Honey::Status::CandidateMunchlaxTreeContradicted)==E::CandidateContradiction);
 static_assert(A::map(A::Tree::Status::CandidateTreeEmpty)==E::CandidateContradiction);
 static_assert(A::map(A::Special::Status::BoundedCandidateUnresolved)==E::BoundedExhausted);
 static_assert(A::map(A::Jk::Status::UnknownSearchWindow)==E::BoundedExhausted);
 static_assert(A::map(A::Special::Status::UnknownMalformedNativeRecord)==E::Unknown);
 static_assert(A::map(A::Sinnoh::Status::UnknownMalformedOriginalIdentity)==E::Unknown);
 static_assert(A::map(A::Honey::Status::UnknownHistoricalTimer)==E::Unknown);
 unsigned valid=0,malformed=0;
 for(unsigned raw=0;raw<256;++raw){
   const auto special=A::map(static_cast<A::Special::Status>(raw));
   const auto staticCase=A::map(static_cast<A::Static::Status>(raw));
   const auto honey=A::map(static_cast<A::Honey::Status>(raw));
   const auto tree=A::map(static_cast<A::Tree::Status>(raw));
   const auto sinnoh=A::map(static_cast<A::Sinnoh::Status>(raw));
   const auto jk=A::map(static_cast<A::Jk::Status>(raw));
   if(raw>=128){
     assert(special==E::Unknown && staticCase==E::Unknown &&
            honey==E::Unknown && tree==E::Unknown &&
            sinnoh==E::Unknown && jk==E::Unknown);
     malformed+=6;
   }else valid+=6;
 }
 for(unsigned method=0;method<256;++method){
     using K=A::Special::Category;
     const auto v=A::Special::category(static_cast<uint8_t>(method));
     if(method>=2&&method<=4)assert(v==K::Fishing);
     else if(method==5)assert(v==K::RockSmash);
     else if(method==6||method==7)assert(v==K::Headbutt);
     else if(method==8)assert(v==K::BugContest);
     else if(method>=10&&method<=14)assert(v==K::Safari);
     else assert(v==K::Unknown);
     ++valid;
 }
 C::Candidate c{};
 c.originalSaveIdentity=17;c.physicalCreatureIdentity=19;c.routeIdentity=21;
 c.game=C::Game::HeartGold;c.family=C::Family::HgssHeadbutt;
 c.originalSaveAndCreatureIndependentlyAttested=true;
 c.originalRouteAndFamilyIndependentlyAttested=true;
 c.originalSourceRowIndependentlyAttested=true;
 c.originalCaptureContextIndependentlyAttested=true;
 c.originalTreePositionAndGroupAttested=true;
 // Published HGSS Headbutt tree grouping is only a group, NOT full original
 // slot/PID/IV; the adapter cannot fabricate a stronger result.
 auto sourceOnly=A::withStage(c,A::Tree::Status::ConditionalOriginalTreeGroup);
 assert(C::evaluate(sourceOnly).status==C::Status::ConditionalOriginalSourceOnly);
 auto withoutTree=sourceOnly;withoutTree.originalTreePositionAndGroupAttested=false;
 assert(C::evaluate(withoutTree).status==C::Status::UnknownOriginalContext);
 auto bounded=A::withStage(c,A::Jk::Status::UnknownSearchWindow);
 assert(C::evaluate(bounded).status==C::Status::UnknownSearchBound);
 auto corrupt=A::withStage(c,A::Special::Status::UnknownMalformedNativeRecord);
 assert(C::evaluate(corrupt).status==C::Status::UnknownStage);
 std::cout<<"L1 Stage5 Gen IV 6 source/status adapters: "<<malformed
          <<" invalid status controls; "<<valid<<" family/method cases PASS\n";
}
