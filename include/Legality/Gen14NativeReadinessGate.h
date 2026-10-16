#pragma once
// L1 final stage: allocation-free, READ-ONLY Gen I-IV native record ingress.
// For PK1/PK2, use ONLY the production strict save parser's validated record.
// For PK3/PK4, supply ONLY a canonical decrypted record after production
// checksum/decryption validation; encrypted bytes are NEVER accepted here.
// A valid native record cannot certify its original capture, PID seed, owner,
// lead, trainer, save provenance, gift, trade, evolution or transfer history.
#include "Pokemon/SpeciesConverter3.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen14NativeReadinessGate {
enum class Format : uint8_t { Unknown,PK1,PK2,PK3,PK4 };
enum class Status : uint8_t {
 UnknownFormat,UnknownValidation,UnknownSize,UnknownEmptyRecord,
 UnknownSpecies,UnknownLevel,UnknownOriginalSave,
 UnknownNativeChecksum,UnknownNativeDecryption,
 UnknownEggHistory,NativeSnapshotOnly,NativeConsistentWithAttestedHistory,
 CandidateNativeIdentityMismatch
};
struct Result {
 Status status=Status::UnknownFormat;
 uint16_t species=0;
 uint32_t pid=0,otId=0;
 std::array<uint8_t,6> ivs{};
 bool isEgg=false;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
struct Legacy {
 Format format=Format::Unknown;
 // The strict PK1/PK2 source parser resolves the non-national Gen I
 // byte species and validates DV/level/party-vs-box independently.
 uint16_t parserNationalSpecies=0,trainerId=0;
 uint8_t parserLevel=0;
 std::array<uint8_t,4> dvs{}; // Atk, Def, Speed, Special
 bool strictOriginalSaveParserSucceeded=false;
 bool strictSpeciesTranslationValidated=false;
 bool strictDvsAndLevelsValidated=false;
 bool egg=false;
};
constexpr Result evaluateLegacy(const Legacy& c) noexcept {
 if(c.format!=Format::PK1&&c.format!=Format::PK2)return {};
 if(!c.strictOriginalSaveParserSucceeded ||
    !c.strictSpeciesTranslationValidated ||
    !c.strictDvsAndLevelsValidated)
    return {Status::UnknownValidation};
 if(!c.parserNationalSpecies)return {Status::UnknownEmptyRecord};
 if(c.parserNationalSpecies>(c.format==Format::PK1?151:251))
    return {Status::UnknownSpecies};
 if(!c.parserLevel||c.parserLevel>100)return {Status::UnknownLevel};
 for(const uint8_t dv:c.dvs)if(dv>15)return {Status::UnknownValidation};
 if(c.egg)return {c.format==Format::PK1 ?
                   Status::UnknownValidation:Status::UnknownEggHistory,
                   c.parserNationalSpecies};
 return {Status::NativeSnapshotOnly,c.parserNationalSpecies,0,c.trainerId};
}
struct Decrypted {
 Format format=Format::Unknown;
 std::span<const uint8_t> canonical{};
 bool exactNativeRecordSizeIndependentlyValidated=false;
 bool originalCiphertextAndChecksumVerified=false;
 bool originalCiphertextDecryptionVerified=false;
};
constexpr uint16_t u16(std::span<const uint8_t> s,std::size_t index) noexcept {
 return static_cast<uint16_t>(s[index] | (uint16_t(s[index+1])<<8));
}
constexpr uint32_t u32(std::span<const uint8_t> s,std::size_t index) noexcept {
 return static_cast<uint32_t>(u16(s,index)) |
        (static_cast<uint32_t>(u16(s,index+2))<<16);
}
constexpr std::array<uint8_t,6> unpackIVs(uint32_t v) noexcept {
 return {static_cast<uint8_t>((v>>0)&31u),
         static_cast<uint8_t>((v>>5)&31u),
         static_cast<uint8_t>((v>>10)&31u),
         static_cast<uint8_t>((v>>15)&31u),
         static_cast<uint8_t>((v>>20)&31u),
         static_cast<uint8_t>((v>>25)&31u)};
}
constexpr Result evaluateDecrypted(const Decrypted& c) noexcept {
 if(c.format!=Format::PK3&&c.format!=Format::PK4)
    return {};
 if(!c.exactNativeRecordSizeIndependentlyValidated)
    return {Status::UnknownValidation};
 const auto len=c.canonical.size();
 if(c.format==Format::PK3?(len!=0x50u&&len!=0x64u):
                          (len!=0x88u&&len!=0xECu))
    return {Status::UnknownSize};
 if(!c.originalCiphertextAndChecksumVerified)
    return {Status::UnknownNativeChecksum};
 if(!c.originalCiphertextDecryptionVerified)
    return {Status::UnknownNativeDecryption};
 const uint16_t stored=u16(c.canonical,c.format==Format::PK3?0x20u:0x08u);
 if(!stored)return {Status::UnknownEmptyRecord};
 const uint16_t national=c.format==Format::PK3?
     Pokemon::gen3InternalToNational(stored):stored;
 if(!national||national>(c.format==Format::PK3?386u:493u))
    return {Status::UnknownSpecies};
 const uint32_t pid=u32(c.canonical,0u);
 const uint32_t otId=u32(c.canonical,c.format==Format::PK3?4u:0x0Cu);
 const uint32_t packed=u32(c.canonical,c.format==Format::PK3?0x48u:0x38u);
 Result result{Status::NativeSnapshotOnly,national,pid,otId,unpackIVs(packed),
               (packed&0x40000000u)!=0};
 // Native eggs have a real encrypted record, but acquisition-time egg
 // provenance requires independent gift/breeding/hatching history.
 if(result.isEgg)result.status=Status::UnknownEggHistory;
 return result;
}
struct AttestedPresent {
 uint64_t originalSpecimenRoot=0,presentSaveIdentity=0;
 uint16_t independentlyAttestedPresentSpecies=0;
 uint32_t independentlyAttestedPid=0,independentlyAttestedOtId=0;
 std::array<uint8_t,6> independentlyAttestedIVs{};
 bool presentRecordAndSameSpecimenIndependentlyAttested=false;
 bool pidOtAndIvsIndependentlyAttested=false;
 bool originalSourceAndCustodyIndependentlyAttested=false;
};
constexpr Result checkPresent(Result native,const AttestedPresent& witness) noexcept {
 if(native.status!=Status::NativeSnapshotOnly)return native;
 if(!witness.originalSpecimenRoot||!witness.presentSaveIdentity||
    !witness.presentRecordAndSameSpecimenIndependentlyAttested ||
    !witness.originalSourceAndCustodyIndependentlyAttested)
    return {Status::UnknownOriginalSave};
 if(!witness.pidOtAndIvsIndependentlyAttested)
    return {Status::UnknownValidation};
 if(native.species!=witness.independentlyAttestedPresentSpecies||
    native.pid!=witness.independentlyAttestedPid||
    native.otId!=witness.independentlyAttestedOtId||
    native.ivs!=witness.independentlyAttestedIVs) {
    native.status=Status::CandidateNativeIdentityMismatch;
    return native;
 }
 native.status=Status::NativeConsistentWithAttestedHistory;
 return native;
}
static_assert(Pokemon::gen3InternalToNational(332)==328u);
static_assert(Pokemon::gen3InternalToNational(353)==311u);
static_assert(Pokemon::gen3InternalToNational(276)==0);
} // namespace Legality::Gen14NativeReadinessGate
