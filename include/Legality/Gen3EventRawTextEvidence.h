#pragma once
// L4 V13 / META-001: Gen III encrypted PK3 record's raw-byte *witness*
// after external, independently validated decryption/checksum.
// Pinned PKHeX PK3.cs @6501f0ab46...: NicknameTrash 0x08/10 bytes,
// OriginalTrainerTrash 0x14/7 bytes, Language 0x12.
// Gen III is NOT UTF-16 and no decoded label reconstructs trash bytes.
// This does not decrypt/modify or authenticate a distribution source.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen3EventRawTextEvidence {
enum class FieldStatus : uint8_t {
    UnknownNative, UnknownOriginal, UnknownEncodingOrTransfer,
    ConditionalBytesMatch, CandidateBytesMismatch
};
struct Source {
    bool verifiedOriginalDistribution=false;
    bool originalNicknameBytesKnown=false;
    bool originalOTBytesKnown=false;
    bool originalLanguageKnown=false;
    uint8_t originalLanguage=0;
    bool nicknameUnchangedSinceReceipt=false;
    bool originalTrainerUnchangedSinceReceipt=false;
    // For eggs: native OT charset can come from containing save rather than
    // the PK3 record language. Any uncertain egg/trade conversion is Unknown.
    bool eggOrTradeReencodingPossible=false;
    std::array<std::byte,10> originalNickname{};
    std::array<std::byte,7> originalOT{};
};
struct Field {
    bool terminatorPresent=false;
    uint8_t terminatorIndex=0;
    bool bytesAfterTerminatorNonFF=false;
};
template<std::size_t N>
constexpr Field inspect(const std::array<std::byte,N>& raw) noexcept {
    Field out{};
    for(std::size_t i=0;i<N;++i) {
        if(!out.terminatorPresent && raw[i]==std::byte{0xff}) {
            out.terminatorPresent=true;
            out.terminatorIndex=static_cast<uint8_t>(i);
        } else if(out.terminatorPresent && raw[i]!=std::byte{0xff})
            out.bytesAfterTerminatorNonFF=true;
    }
    return out;
}
struct Result {
    bool nativeAvailable=false;
    uint8_t nativeLanguage=0;
    std::array<std::byte,10> nickname{};
    std::array<std::byte,7> originalTrainer{};
    Field nicknameStructure{};
    Field otStructure{};
    FieldStatus nicknameStatus=FieldStatus::UnknownNative;
    FieldStatus otStatus=FieldStatus::UnknownNative;
    constexpr bool anyCandidateMismatch() const noexcept {
        return nicknameStatus==FieldStatus::CandidateBytesMismatch ||
               otStatus==FieldStatus::CandidateBytesMismatch;
    }
    constexpr bool bothConditionalMatches() const noexcept {
        return nativeAvailable &&
               nicknameStatus==FieldStatus::ConditionalBytesMatch &&
               otStatus==FieldStatus::ConditionalBytesMatch;
    }
};
constexpr FieldStatus compare(bool sourceVerified,bool originalFieldKnown,
                              bool unchanged,bool encodingKnown,bool sameBytes) noexcept {
    if(!sourceVerified || !originalFieldKnown)
        return FieldStatus::UnknownOriginal;
    if(!unchanged || !encodingKnown)
        return FieldStatus::UnknownEncodingOrTransfer;
    return sameBytes ? FieldStatus::ConditionalBytesMatch
                     : FieldStatus::CandidateBytesMismatch;
}
// The caller must first validate the decrypted canonical PK3 record:
// the four internal substructures are encrypted/shuffled on disk and
// cannot be interpreted at offsets below unless properly decoded.
// Current host accepted 80-byte stored or 100-byte party native PK3.
constexpr Result inspectVerifiedDecrypted(
    std::span<const std::byte> decrypted,
    bool independentlyVerifiedChecksum,
    const Source& source={}) noexcept {
    Result out{};
    if(!independentlyVerifiedChecksum ||
       !(decrypted.size()==0x50u || decrypted.size()==0x64u))
        return out;
    out.nativeAvailable=true;
    out.nativeLanguage=std::to_integer<uint8_t>(decrypted[0x12]);
    for(std::size_t i=0;i<out.nickname.size();++i)
        out.nickname[i]=decrypted[0x08+i];
    for(std::size_t i=0;i<out.originalTrainer.size();++i)
        out.originalTrainer[i]=decrypted[0x14+i];
    out.nicknameStructure=inspect(out.nickname);
    out.otStructure=inspect(out.originalTrainer);
    const bool knownEncoding=source.originalLanguageKnown &&
                             source.originalLanguage==out.nativeLanguage &&
                             !source.eggOrTradeReencodingPossible;
    out.nicknameStatus=compare(source.verifiedOriginalDistribution,
                               source.originalNicknameBytesKnown,
                               source.nicknameUnchangedSinceReceipt,
                               knownEncoding,
                               out.nickname==source.originalNickname);
    out.otStatus=compare(source.verifiedOriginalDistribution,
                         source.originalOTBytesKnown,
                         source.originalTrainerUnchangedSinceReceipt,
                         knownEncoding,
                         out.originalTrainer==source.originalOT);
    return out;
}
static_assert(inspect(std::array<std::byte,3>{
    std::byte{0x40},std::byte{0xff},std::byte{0x42}
}).bytesAfterTerminatorNonFF);
static_assert(!inspect(std::array<std::byte,2>{
    std::byte{0x12},std::byte{0x34}
}).terminatorPresent);
} // namespace Legality::Gen3EventRawTextEvidence
