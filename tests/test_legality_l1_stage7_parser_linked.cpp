// L1 Stage 7 production-backed native crypto/reader ingress. Synthetic
// storage fixtures are NOT retail original encounters. Source buffers remain
// unchanged; actual encrypted PK3 and PK4 are parsed by production readers.
#include "Legality/Gen14NativeReadinessGate.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Encryption/Encryption3FRLG.h"
#include "l1_native_encrypted_formats.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <vector>
namespace N=Legality::Gen14NativeReadinessGate;
namespace F=L1NativeEncryptedFormats;
static std::vector<std::byte> binary(std::span<const uint8_t> values) {
 std::vector<std::byte> bytes;bytes.reserve(values.size());
 for(uint8_t b:values)bytes.push_back(static_cast<std::byte>(b));
 return bytes;
}
static std::vector<uint8_t> octets(std::span<const std::byte> values) {
 std::vector<uint8_t> bytes;bytes.reserve(values.size());
 for(std::byte b:values)bytes.push_back(static_cast<uint8_t>(b));
 return bytes;
}
int main() {
 uint64_t pk1=0,pk2=0,pk3=0,pk4=0,unknown=0;
 // Production Gen I/II strict parser RECORD types: the actual original save
 // validators must supply attested strict parser success, not a fabricated
 // raw byte offset or a current container's title. These synthetic records
 // only assert the native-interface ABI and mutation quarantine.
 for(unsigned species=1;species<=251;++species){
   PokeVault::Integration::Gen1::PokemonRecord first{};
   PokeVault::Integration::Gen2::PokemonRecord second{};
   first.species=static_cast<uint16_t>(species);
   second.species=static_cast<uint16_t>(species);
   first.level=second.level=25;
   first.trainerId=second.trainerId=0x1234;
   first.dvs={0,15,3,8,11};second.dvs={0,15,3,8,11};
   auto make=[&](N::Format fmt,uint16_t number,
                 uint16_t tid,const std::array<uint8_t,5>& dvs){
      N::Legacy p{};p.format=fmt;p.parserNationalSpecies=number;
      p.trainerId=tid;p.parserLevel=25;
      p.dvs={dvs[1],dvs[2],dvs[3],dvs[4]};
      p.strictOriginalSaveParserSucceeded=true;
      p.strictSpeciesTranslationValidated=true;
      p.strictDvsAndLevelsValidated=true;return p;
   };
   auto a=make(N::Format::PK1,first.species,first.trainerId,first.dvs);
   auto b=make(N::Format::PK2,second.species,second.trainerId,second.dvs);
   assert(N::evaluateLegacy(b).status==N::Status::NativeSnapshotOnly);++pk2;
   if(species<=151){assert(N::evaluateLegacy(a).status==N::Status::NativeSnapshotOnly);++pk1;}
   else {assert(N::evaluateLegacy(a).status==N::Status::UnknownSpecies);++unknown;}
   a.strictOriginalSaveParserSucceeded=false;
   assert(N::evaluateLegacy(a).status==N::Status::UnknownValidation);++unknown;
   b.strictSpeciesTranslationValidated=false;
   assert(N::evaluateLegacy(b).status==N::Status::UnknownValidation);++unknown;
 }
 for(uint32_t shuffle=0;shuffle<24;++shuffle)
   for(unsigned size:{0x50u,0x64u}){
     std::vector<uint8_t> plain(size,0);
     F::wr32(plain,0,shuffle);F::wr32(plain,4,0x9B004321u+shuffle);
     // PK3 internal id 332 is NATIONAL Trapinch #328, not Plusle or
     // an invalid non-national species. This is production conversion.
     F::wr16(plain,0x20,332u);
     F::wr32(plain,0x48,0x16AC67E3u);
     auto encrypted=F::encrypt3(plain);
     assert(F::valid3(encrypted));
     const auto untouched=encrypted;
     auto bytes=binary(encrypted);
     std::unique_ptr<std::byte[]> clear{
        Encryption::decryptArray3FRLG(bytes)};
     assert(clear);
     const auto canonical=octets(std::span<const std::byte>(clear.get(),size));
     assert(F::rd16(canonical,0x20)==332u);
     const N::Decrypted input{N::Format::PK3,canonical,true,true,true};
     const auto parsed=N::evaluateDecrypted(input);
     assert(parsed.status==N::Status::NativeSnapshotOnly);
     assert(parsed.species==328&&parsed.pid==shuffle&&
            parsed.otId==0x9B004321u+shuffle);
     assert(encrypted==untouched);
     ++pk3;
     auto unwitnessed=input;unwitnessed.originalCiphertextAndChecksumVerified=false;
     assert(N::evaluateDecrypted(unwitnessed).status==N::Status::UnknownNativeChecksum);
     ++unknown;
     auto corrupt=encrypted;corrupt[0x20]^=0x40u;
     assert(!F::valid3(corrupt));++unknown;
   }
 for(uint32_t shuffle=0;shuffle<32;++shuffle)
   for(unsigned size:{0x88u,0xECu}){
     std::vector<uint8_t> canonical(size,0);
     const uint32_t pid=(shuffle<<13)|0x103u;
     F::wr32(canonical,0,pid);F::wr16(canonical,8,446);
     F::wr32(canonical,0x0Cu,0x12345678u);
     canonical[0x5F]=12u; // native Platinum origin byte
     F::wr16(canonical,0x80,20);
     canonical[0x84]=12;
     auto cipher=F::encrypt4(canonical);
     assert(F::valid4(cipher));
     const auto untouched=cipher;
     auto bytes=binary(cipher);
     Pokemon::Pokemon4ReadOnly native(bytes,Enums::GameVersion::PT);
     assert(native.valid()&&!native.empty());
     assert(native.species()==446&&native.pid()==pid);
     assert(native.originVersion()==12u);
     auto canonicalRead=octets(native.decryptedBytes());
     const N::Decrypted input{N::Format::PK4,canonicalRead,
                             native.sizeValid(),native.checksumValid()&&native.sanityValid(),
                             native.valid()};
     const auto parsed=N::evaluateDecrypted(input);
     assert(parsed.status==N::Status::NativeSnapshotOnly);
     assert(parsed.species==446&&parsed.otId==0x12345678u);
     assert(cipher==untouched);
     ++pk4;
     auto bad=bytes;bad[0x25]^=std::byte{0x40};
     Pokemon::Pokemon4ReadOnly quarantine(bad,Enums::GameVersion::PT);
     assert(!quarantine.valid());
     const N::Decrypted corrupt{N::Format::PK4,octets(quarantine.decryptedBytes()),
                                quarantine.sizeValid(),quarantine.checksumValid(),
                                quarantine.valid()};
     assert(N::evaluateDecrypted(corrupt).status==N::Status::UnknownNativeChecksum);
     ++unknown;
   }
 assert(pk1==151&&pk2==251&&pk3==48&&pk4==64);
 std::cout<<"L1 S7 production-backed PK1/PK2 strict-record ABI "<<pk1<<"/"<<pk2
          <<", PK3 native decrypt "<<pk3<<", real read-only PK4 "<<pk4
          <<", Unknown "<<unknown<<" PASS\n";
}
