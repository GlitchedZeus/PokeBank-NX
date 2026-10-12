#include "Legality/Gen4EventBallHistoryEvidence.h"
#include "Encryption/Encryption4.h"
#include "Enums/GameVersion.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace B=Legality::Gen4EventBallHistoryEvidence;
using Status=B::Status;
void w16(std::vector<std::byte>& d,size_t p,uint16_t n) {
    d[p]=static_cast<std::byte>(n&255u);d[p+1]=static_cast<std::byte>(n>>8);
}
int main() {
    constexpr B::Native wc4{4,4,16,0,3053,0,40,false};
    constexpr B::SourceEvidence archive{
        B::Route::EventArchive,true,true,16,true
    };
    static_assert(B::evaluate(true,wc4,archive).status==Status::CandidateBallMatch);
    auto unknown=archive;unknown.exactOriginIndependentlyVerified=false;
    assert(B::evaluate(true,wc4,unknown).status==Status::UnknownOriginalRoute);
    unknown=archive;unknown.ballPreservationHistoryKnown=false;
    assert(B::evaluate(true,wc4,unknown).status==Status::UnknownHistoricalRetention);
    unknown=archive;unknown.originalBallIndependentlyVerified=false;
    assert(B::evaluate(true,wc4,unknown).status==Status::UnknownOriginalBall);
    unknown=archive;unknown.originalBall=24;
    assert(B::evaluate(true,wc4,unknown).status==Status::CandidateBallMismatch);
    unknown=archive;unknown.originalBall=0;
    assert(B::evaluate(true,wc4,unknown).status==Status::UnknownOriginalBall);
    unknown=archive;unknown.originalBall=25;
    assert(B::evaluate(true,wc4,unknown).status==Status::UnknownOriginalBall);
    assert(B::evaluate(false,wc4,archive).status==Status::UnavailableNative);
    constexpr B::SourceEvidence pal{
        B::Route::PalParkGen3,true,true,12,true
    };
    constexpr B::Native transferred{3,12,0,0x37,0,0,15,false};
    static_assert(B::evaluate(true,transferred,pal).status==Status::CandidateBallMatch);
    auto pt=transferred;pt.metLocationExtended=0x37;pt.ballHGSS=12;
    assert(B::evaluate(true,pt,pal).status==Status::CandidateBallMatch);
    pt.ballHGSS=11;
    assert(B::evaluate(true,pt,pal).status==Status::CandidateSplitFieldsMismatch);
    auto notPal=transferred;notPal.metLocationDP=1;
    assert(B::evaluate(true,notPal,pal).status==Status::UnknownSplitFieldOrigin);
    auto notGen3=transferred;notGen3.originGeneration=4;
    assert(B::evaluate(true,notGen3,pal).status==Status::UnknownSplitFieldOrigin);
    auto pAlt=pal;pAlt.originalBall=8;
    assert(B::evaluate(true,transferred,pAlt).status==Status::CandidateBallMismatch);
    constexpr B::SourceEvidence eggs{B::Route::OrdinaryEgg,true,false,0,false};
    // The accepted ordinary-egg analyzer has its own historical guards;
    // prove the route is not automatically passed by all ball-4 records.
    constexpr B::Native empty{4,4,0,0,0,0,1,false};
    assert(B::evaluate(true,empty,eggs).status==Status::UnknownOrdinaryEggHistory);
    // Synthetic encrypted PK4 with distinct DPPt and HGSS source bytes.
    std::vector<std::byte> raw(Encryption::SIZE_STORED4);
    w16(raw,0x08,4);w16(raw,0x46,3053);
    raw[0x5f]=std::byte{7};raw[0x83]=std::byte{4};
    raw[0x86]=std::byte{16};raw[0x84]=std::byte{40};
    w16(raw,0x06,Encryption::checksum4(raw));
    const auto cipher=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly native(cipher,Enums::GameVersion::HGSS);
    assert(native.valid());
    const auto saved=native.originalEncryptedBytes();
    const std::vector<std::byte> before(saved.begin(),saved.end());
    auto result=B::evaluateNative(native,archive);
    assert(result.status==Status::CandidateBallMatch && result.observedBall==16);
    assert(B::evaluateNative(native).status==Status::UnknownOriginalRoute);
    auto corrupt=cipher;corrupt[0x32]^=std::byte{1};
    Pokemon::Pokemon4ReadOnly invalid(corrupt,Enums::GameVersion::HGSS);
    assert(B::evaluateNative(invalid,archive).status==Status::UnavailableNative);
    const auto after=native.originalEncryptedBytes();
    assert(after.size()==before.size());
    for(size_t i=0;i<before.size();++i)assert(after[i]==before[i]);
    std::cout<<"L4 V16 event, Pal Park and native PK4 ball history: PASS\n";
}
