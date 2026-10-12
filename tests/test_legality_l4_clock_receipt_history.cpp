#include "Legality/Gen4EventClockReceiptEvidence.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace D=Legality::Gen4EventClockReceiptEvidence;
int main(){
  D::Evidence e{}; e.releaseWindowIndependentlyVerified=true;
  e.first={2008,10,1};e.last={2009,5,31};
  e.originalReceiptField=D::Field::Met;
  e.originalFieldUnchangedIndependentlyVerified=true;
  e.clockAccurateAtOriginalReceiptIndependentlyVerified=true;
  e.noLaterDateRewriteIndependentlyVerified=true;
  auto yes=D::evaluate(true,{2007,1,1},{2009,4,5},e);
  assert(yes.status==D::Status::CandidateWithinWindow&&!yes.authenticated);
  auto outside=D::evaluate(true,{2009,2,1},{2010,1,1},e);
  assert(outside.status==D::Status::CandidateOutsideWindow);
  for(unsigned mask=0;mask<64;++mask){
    auto s=e;
    s.releaseWindowIndependentlyVerified=(mask&1u)!=0;
    s.originalFieldUnchangedIndependentlyVerified=(mask&2u)!=0;
    s.clockAccurateAtOriginalReceiptIndependentlyVerified=(mask&4u)!=0;
    s.noLaterDateRewriteIndependentlyVerified=(mask&8u)!=0;
    s.dateFieldHasEggOrHatchAmbiguity=(mask&16u)!=0;
    s.eggOrHatchHistoryComplete=(mask&32u)!=0;
    auto x=D::evaluate(true,{2008,1,1},{2010,1,1},s);
    const bool usable=(mask&15u)==15u&&(!(mask&16u)||(mask&32u));
    assert((x.status==D::Status::CandidateOutsideWindow)==usable);
    assert(!x.authenticated);
  }
  // Every byte-range native date is tested for validity without relying on
  // host system clock/timezone; even a well-formed date is not a receipt.
  uint64_t checked=0,valid=0;
  for(unsigned year=0;year<256;++year)
  for(unsigned month=0;month<=13;++month)
  for(unsigned day=0;day<=32;++day){
    const D::Date date{static_cast<uint16_t>(2000+year),
                       static_cast<uint8_t>(month),static_cast<uint8_t>(day)};
    const auto r=D::evaluate(true,{2000,1,1},date,e);
    const bool ok=D::valid(date);
    assert((r.status==D::Status::UnknownNativeCalendar)==!ok);
    if(ok)++valid;
    ++checked;
  }
  assert(D::evaluate(false,{},{},e).status==D::Status::UnknownNative);
  e.originalReceiptField=D::Field::Unknown;
  assert(D::evaluate(true,{2009,4,5},{2009,4,5},e).status==D::Status::UnknownReceiptField);
  std::cout<<"L4 Stage4 calendar: "<<checked<<" byte-date combinations, "
           <<valid<<" valid, 64 clock/receipt masks: PASS\n";
}
