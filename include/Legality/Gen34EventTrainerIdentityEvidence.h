#pragma once
// L4 V12: independently attested original trainer & language source fields
// across native PK3 (validated decrypted) and strict immutable native PK4.
// Pinned PKHeX PK3.cs: TID/SID 0x04/0x06, origin bits 0x46;
// language 0x12. PK4.cs TID/SID 0x0c/0x0e, language 0x17,
// origin 0x5f, OT gender top bit 0x84.
// This is *candidate-local* evidence, not an event authenticity proof.
#include "Pokemon/Pokemon4ReadOnly.h"
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen34EventTrainerIdentityEvidence {
enum class FieldStatus : uint8_t {
    UnavailableNative, UnknownOriginalSource, UnknownTransform,
    ConditionalMatch, CandidateMismatch
};
struct Observed {
    uint16_t tid=0;
    uint16_t sid=0;
    uint8_t originalTrainerGender=0;
    uint8_t originVersion=0;
    uint8_t language=0;
};
struct Source {
    bool exactOriginalGiftIndependentlyVerified=false;
    bool originalIDsKnown=false;
    uint16_t originalTid=0;
    uint16_t originalSid=0;
    bool originalGenderKnown=false;
    uint8_t originalGender=0;
    bool originalOriginVersionKnown=false;
    uint8_t originalVersion=0;
    bool originalLanguageKnown=false;
    uint8_t originalLanguage=0;
    // A source can become re-encoded, migrated or staged in another
    // game. Each field family needs separate history-backed comparability.
    bool idsUnchanged=false;
    bool otGenderUnchanged=false;
    bool originVersionUnchanged=false;
    bool languageUnchanged=false;
};
struct Result {
    bool nativeAvailable=false;
    Observed observed{};
    FieldStatus ids=FieldStatus::UnavailableNative;
    FieldStatus otGender=FieldStatus::UnavailableNative;
    FieldStatus originVersion=FieldStatus::UnavailableNative;
    FieldStatus language=FieldStatus::UnavailableNative;
    constexpr bool anyCandidateMismatch() const noexcept {
        return ids==FieldStatus::CandidateMismatch ||
               otGender==FieldStatus::CandidateMismatch ||
               originVersion==FieldStatus::CandidateMismatch ||
               language==FieldStatus::CandidateMismatch;
    }
    constexpr bool allConditionalMatches() const noexcept {
        return nativeAvailable && ids==FieldStatus::ConditionalMatch &&
               otGender==FieldStatus::ConditionalMatch &&
               originVersion==FieldStatus::ConditionalMatch &&
               language==FieldStatus::ConditionalMatch;
    }
};
constexpr FieldStatus compare(bool originalKnown,bool unchanged,
                              bool matched) noexcept {
    if(!originalKnown)return FieldStatus::UnknownOriginalSource;
    if(!unchanged)return FieldStatus::UnknownTransform;
    return matched?FieldStatus::ConditionalMatch:FieldStatus::CandidateMismatch;
}
constexpr Result evaluate(bool available,const Observed& native,
                          const Source& source={}) noexcept {
    Result out{};
    if(!available)return out;
    out.nativeAvailable=true;out.observed=native;
    if(!source.exactOriginalGiftIndependentlyVerified) {
        out.ids=out.otGender=out.originVersion=out.language=
            FieldStatus::UnknownOriginalSource;
        return out;
    }
    out.ids=compare(source.originalIDsKnown,source.idsUnchanged,
                    native.tid==source.originalTid &&
                    native.sid==source.originalSid);
    out.otGender=compare(source.originalGenderKnown,
                         source.otGenderUnchanged,
                         native.originalTrainerGender==source.originalGender);
    out.originVersion=compare(source.originalOriginVersionKnown,
                              source.originVersionUnchanged,
                              native.originVersion==source.originalVersion);
    out.language=compare(source.originalLanguageKnown,
                         source.languageUnchanged,
                         native.language==source.originalLanguage);
    return out;
}
constexpr uint16_t read16(std::span<const std::byte> bytes,
                          std::size_t index) noexcept {
    return static_cast<uint16_t>(
        std::to_integer<uint8_t>(bytes[index]) |
        (static_cast<uint16_t>(
            std::to_integer<uint8_t>(bytes[index+1]))<<8));
}
// The native PK3 must already have been decrypted into canonical GAEM
// substructure order AND externally checksum/sanity validated. No blind
// unpacking of ciphertext is attempted by this source-evidence helper.
constexpr Result evaluateDecryptedPK3(std::span<const std::byte> bytes,
        bool independentlyValidPK3,const Source& source={}) noexcept {
    if(!independentlyValidPK3 ||
       (bytes.size()!=0x50u && bytes.size()!=0x64u))return {};
    const uint16_t origins=read16(bytes,0x46);
    return evaluate(true,{
        read16(bytes,0x04),read16(bytes,0x06),
        static_cast<uint8_t>((origins>>15u)&1u),
        static_cast<uint8_t>((origins>>7u)&0x0fu),
        std::to_integer<uint8_t>(bytes[0x12])
    },source);
}
inline Result evaluateNativePK4(const Pokemon::Pokemon4ReadOnly& pk4,
                               const Source& source={}) noexcept {
    if(!pk4.valid() || pk4.empty())return {};
    return evaluate(true,{
        pk4.tid(),pk4.sid(),pk4.originalTrainerGender(),
        pk4.originVersion(),pk4.language()
    },source);
}
static_assert(compare(true,true,true)==FieldStatus::ConditionalMatch);
static_assert(compare(true,true,false)==FieldStatus::CandidateMismatch);
static_assert(compare(true,false,false)==FieldStatus::UnknownTransform);
} // namespace Legality::Gen34EventTrainerIdentityEvidence
