#include "Legality/Gen14NativeReadinessGate.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
namespace N=Legality::Gen14NativeReadinessGate;
int main(){
 uint64_t assertions=0;
 for(auto f:{N::Format::PK1,N::Format::PK2}) {
  for(unsigned species=0;species<512;++species){
   N::Legacy c{};
   c.format=f;c.parserNationalSpecies=species;
   c.trainerId=0xAABBu;c.parserLevel=42;c.dvs={3,15,8,4};
   c.strictOriginalSaveParserSucceeded=true;
   c.strictSpeciesTranslationValidated=true;
   c.strictDvsAndLevelsValidated=true;
   const auto result=N::evaluateLegacy(c);
   const auto expected=species==0?N::Status::UnknownEmptyRecord:
           species>(f==N::Format::PK1?151u:251u)?N::Status::UnknownSpecies:
           N::Status::NativeSnapshotOnly;
   assert(result.status==expected);
   assert(!result.globalInvalidSupported&&!result.globallyLegalSupported);
   ++assertions;
   if(expected!=N::Status::NativeSnapshotOnly)continue;
   for(unsigned gate=0;gate<3;++gate){
     auto missing=c;
     if(gate==0)missing.strictOriginalSaveParserSucceeded=false;
     if(gate==1)missing.strictSpeciesTranslationValidated=false;
     if(gate==2)missing.strictDvsAndLevelsValidated=false;
     assert(N::evaluateLegacy(missing).status==N::Status::UnknownValidation);
     ++assertions;
   }
   for(unsigned dv=16;dv<256;++dv){
      auto invalid=c;invalid.dvs[0]=dv;
      assert(N::evaluateLegacy(invalid).status==N::Status::UnknownValidation);
      ++assertions;
   }
   for(unsigned level:{0u,101u,255u}){
      auto invalid=c;invalid.parserLevel=level;
      assert(N::evaluateLegacy(invalid).status==N::Status::UnknownLevel);
      ++assertions;
   }
   auto egg=c;egg.egg=true;
   assert(N::evaluateLegacy(egg).status==
       (f==N::Format::PK1?N::Status::UnknownValidation:N::Status::UnknownEggHistory));
   ++assertions;
  }
 }
 for(auto format:{N::Format::PK3,N::Format::PK4}){
  const auto at=format==N::Format::PK3?0x20u:0x08u;
  for(unsigned species=0;species<65536;++species){
    std::array<uint8_t,0xEC> storage{};
    storage[at]=static_cast<uint8_t>(species);
    storage[at+1]=static_cast<uint8_t>(species>>8);
    const uint32_t pid=0xA55A0BADu;
    storage[0]=pid&255u;storage[1]=(pid>>8)&255u;
    storage[2]=(pid>>16)&255u;storage[3]=pid>>24;
    N::Decrypted d{};
    d.format=format;d.canonical=std::span<const uint8_t>(
        storage.data(),format==N::Format::PK3?0x50u:0x88u);
    d.exactNativeRecordSizeIndependentlyValidated=true;
    d.originalCiphertextAndChecksumVerified=true;
    d.originalCiphertextDecryptionVerified=true;
    const auto result=N::evaluateDecrypted(d);
    const uint16_t national=format==N::Format::PK3?
        Pokemon::gen3InternalToNational(static_cast<uint16_t>(species)):
        static_cast<uint16_t>(species);
    const auto expected=national==0?N::Status::UnknownEmptyRecord:
       national>(format==N::Format::PK3?386u:493u)?
       N::Status::UnknownSpecies:N::Status::NativeSnapshotOnly;
    assert(result.status==expected);
    assert(!result.globalInvalidSupported&&!result.globallyLegalSupported);
    ++assertions;
    if(expected!=N::Status::NativeSnapshotOnly)continue;
    assert(result.species==national&&result.pid==pid);
    assertions+=2;
    auto missing=d;missing.originalCiphertextAndChecksumVerified=false;
    assert(N::evaluateDecrypted(missing).status==N::Status::UnknownNativeChecksum);
    ++assertions;
    missing=d;missing.originalCiphertextDecryptionVerified=false;
    assert(N::evaluateDecrypted(missing).status==N::Status::UnknownNativeDecryption);
    ++assertions;
    missing=d;missing.exactNativeRecordSizeIndependentlyValidated=false;
    assert(N::evaluateDecrypted(missing).status==N::Status::UnknownValidation);
    ++assertions;
    N::AttestedPresent att{};
    att.originalSpecimenRoot=123;att.presentSaveIdentity=456;
    att.independentlyAttestedPresentSpecies=national;
    att.independentlyAttestedPid=pid;att.independentlyAttestedOtId=result.otId;
    att.independentlyAttestedIVs=result.ivs;
    att.presentRecordAndSameSpecimenIndependentlyAttested=true;
    att.pidOtAndIvsIndependentlyAttested=true;
    att.originalSourceAndCustodyIndependentlyAttested=true;
    assert(N::checkPresent(result,att).status==
           N::Status::NativeConsistentWithAttestedHistory);
    ++assertions;
    att.originalSourceAndCustodyIndependentlyAttested=false;
    assert(N::checkPresent(result,att).status==N::Status::UnknownOriginalSave);
    ++assertions;
    att.originalSourceAndCustodyIndependentlyAttested=true;
    att.independentlyAttestedPresentSpecies^=1u;
    assert(N::checkPresent(result,att).status==N::Status::CandidateNativeIdentityMismatch);
    ++assertions;
  }
 }
 for(auto format:{N::Format::PK3,N::Format::PK4}){
   std::array<uint8_t,0xEC> bytes{};
   const unsigned speciesOff=format==N::Format::PK3?0x20u:0x08u;
   const unsigned ivOff=format==N::Format::PK3?0x48u:0x38u;
   bytes[speciesOff]=25;
   bytes[ivOff+3]=0x40u;
   for(std::size_t len=0;len<=0xECu;++len){
     N::Decrypted input{};
     input.format=format;input.canonical=std::span<const uint8_t>(bytes.data(),len);
     input.exactNativeRecordSizeIndependentlyValidated=true;
     input.originalCiphertextAndChecksumVerified=true;
     input.originalCiphertextDecryptionVerified=true;
     const bool allowed=format==N::Format::PK3?
         (len==0x50u||len==0x64u):(len==0x88u||len==0xECu);
     assert(N::evaluateDecrypted(input).status==
         (allowed?N::Status::UnknownEggHistory:N::Status::UnknownSize));
     ++assertions;
   }
 }
 // Reject encrypted or alien input types: byte data cannot be reinterpreted
 // as an original-save capture witness or a writable edit buffer.
 std::array<uint8_t,0xEC> zero{};
 for(unsigned type=0;type<256;++type){
   N::Decrypted native{static_cast<N::Format>(type),zero,true,true,true};
   auto result=N::evaluateDecrypted(native);
   if(type!=static_cast<unsigned>(N::Format::PK3)&&
      type!=static_cast<unsigned>(N::Format::PK4))
       assert(result.status==N::Status::UnknownFormat);
   N::Legacy legacy{};legacy.format=static_cast<N::Format>(type);
   legacy.strictOriginalSaveParserSucceeded=true;
   legacy.strictSpeciesTranslationValidated=true;
   legacy.strictDvsAndLevelsValidated=true;
   if(type!=static_cast<unsigned>(N::Format::PK1)&&
      type!=static_cast<unsigned>(N::Format::PK2))
       assert(N::evaluateLegacy(legacy).status==N::Status::UnknownFormat);
   ++assertions;
 }
 assert(Pokemon::gen3InternalToNational(332)==328);
 assert(Pokemon::gen3InternalToNational(353)==311);
 assert(Pokemon::gen3InternalToNational(276)==0);
 std::cout<<"L1 Stage7 strict native PK1/PK2 parser receipts, complete 65536+65536 "
          <<"PK3/PK4 species domains, checksum, invalid sizes and custody: "
          <<assertions<<" PASS (no global verdict)\n";
}
