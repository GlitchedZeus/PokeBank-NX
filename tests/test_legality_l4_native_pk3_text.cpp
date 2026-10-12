#include "Legality/Gen3EventNativeRawTextEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
namespace N=Legality::Gen3EventNativeRawTextEvidence;
namespace T=Legality::Gen3EventRawTextEvidence;
using S=T::FieldStatus;
void w16(std::vector<std::byte>& v,size_t o,uint16_t value) {
    v[o]=static_cast<std::byte>(value&0xffu);
    v[o+1]=static_cast<std::byte>(value>>8u);
}
void w32(std::vector<std::byte>& v,size_t o,uint32_t value) {
    w16(v,o,static_cast<uint16_t>(value));
    w16(v,o+2,static_cast<uint16_t>(value>>16u));
}
int main() {
    unsigned cases=0;
    for(const auto size:{Encryption::SIZE_STORED3_FRLG,Encryption::SIZE_PARTY3_FRLG}) {
        // Exercise each of the 24 authentic Gen III encrypted block orders.
        for(uint32_t permutation=0;permutation<24;++permutation) {
            std::vector<std::byte> decrypted(size);
            w32(decrypted,0x00,permutation+24u);
            w16(decrypted,0x04,7200);w16(decrypted,0x06,58364);
            decrypted[0x12]=std::byte{2};
            decrypted[0x08]=std::byte{0xbb};decrypted[0x09]=std::byte{0xff};
            decrypted[0x0a]=std::byte{0x53};
            decrypted[0x14]=std::byte{0xc1};decrypted[0x15]=std::byte{0xff};
            decrypted[0x16]=std::byte{0x72};
            w16(decrypted,0x20,4);
            w16(decrypted,0x1c,Encryption::checksum3FRLG(decrypted));
            std::unique_ptr<std::byte[]> allocated(
                Encryption::encryptArray3FRLG(decrypted));
            std::vector<std::byte> native(allocated.get(),allocated.get()+size);
            const auto preserved=native;
            N::Source source{};
            source.verifiedOriginalDistribution=true;
            source.originalNicknameBytesKnown=true;
            source.originalOTBytesKnown=true;
            source.originalLanguageKnown=true;
            source.originalLanguage=2;
            source.nicknameUnchangedSinceReceipt=true;
            source.originalTrainerUnchangedSinceReceipt=true;
            for(size_t i=0;i<10;++i)source.originalNickname[i]=decrypted[0x08+i];
            for(size_t i=0;i<7;++i)source.originalOT[i]=decrypted[0x14+i];
            const auto yes=N::inspectEncrypted(native,source);
            assert(yes.bothConditionalMatches());
            assert(yes.nativeLanguage==2 && yes.nickname[2]==std::byte{0x53});
            assert(yes.originalTrainer[2]==std::byte{0x72});
            assert(native==preserved);
            // Changing the nonencrypted raw name is a contradiction of ONE
            // independently asserted unchanged original, not global Invalid.
            auto renamed=native;renamed[0x0a]^=std::byte{1};
            assert(N::inspectEncrypted(renamed,source).nicknameStatus==S::CandidateBytesMismatch);
            // Native OT of an independently proven reencoded trade cannot
            // be byte-compared to a hypothetical source archive's OT.
            auto traded=source;traded.eggOrTradeReencodingPossible=true;
            assert(N::inspectEncrypted(renamed,traded).nicknameStatus==S::UnknownEncodingOrTransfer);
            // An unexplained rename or a different game-font context is
            // also Unknown; never call it an event-source contradiction.
            traded=source;traded.nicknameUnchangedSinceReceipt=false;
            assert(N::inspectEncrypted(renamed,traded).nicknameStatus==S::UnknownEncodingOrTransfer);
            traded=source;traded.originalLanguage=1;
            assert(N::inspectEncrypted(renamed,traded).nicknameStatus==S::UnknownEncodingOrTransfer);
            // Valid structure does NOT supply an authenticated original.
            assert(N::inspectEncrypted(native).nicknameStatus==S::UnknownOriginal);
            // Tamper only encrypted payload: checksum no longer agrees.
            auto corrupted=native;corrupted[0x30]^=std::byte{0x40};
            assert(!N::inspectEncrypted(corrupted,source).nativeAvailable);
            auto badEgg=native;badEgg[0x13]|=std::byte{1};
            assert(!N::inspectEncrypted(badEgg,source).nativeAvailable);
            ++cases;
        }
    }
    std::array<std::byte,32> shortRecord{};
    assert(!N::inspectEncrypted(shortRecord).nativeAvailable);
    std::array<std::byte,0x88> wrongGeneration{};
    assert(!N::inspectEncrypted(wrongGeneration).nativeAvailable);
    assert(cases==48u);
    std::cout<<"L4 native encrypted PK3: 24 block orders x stored/party, "
             <<cases<<" conditional/negative/Unknown fixture sets: PASS\n";
}
