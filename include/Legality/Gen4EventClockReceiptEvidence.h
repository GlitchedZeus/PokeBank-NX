#pragma once
// L4 Stage 4: original retail release interval versus mutable Gen IV
// egg/met DS-clock bytes. A native PK4 date is never a real-time receipt.
// Clock tampering, hatching, game trades and missing receipts remain Unknown.
#include <cstdint>

namespace Legality::Gen4EventClockReceiptEvidence {
struct Date { uint16_t year=0; uint8_t month=0,day=0; };
enum class Field : uint8_t { Unknown, Egg, Met };
enum class Status : uint8_t {
    UnknownNative, UnknownReleaseWindow, UnknownReceiptField,
    UnknownClockOrTransform, UnknownNativeCalendar,
    CandidateWithinWindow, CandidateOutsideWindow
};
struct Evidence {
    bool releaseWindowIndependentlyVerified=false;
    Date first{},last{};
    Field originalReceiptField=Field::Unknown;
    bool originalFieldUnchangedIndependentlyVerified=false;
    bool clockAccurateAtOriginalReceiptIndependentlyVerified=false;
    bool noLaterDateRewriteIndependentlyVerified=false;
    // Egg/hatched ambiguity: which native date is original receipt must be
    // attested rather than selected from whichever happens to fit the window.
    bool eggOrHatchHistoryComplete=false;
    bool dateFieldHasEggOrHatchAmbiguity=false;
};
struct Result { Status status=Status::UnknownNative; Field field=Field::Unknown; Date displayed{}; bool authenticated=false; };
constexpr bool leap(uint16_t year) noexcept {
    return year%4u==0u && (year%100u!=0u || year%400u==0u);
}
constexpr unsigned days(uint16_t year,uint8_t month) noexcept {
    constexpr unsigned ds[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
    if(month<1 || month>12)return 0;
    return ds[month]+(month==2&&leap(year)?1u:0u);
}
constexpr bool valid(Date d) noexcept {
    return d.year>=2000 && d.year<=2255 && d.day>0 && d.day<=days(d.year,d.month);
}
constexpr unsigned key(Date d) noexcept {return d.year*10000u+d.month*100u+d.day;}
constexpr Result evaluate(bool nativeAvailable,Date egg,Date met,const Evidence& source={}) noexcept {
    Result r{};
    if(!nativeAvailable)return r;
    if(!source.releaseWindowIndependentlyVerified || !valid(source.first) ||
       !valid(source.last) || key(source.first)>key(source.last)) {
        r.status=Status::UnknownReleaseWindow;return r;
    }
    if(source.originalReceiptField==Field::Unknown) {
        r.status=Status::UnknownReceiptField;return r;
    }
    r.field=source.originalReceiptField;
    r.displayed=r.field==Field::Egg?egg:met;
    if(!valid(r.displayed)) {
        r.status=Status::UnknownNativeCalendar;return r;
    }
    if(!source.originalFieldUnchangedIndependentlyVerified ||
       !source.clockAccurateAtOriginalReceiptIndependentlyVerified ||
       !source.noLaterDateRewriteIndependentlyVerified ||
       (source.dateFieldHasEggOrHatchAmbiguity &&
        !source.eggOrHatchHistoryComplete)) {
        r.status=Status::UnknownClockOrTransform;return r;
    }
    r.status=key(r.displayed)>=key(source.first) &&
             key(r.displayed)<=key(source.last)?
             Status::CandidateWithinWindow:Status::CandidateOutsideWindow;
    return r;
}
static_assert(valid({2000,2,29}) && valid({2004,2,29}));
static_assert(!valid({2100,2,29}) && !valid({2009,2,29}));
static_assert(!valid({2009,4,31}) && valid({2009,4,30}));
} // namespace Legality::Gen4EventClockReceiptEvidence
