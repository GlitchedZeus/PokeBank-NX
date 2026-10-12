#pragma once
// L4 V17: source-bounded Gen IV original event egg/met date evidence.
// Verified offsets: PKHeX PK4@6501f0ab... EggYear/Month/Day 0x78..0x7A,
// MetYear/Month/Day 0x7B..0x7D. Native raw dates are NOT secure timestamps.
// Console date changes, egg hatching and trades can alter displayed dates.
// An externally verified original event release window never, by itself,
// proves a detached PK4's displayed date must fall in that window.
#include "Pokemon/Pokemon4ReadOnly.h"
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4EventDateHistoryEvidence {
enum class Field : uint8_t { Unknown, EggDate, MetDate };
enum class Status : uint8_t {
    UnavailableNative, UnknownSourceWindow, UnknownDateField,
    UnknownNativeDate, UnknownOriginalDateHistory,
    CandidateWithinReleaseWindow, CandidateOutsideReleaseWindow
};
struct Date {
    uint16_t year=0;
    uint8_t month=0;
    uint8_t day=0;
};
struct SourceWindow {
    bool originalReleaseIndependentlyVerified=false;
    Date first{};
    Date last{};
    // Date field must be demonstrably an original-source timestamp, not a
    // later hatch or a cross-game transformation.
    Field originalReceiptField=Field::Unknown;
    bool originalReceiptFieldUnchanged=false;
    // An ordinary Gen IV DS clock can be adjusted; only an independently
    // documented clock/receipt history can make out-of-window meaningful.
    bool originalClockHistoryIndependentlyKnown=false;
};
struct Result {
    Status status=Status::UnavailableNative;
    Date observed{};
    Field fieldUsed=Field::Unknown;
    bool hasNativeDate=false;
};
constexpr bool leap(uint16_t year) noexcept {
    return year%4u==0u && (year%100u!=0u || year%400u==0u);
}
constexpr uint8_t daysInMonth(uint16_t year,uint8_t month) noexcept {
    constexpr uint8_t days[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
    if(month<1 || month>12)return 0;
    return static_cast<uint8_t>(days[month]+(month==2 && leap(year)));
}
constexpr bool valid(Date d) noexcept {
    return d.year>=2000 && d.year<=2255 && d.day>=1 &&
           d.day<=daysInMonth(d.year,d.month);
}
constexpr uint32_t key(Date d) noexcept {
    return static_cast<uint32_t>(d.year)*10000u +
           static_cast<uint32_t>(d.month)*100u+d.day;
}
constexpr Date fromNative(uint8_t year,uint8_t month,uint8_t day) noexcept {
    return {static_cast<uint16_t>(2000u+year),month,day};
}
constexpr Result evaluate(bool nativeAvailable,Date eggDate,Date metDate,
                          const SourceWindow& source={}) noexcept {
    Result out{};
    if(!nativeAvailable)return out;
    if(!source.originalReleaseIndependentlyVerified ||
       !valid(source.first) || !valid(source.last) ||
       key(source.first)>key(source.last)) {
        out.status=Status::UnknownSourceWindow;return out;
    }
    if(source.originalReceiptField==Field::Unknown) {
        out.status=Status::UnknownDateField;return out;
    }
    out.fieldUsed=source.originalReceiptField;
    out.observed=source.originalReceiptField==Field::EggDate ? eggDate : metDate;
    if(!valid(out.observed)) {
        out.status=Status::UnknownNativeDate;return out;
    }
    out.hasNativeDate=true;
    if(!source.originalReceiptFieldUnchanged ||
       !source.originalClockHistoryIndependentlyKnown) {
        out.status=Status::UnknownOriginalDateHistory;return out;
    }
    out.status=(key(source.first)<=key(out.observed) &&
                key(out.observed)<=key(source.last))
        ? Status::CandidateWithinReleaseWindow
        : Status::CandidateOutsideReleaseWindow;
    return out;
}
inline Result evaluateNative(const Pokemon::Pokemon4ReadOnly& pokemon,
                             const SourceWindow& source={}) noexcept {
    if(!pokemon.valid() || pokemon.empty())return {};
    const auto bytes=pokemon.decryptedBytes();
    if(bytes.size()<0x7e)return {};
    const Date egg=fromNative(static_cast<uint8_t>(bytes[0x78]),
                              static_cast<uint8_t>(bytes[0x79]),
                              static_cast<uint8_t>(bytes[0x7a]));
    const Date met=fromNative(static_cast<uint8_t>(bytes[0x7b]),
                              static_cast<uint8_t>(bytes[0x7c]),
                              static_cast<uint8_t>(bytes[0x7d]));
    return evaluate(true,egg,met,source);
}
static_assert(valid({2000,2,29}) && valid({2004,2,29}));
static_assert(!valid({2100,2,29}) && !valid({2009,2,29}));
static_assert(!valid({2009,0,1}) && !valid({2009,13,1}));
static_assert(!valid({2009,4,31}) && valid({2009,4,30}));
} // namespace Legality::Gen4EventDateHistoryEvidence
