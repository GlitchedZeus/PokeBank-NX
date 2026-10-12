// L5 Stage 2: actual PokeBank NX PK3/PK4 encryption/checksum adversarial test.
// Compile only against original repository Encryption3FRLG.cpp and Encryption4.cpp.
// Synthetic records, NOT authenticated original retail data or global legality.
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <span>
#include <vector>
using Bytes=std::vector<std::byte>;
static unsigned long long checks=0, bitRejects=0, partyUnprotected=0;
#define CHECK(x) do {++checks; if(!(x)){std::cerr<<"L5 Stage2 FAIL line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
static uint8_t byte(std::byte x){return static_cast<uint8_t>(x);}
static uint16_t get16(const Bytes& b,size_t o){return uint16_t(byte(b[o]))|uint16_t(byte(b[o+1]))<<8;}
static void put16(Bytes& b,size_t o,uint16_t v){b[o]=std::byte(v);b[o+1]=std::byte(v>>8);}
static void put32(Bytes& b,size_t o,uint32_t v){
 for(size_t k=0;k<4;k++)b[o+k]=std::byte(v>>(8*k));
}
static Bytes gen3(size_t n,uint32_t pid,uint32_t ot,uint8_t ev,bool egg){
 Bytes plain(n);
 for(size_t o=0x20;o<0x50;o++)plain[o]=std::byte((o*37u+pid*13u+ot)%256u);
 put32(plain,0,pid);put32(plain,4,ot);put16(plain,0x20,1u);
 plain[0x38]=std::byte(ev);
 if(egg){
  const uint32_t flags=0x40000000u;
  put32(plain,0x48,flags);
 }
 put16(plain,0x1c,Encryption::checksum3FRLG(plain));
 return plain;
}
static Bytes gen4(size_t n,uint32_t pid,uint8_t ev,bool egg){
 Bytes plain(n);
 for(size_t o=8;o<n;o++)plain[o]=std::byte((o*19u+pid*11u)%256u);
 put32(plain,0,pid);put16(plain,4,0);put16(plain,8,1u);
 plain[0x18]=std::byte(ev);
 if(egg)put32(plain,0x38,0x40000000u);
 put16(plain,6,Encryption::checksum4(plain));
 return plain;
}
static Bytes decrypt3(const Bytes& cipher){
 std::unique_ptr<std::byte[]> decoded(Encryption::decryptArray3FRLG(cipher));
 CHECK(decoded!=nullptr);
 return Bytes(decoded.get(),decoded.get()+cipher.size());
}
static Bytes encrypt3(const Bytes& plain){
 std::unique_ptr<std::byte[]> encoded(Encryption::encryptArray3FRLG(plain));
 CHECK(encoded!=nullptr);
 return Bytes(encoded.get(),encoded.get()+plain.size());
}
static void pk3(){
 const uint32_t ots[]={0u,0x0000ffffu,0xfedcba98u,0xffffffffu};
 const uint8_t evs[]={0u,1u,252u,253u,254u,255u};
 for(size_t n:{size_t(80),size_t(100)})
  for(uint32_t shuffle=0;shuffle<24;shuffle++)
   for(uint32_t ot:ots)
    for(uint8_t ev:evs){
     const uint32_t pid=shuffle+0x10000u*uint32_t(ot&7u);
     const Bytes plain=gen3(n,pid,ot,ev,(shuffle&1u)!=0);
     const Bytes untouched=plain;
     const Bytes cipher=encrypt3(plain);
     CHECK(plain==untouched);
     const Bytes savedCipher=cipher;
     const Bytes recovered=decrypt3(cipher);
     CHECK(cipher==savedCipher);
     CHECK(recovered==plain);
     CHECK(get16(recovered,0x1c)==Encryption::checksum3FRLG(recovered));
     CHECK(byte(recovered[0x38])==ev);
     CHECK(encrypt3(recovered)==cipher);
     // Each encrypted bit in the PK3 checksum-covered region MUST be rejected.
     if(ev==253u&&ot==ots[0]){
      for(size_t i=0x20;i<0x50;i++)for(unsigned bit=0;bit<8;bit++){
       Bytes tampered=cipher;tampered[i]^=std::byte(1u<<bit);
       const Bytes corrupted=decrypt3(tampered);
       CHECK(get16(corrupted,0x1c)!=Encryption::checksum3FRLG(corrupted));
       CHECK(cipher==savedCipher);
       ++bitRejects;
      }
     }
     // PK3 party-only 0x50..0x63 is not checksum protected.
     if(n==100&&ev==255u&&ot==ots[0]){
      Bytes tampered=cipher;tampered[0x50]^=std::byte(1);
      const Bytes corrupted=decrypt3(tampered);
      CHECK(get16(corrupted,0x1c)==Encryption::checksum3FRLG(corrupted));
      CHECK(corrupted!=recovered);
      ++partyUnprotected;
     }
    }
}
static void pk4(){
 const uint8_t evs[]={0u,1u,252u,253u,254u,255u};
 for(size_t n:{size_t(136),size_t(236)})
  for(uint32_t shuffle=0;shuffle<32;shuffle++)
   for(uint8_t ev:evs)
    for(int egg=0;egg<2;egg++){
     const uint32_t pid=(shuffle<<13)|0x58au;
     const Bytes plain=gen4(n,pid,ev,egg!=0);
     const Bytes pristine=plain;
     const Bytes cipher=Encryption::encryptArray4(plain);
     CHECK(plain==pristine);
     CHECK(cipher.size()==n);
     const Bytes savedCipher=cipher;
     const Bytes recovered=Encryption::decryptArray4(cipher);
     CHECK(cipher==savedCipher);
     CHECK(recovered==plain);
     CHECK(get16(recovered,6)==Encryption::checksum4(recovered));
     CHECK(byte(recovered[0x18])==ev);
     CHECK(Encryption::encryptArray4(recovered)==cipher);
     if(ev==253u&&egg==0){
      for(size_t i=8;i<0x88;i++)for(unsigned bit=0;bit<8;bit++){
       Bytes tampered=cipher;tampered[i]^=std::byte(1u<<bit);
       const Bytes corrupted=Encryption::decryptArray4(tampered);
       CHECK(get16(corrupted,6)!=Encryption::checksum4(corrupted));
       CHECK(cipher==savedCipher);
       ++bitRejects;
      }
     }
     if(n==236&&ev==255u&&egg==0){
      Bytes tampered=cipher;tampered[0x88]^=std::byte(1);
      const Bytes corrupted=Encryption::decryptArray4(tampered);
      CHECK(get16(corrupted,6)==Encryption::checksum4(corrupted));
      CHECK(corrupted!=recovered);
      ++partyUnprotected;
     }
    }
 for(size_t n=0;n<260;n++){
  if(n==136||n==236)continue;
  const Bytes malformed(n,std::byte{0});
  CHECK(!Encryption::validRecordSize4(n));
  CHECK(Encryption::encryptArray4(malformed).empty());
  CHECK(Encryption::decryptArray4(malformed).empty());
  CHECK(Encryption::blankRecord4(n).empty());
 }
 for(size_t n:{size_t(136),size_t(236)}){
  CHECK(Encryption::validRecordSize4(n));
  const Bytes blank=Encryption::blankRecord4(n);
  CHECK(blank.size()==n);
  const Bytes unpacked=Encryption::decryptArray4(blank);
  CHECK(unpacked.size()==n);
  CHECK(get16(unpacked,6)==Encryption::checksum4(unpacked));
  CHECK(Encryption::encryptArray4(unpacked)==blank);
 }
}
int main(){
 pk3();pk4();
 CHECK(bitRejects==2ull*24ull*48ull*8ull+2ull*32ull*128ull*8ull);
 CHECK(partyUnprotected==24ull+32ull);
 std::cout<<"L5 Stage2 ACTUAL production PK3/PK4 codec PASS "
          <<checks<<" assertions, "<<bitRejects
          <<" covered-ciphertext single-bit quarantine controls, "
          <<partyUnprotected<<" party-only checksum blind spots (expected)"
          <<" (synthetic; no original retail parity)\n";
}
