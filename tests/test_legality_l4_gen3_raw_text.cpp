#include "Legality/Gen3EventRawTextEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
namespace T=Legality::Gen3EventRawTextEvidence;
using S=T::FieldStatus;
int main() {
    // Synthetic DECRYPTED canonical PK3 bytes (not a recovered cartridge
    // receipt); external checksum fact is supplied independently.
    std::array<std::byte,0x50> raw{};
    raw[0x12]=std::byte{2}; // English encoding in this hypothetical source.
    raw[0x08]=std::byte{0xbb};raw[0x09]=std::byte{0xff};
    raw[0x0a]=std::byte{0xaa}; // Retained raw trash, not decoded Unicode.
    raw[0x14]=std::byte{0xc1};raw[0x15]=std::byte{0xff};
    raw[0x16]=std::byte{0x92};
    T::Source original{};
    original.verifiedOriginalDistribution=true;
    original.originalLanguageKnown=true;
    original.originalLanguage=2;
    original.originalNicknameBytesKnown=true;
    original.originalOTBytesKnown=true;
    original.nicknameUnchangedSinceReceipt=true;
    original.originalTrainerUnchangedSinceReceipt=true;
    for(std::size_t i=0;i<10;++i)
        original.originalNickname[i]=raw[0x08+i];
    for(std::size_t i=0;i<7;++i)
        original.originalOT[i]=raw[0x14+i];
    const auto before=raw;
    const auto yes=T::inspectVerifiedDecrypted(raw,true,original);
    assert(yes.bothConditionalMatches());
    assert(yes.nativeLanguage==2);
    assert(yes.nicknameStructure.terminatorPresent &&
           yes.nicknameStructure.terminatorIndex==1 &&
           yes.nicknameStructure.bytesAfterTerminatorNonFF);
    assert(yes.otStructure.terminatorPresent &&
           yes.otStructure.bytesAfterTerminatorNonFF);
    assert(raw==before); // all native PK3 data preserved.

    auto mismatched=raw;mismatched[0x0a]=std::byte{0x33};
    auto unequal=T::inspectVerifiedDecrypted(mismatched,true,original);
    assert(unequal.nicknameStatus==S::CandidateBytesMismatch);
    assert(unequal.otStatus==S::ConditionalBytesMatch);
    assert(unequal.anyCandidateMismatch());
    mismatched=raw;mismatched[0x16]=std::byte{0x22};
    unequal=T::inspectVerifiedDecrypted(mismatched,true,original);
    assert(unequal.otStatus==S::CandidateBytesMismatch);
    assert(unequal.nicknameStatus==S::ConditionalBytesMatch);
    auto unknown=original;unknown.verifiedOriginalDistribution=false;
    assert(T::inspectVerifiedDecrypted(raw,true,unknown).nicknameStatus==S::UnknownOriginal);
    unknown=original;unknown.originalOTBytesKnown=false;
    assert(T::inspectVerifiedDecrypted(raw,true,unknown).otStatus==S::UnknownOriginal);
    unknown=original;unknown.eggOrTradeReencodingPossible=true;
    assert(T::inspectVerifiedDecrypted(mismatched,true,unknown).otStatus==
           S::UnknownEncodingOrTransfer);
    unknown=original;unknown.originalLanguage=1;
    assert(T::inspectVerifiedDecrypted(mismatched,true,unknown).otStatus==
           S::UnknownEncodingOrTransfer);
    unknown=original;unknown.nicknameUnchangedSinceReceipt=false;
    assert(T::inspectVerifiedDecrypted(mismatched,true,unknown).nicknameStatus==
           S::UnknownEncodingOrTransfer);
    assert(!T::inspectVerifiedDecrypted(raw,false,original).nativeAvailable);
    std::span<const std::byte> shortRaw(raw.data(),32);
    assert(!T::inspectVerifiedDecrypted(shortRaw,true,original).nativeAvailable);
    std::array<std::byte,0x64> party{};
    for(std::size_t i=0;i<raw.size();++i)party[i]=raw[i];
    assert(T::inspectVerifiedDecrypted(party,true,original).bothConditionalMatches());
    // A fully filled 10-byte name need not have a visible terminator.
    for(std::size_t i=0;i<10;++i)raw[0x08+i]=std::byte{0x42};
    assert(!T::inspectVerifiedDecrypted(raw,true,original).nicknameStructure.terminatorPresent);
    std::cout<<"L4 V13 PK3 raw nickname/OT source and encoding evidence: PASS\n";
}
