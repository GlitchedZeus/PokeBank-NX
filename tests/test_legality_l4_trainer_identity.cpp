#include "Legality/Gen34EventTrainerIdentityEvidence.h"
#include "Encryption/Encryption4.h"
#include "Enums/GameVersion.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
namespace T=Legality::Gen34EventTrainerIdentityEvidence;
using S=T::FieldStatus;
void w16(std::span<std::byte> d,std::size_t p,uint16_t n) {
    d[p]=static_cast<std::byte>(n&255u);
    d[p+1]=static_cast<std::byte>(n>>8);
}
int main() {
    constexpr T::Observed pk3{7200,58364,1,3,2};
    constexpr T::Source verified{
        true,true,7200,58364,true,1,true,3,true,2,
        true,true,true,true
    };
    static_assert(T::evaluate(true,pk3,verified).allConditionalMatches());
    assert(!T::evaluate(false,pk3,verified).nativeAvailable);
    auto src=verified;src.exactOriginalGiftIndependentlyVerified=false;
    assert(T::evaluate(true,pk3,src).ids==S::UnknownOriginalSource);
    src=verified;src.originalSid=1234;
    assert(T::evaluate(true,pk3,src).ids==S::CandidateMismatch);
    src=verified;src.idsUnchanged=false;
    assert(T::evaluate(true,pk3,src).ids==S::UnknownTransform);
    src=verified;src.originalGender=0;
    assert(T::evaluate(true,pk3,src).otGender==S::CandidateMismatch);
    src=verified;src.originalVersion=4;
    assert(T::evaluate(true,pk3,src).originVersion==S::CandidateMismatch);
    src=verified;src.originalLanguage=1;
    assert(T::evaluate(true,pk3,src).language==S::CandidateMismatch);
    src.languageUnchanged=false;
    assert(T::evaluate(true,pk3,src).language==S::UnknownTransform);

    // Synthetic independently "validated" DECRYPTED canonical PK3.
    // The bool represents an external PK3 checksum verification;
    // never infer checksum trust from raw data alone.
    std::array<std::byte,0x50> raw{};
    w16(raw,0x04,7200);w16(raw,0x06,58364);
    raw[0x12]=std::byte{2};
    const uint16_t origin=static_cast<uint16_t>(
        (3u<<7u) | (1u<<15u));
    w16(raw,0x46,origin);
    const auto preserved=raw;
    assert(T::evaluateDecryptedPK3(raw,true,verified).allConditionalMatches());
    assert(raw==preserved);
    assert(!T::evaluateDecryptedPK3(raw,false,verified).nativeAvailable);
    assert(!T::evaluateDecryptedPK3(
        std::span<const std::byte>(raw.data(),32),true,verified).nativeAvailable);
    std::array<std::byte,0x64> party{};
    for(std::size_t i=0;i<raw.size();++i)party[i]=raw[i];
    assert(T::evaluateDecryptedPK3(party,true,verified).allConditionalMatches());

    // Actual checksum-valid encrypted PK4 native fields use different
    // trainer ID and gender byte positions. Compare independent source
    // metadata only when origin and context are independently known.
    std::vector<std::byte> d4(Encryption::SIZE_STORED4);
    w16(d4,0x08,4);w16(d4,0x0c,7200);w16(d4,0x0e,58364);
    d4[0x17]=std::byte{1};d4[0x5f]=std::byte{7};
    d4[0x84]=std::byte{0x80}; // top bit = OT gender 1
    w16(d4,0x06,Encryption::checksum4(d4));
    const auto enc=Encryption::encryptArray4(d4);
    Pokemon::Pokemon4ReadOnly native(enc,Enums::GameVersion::HGSS);
    assert(native.valid());
    const auto originalBytes=native.originalEncryptedBytes();
    const std::vector<std::byte> before(originalBytes.begin(),originalBytes.end());
    auto source4=verified;
    source4.originalVersion=7;source4.originalLanguage=1;
    assert(T::evaluateNativePK4(native,source4).allConditionalMatches());
    source4.originalSid=1111;
    assert(T::evaluateNativePK4(native,source4).ids==S::CandidateMismatch);
    source4.idsUnchanged=false;
    assert(T::evaluateNativePK4(native,source4).ids==S::UnknownTransform);
    auto broken=enc;broken[0x20]^=std::byte{1};
    Pokemon::Pokemon4ReadOnly invalid(broken,Enums::GameVersion::HGSS);
    assert(!T::evaluateNativePK4(invalid,source4).nativeAvailable);
    const auto after=native.originalEncryptedBytes();
    assert(after.size()==before.size());
    for(std::size_t i=0;i<after.size();++i)assert(after[i]==before[i]);
    std::cout<<"L4 V12 native PK3/PK4 trainer original identity: PASS\n";
}
