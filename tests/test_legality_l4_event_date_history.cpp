#include "Legality/Gen4EventDateHistoryEvidence.h"
#include "Encryption/Encryption4.h"
#include "Enums/GameVersion.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace D=Legality::Gen4EventDateHistoryEvidence;
using Status=D::Status;
void w16(std::vector<std::byte>& d,size_t p,uint16_t n) {
    d[p]=static_cast<std::byte>(n&255u);
    d[p+1]=static_cast<std::byte>(n>>8u);
}
int main() {
    constexpr D::Date egg{2009,7,10};
    constexpr D::Date met{2009,8,1};
    constexpr D::SourceWindow fixed{
        true,{2009,7,1},{2009,7,31},
        D::Field::EggDate,true,true
    };
    static_assert(D::evaluate(true,egg,met,fixed).status==
                  Status::CandidateWithinReleaseWindow);
    auto range=fixed;range.first={2009,8,1};range.last={2009,8,31};
    assert(D::evaluate(true,egg,met,range).status==Status::CandidateOutsideReleaseWindow);
    assert(D::evaluate(false,egg,met,fixed).status==Status::UnavailableNative);
    range=fixed;range.originalClockHistoryIndependentlyKnown=false;
    assert(D::evaluate(true,egg,met,range).status==Status::UnknownOriginalDateHistory);
    range=fixed;range.originalReceiptFieldUnchanged=false;
    assert(D::evaluate(true,egg,met,range).status==Status::UnknownOriginalDateHistory);
    range=fixed;range.originalReceiptField=D::Field::Unknown;
    assert(D::evaluate(true,egg,met,range).status==Status::UnknownDateField);
    range=fixed;range.originalReceiptField=D::Field::MetDate;
    assert(D::evaluate(true,egg,met,range).status==Status::CandidateOutsideReleaseWindow);
    range=fixed;range.originalReleaseIndependentlyVerified=false;
    assert(D::evaluate(true,egg,met,range).status==Status::UnknownSourceWindow);
    range=fixed;range.first={2009,8,1};range.last={2009,7,1};
    assert(D::evaluate(true,egg,met,range).status==Status::UnknownSourceWindow);
    assert(D::evaluate(true,{2009,2,29},met,fixed).status==Status::UnknownNativeDate);
    assert(D::evaluate(true,{2009,0,0},met,fixed).status==Status::UnknownNativeDate);
    // Synthetic native encrypted PK4 validates exact pinned field offsets.
    std::vector<std::byte> plain(Encryption::SIZE_STORED4);
    w16(plain,0x08,490);
    plain[0x78]=std::byte{9};
    plain[0x79]=std::byte{7};
    plain[0x7a]=std::byte{10};
    plain[0x7b]=std::byte{9};
    plain[0x7c]=std::byte{8};
    plain[0x7d]=std::byte{1};
    w16(plain,0x06,Encryption::checksum4(plain));
    const auto bytes=Encryption::encryptArray4(plain);
    Pokemon::Pokemon4ReadOnly native(bytes,Enums::GameVersion::DP);
    assert(native.valid() && !native.empty());
    const std::vector<std::byte> preserved(
        native.originalEncryptedBytes().begin(),native.originalEncryptedBytes().end());
    auto actual=D::evaluateNative(native,fixed);
    assert(actual.status==Status::CandidateWithinReleaseWindow);
    assert(actual.observed.year==2009 && actual.observed.month==7 &&
           actual.observed.day==10);
    auto unknown=fixed;unknown.originalClockHistoryIndependentlyKnown=false;
    assert(D::evaluateNative(native,unknown).status==Status::UnknownOriginalDateHistory);
    auto broken=bytes;broken[0x5b]^=std::byte{0x04};
    Pokemon::Pokemon4ReadOnly invalid(broken,Enums::GameVersion::DP);
    assert(D::evaluateNative(invalid,fixed).status==Status::UnavailableNative);
    const auto after=native.originalEncryptedBytes();
    assert(after.size()==preserved.size());
    for(size_t i=0;i<after.size();++i)assert(after[i]==preserved[i]);
    std::cout<<"L4 V17 native PK4 egg/met dates and original clock history: PASS\n";
}
