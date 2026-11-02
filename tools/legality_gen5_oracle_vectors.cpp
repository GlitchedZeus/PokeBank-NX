// Exact Product PR #300 PK5 generates 24 permutations, stored and party.
#include "Legality/Gen5ProductNativeEvidence.h"
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#ifdef NDEBUG
#error "assertions required"
#endif
namespace P=PokeVault::Integration::Gen5;
namespace E=Legality::Gen5ProductNativeEvidence;
static std::vector<uint8_t> make(size_t len,uint32_t pid,uint16_t species=25,
                                  uint8_t nature=3,unsigned ev=510) {
    std::vector<uint8_t> d(len,0);
    P::Crypto::write16(d,0,uint16_t(pid));P::Crypto::write16(d,2,uint16_t(pid>>16));
    P::Crypto::write16(d,8,species);d[0x41]=nature;
    d[0x18]=uint8_t(ev>255?255:ev);
    auto rest=ev-d[0x18];d[0x19]=uint8_t(rest>255?255:rest);
    d[0x1A]=uint8_t(rest-d[0x19]);d[0x17]=2;d[0x5F]=21;
    P::Crypto::write16(d,0x28,33);
    if(len==P::Crypto::PartySize)d[0x8C]=50;
    return P::Crypto::encryptCandidate(d);
}
static const char* status(E::Status s) {
    switch(s) {
      case E::Status::Parsed:return "Parsed";
      case E::Status::InvalidSanity:return "InvalidSanity";
      case E::Status::InvalidChecksum:return "InvalidChecksum";
      case E::Status::InvalidSpecies:return "InvalidSpecies";
      case E::Status::InvalidNature:return "InvalidNature";
      case E::Status::InvalidEVTotal:return "InvalidEVTotal";
      case E::Status::EmptySlot:return "EmptySlot";
      default:return "UnknownLength";
    }
}
static void emit(std::string id,const std::vector<uint8_t>& bytes,E::Status expect) {
    auto original=bytes;
    auto e=E::inspect(bytes);
    assert(e.status==expect && bytes==original);
    std::cout<<id<<'\t';
    for(auto b:bytes)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(b);
    std::cout<<std::dec<<'\t'<<status(e.status)<<'\t'<<e.species
             <<'\t'<<unsigned(e.nature)<<'\t'<<e.evTotal<<'\t'<<e.pid<<'\n';
}
int main() {
    for(auto len:{P::Crypto::StoredSize,P::Crypto::PartySize}) {
      for(uint32_t i=0;i<24;++i) {
        uint32_t pid=(0x04512345u & ~(31u<<13)) | (i<<13);
        auto src=make(len,pid),id=std::to_string(len)+"-"+std::to_string(i);
        emit(id+"-valid",src,E::Status::Parsed);
        auto bad=src;bad[20]^=1;emit(id+"-checksum",bad,E::Status::InvalidChecksum);
        bad=src;bad[4]=1;emit(id+"-sanity",bad,E::Status::InvalidSanity);
        emit(id+"-species",make(len,pid,650),E::Status::InvalidSpecies);
        emit(id+"-nature",make(len,pid,25,25),E::Status::InvalidNature);
        emit(id+"-ev",make(len,pid,25,3,511),E::Status::InvalidEVTotal);
      }
      emit(std::to_string(len)+"-empty",std::vector<uint8_t>(len,0),E::Status::EmptySlot);
    }
}
