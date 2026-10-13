// L5 independent adversarial execution of exact L4 production header (no edits).
// Synthetic source-stage evidence; cannot authenticate original event or globally invalidate.
#include "Legality/Gen4EventClockReceiptEvidence.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
namespace D=Legality::Gen4EventClockReceiptEvidence;
static uint64_t checks=0, outside=0, unknown=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"L5-L4 failed line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
int main(){
 D::Evidence evidence{};evidence.releaseWindowIndependentlyVerified=true;
 evidence.first={2008,10,1};evidence.last={2009,5,31};
 evidence.originalReceiptField=D::Field::Met;
 evidence.originalFieldUnchangedIndependentlyVerified=true;
 evidence.clockAccurateAtOriginalReceiptIndependentlyVerified=true;
 evidence.noLaterDateRewriteIndependentlyVerified=true;
 for(unsigned year=0;year<256;year++)for(unsigned mon=0;mon<=13;mon++)for(unsigned day=0;day<=32;day++){
  const D::Date d{static_cast<uint16_t>(year+2000),static_cast<uint8_t>(mon),static_cast<uint8_t>(day)};
  auto result=D::evaluate(true,{2007,1,1},d,evidence);
  CHECK(!result.authenticated);
  if(!D::valid(d)){CHECK(result.status==D::Status::UnknownNativeCalendar);++unknown;}
  else {const auto expected=D::key(d)>=D::key(evidence.first)&&D::key(d)<=D::key(evidence.last)?
           D::Status::CandidateWithinWindow:D::Status::CandidateOutsideWindow;
        CHECK(result.status==expected);if(result.status==D::Status::CandidateOutsideWindow)++outside;}
  CHECK(D::evaluate(false,d,d,evidence).status==D::Status::UnknownNative);
  // Met-vs-egg field identity is part of original attestation; must not select whichever date fits.
  auto egg=evidence;egg.originalReceiptField=D::Field::Egg;
  const auto selected=D::evaluate(true,d,{2009,1,1},egg);
  CHECK(selected.status==result.status);
  // Date value is not sufficient without immutable/accurate receipt evidence.
  for(unsigned flag=0;flag<3;flag++){
   auto missing=evidence;
   if(flag==0)missing.originalFieldUnchangedIndependentlyVerified=false;
   if(flag==1)missing.clockAccurateAtOriginalReceiptIndependentlyVerified=false;
   if(flag==2)missing.noLaterDateRewriteIndependentlyVerified=false;
   const auto x=D::evaluate(true,{2009,1,1},d,missing);
   CHECK(x.status==(D::valid(d)?D::Status::UnknownClockOrTransform:D::Status::UnknownNativeCalendar));
  }
 }
 for(unsigned bits=0;bits<128;bits++){
  auto e=evidence;
  e.releaseWindowIndependentlyVerified=(bits&1u)!=0;
  e.originalFieldUnchangedIndependentlyVerified=(bits&2u)!=0;
  e.clockAccurateAtOriginalReceiptIndependentlyVerified=(bits&4u)!=0;
  e.noLaterDateRewriteIndependentlyVerified=(bits&8u)!=0;
  e.dateFieldHasEggOrHatchAmbiguity=(bits&16u)!=0;
  e.eggOrHatchHistoryComplete=(bits&32u)!=0;
  e.originalReceiptField=(bits&64u)!=0?D::Field::Unknown:D::Field::Met;
  const auto x=D::evaluate(true,{2009,1,1},{2012,1,1},e);
  const bool complete=(bits&15u)==15u && ((bits&16u)==0 || (bits&32u)!=0) && ((bits&64u)==0);
  CHECK((x.status==D::Status::CandidateOutsideWindow)==complete);
  CHECK(!x.authenticated);
 }
 // Impossible window, leap boundaries and reversed dates never produce a contradictory receipt.
 for(D::Date date: {D::Date{2100,2,29},D::Date{2009,4,31},D::Date{2007,2,29},D::Date{0,1,1}}){
  auto e=evidence;e.first=date;
  CHECK(D::evaluate(true,{2009,1,1},{2009,1,1},e).status==D::Status::UnknownReleaseWindow);
 }
 auto reverse=evidence;reverse.first={2012,1,1};reverse.last={2008,1,1};
 CHECK(D::evaluate(true,{2009,1,1},{2009,1,1},reverse).status==D::Status::UnknownReleaseWindow);
 std::cout<<"L5 Stage4 actual L4 clock helper PASS "<<checks<<" assertions; "<<outside
          <<" candidate-local out-of-window; "<<unknown
          <<" invalid native calendar quarantines (no globally Invalid/authentication)\n";
}
