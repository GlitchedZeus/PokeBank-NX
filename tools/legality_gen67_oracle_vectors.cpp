// SPDX-License-Identifier: GPL-3.0-or-later
// Detached native PK6/PK7 field vectors; test-only encryption counterpart.
// TSV parsed by ORIGINAL PKHeX.Core.PK6/PK7 in host-side differential CI.
#include "Legality/Gen6NativePK6Evidence.h"
#include "Legality/Gen7NativePK7Evidence.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <vector>
#ifdef NDEBUG
#error differential vectors must be built with assertions
#endif
namespace G=Legality::Gen6NativePK6;
namespace S=Legality::Gen7NativePK7;
static void put16(std::vector<uint8_t>& data,size_t p,uint16_t value) {
    data[p]=uint8_t(value);data[p+1]=uint8_t(value>>8);
}
static std::vector<uint8_t> make(size_t length,uint32_t ec,uint16_t species,
                                  uint8_t nature=24,unsigned ev=510) {
    std::vector<uint8_t> data(length,0);
    put16(data,0,uint16_t(ec));put16(data,2,uint16_t(ec>>16));
    put16(data,8,species);put16(data,0x18,0x6789);put16(data,0x1a,0x4567);
    data[0x1c]=nature;
    data[0x1e]=uint8_t(std::min(ev,252u));ev-=data[0x1e];
    data[0x1f]=uint8_t(std::min(ev,252u));ev-=data[0x1f];
    data[0x20]=uint8_t(ev);
    uint32_t chk=0;
    for(size_t p=8;p<G::StoredBytes;p+=2)chk+=G::read16(data,p);
    put16(data,6,uint16_t(chk));return data;
}
static void recalc(std::vector<uint8_t>& data) {
    uint32_t sum=0;
    for(size_t i=8;i<G::StoredBytes;i+=2)sum+=G::read16(data,i);
    put16(data,6,uint16_t(sum));
}
static std::vector<uint8_t> encrypt(std::vector<uint8_t> plain) {
    const auto ec=G::read32(plain,0);
    const auto original=plain;
    const auto& p=G::BlockPosition[((ec>>13)&31u)%24u];
    for(size_t a=0;a<4;++a)for(size_t j=0;j<G::BlockBytes;++j)
        plain[8+size_t(p[a])*G::BlockBytes+j]=original[8+a*G::BlockBytes+j];
    G::xorWords(std::span<uint8_t>(plain).subspan(8,G::StoredPayload),ec);
    if(plain.size()==G::PartyBytes)
        G::xorWords(std::span<uint8_t>(plain).subspan(G::StoredBytes,28),ec);
    // PKHeX's PK6/PK7 auto-detection relies on unused UTF-16 terminators.
    // Only emit ciphertext that the original host constructor auto-detects.
    assert(G::read16(plain,0x58)!=0 || G::read16(plain,0xc8)!=0);
    return plain;
}
static const char* label(G::Status status) {
    switch(status) {
    case G::Status::Parsed:return "Parsed";
    case G::Status::InvalidSanity:return "InvalidSanity";
    case G::Status::InvalidChecksum:return "InvalidChecksum";
    case G::Status::InvalidSpecies:return "InvalidSpecies";
    case G::Status::InvalidNature:return "InvalidNature";
    case G::Status::InvalidEVPerStat:return "InvalidEVPerStat";
    case G::Status::InvalidEVTotal:return "InvalidEVTotal";
    case G::Status::EmptySlot:return "EmptySlot";
    default:return "UnknownLength";
    }
}
static uint64_t count=0;
static void emit(std::string id,const char* family,const char* encoding,
                 const std::vector<uint8_t>& data,G::Status want) {
    const auto original=data;
    const auto evidence=std::string_view(family)=="PK6" ? G::inspect(data,std::string_view(encoding)=="encrypted")
        : S::inspect(data,std::string_view(encoding)=="encrypted");
    assert(evidence.status==want && data==original);
    std::cout<<id<<'\t'<<family<<'\t'<<encoding<<'\t';
    for(uint8_t v:data)std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(v);
    std::cout<<std::dec<<'\t'<<label(evidence.status)<<'\t'<<evidence.species<<'\t'
             <<unsigned(evidence.nature)<<'\t'<<evidence.evTotal<<'\t'<<evidence.pid<<'\n';
    ++count;
}
int main() {
    for(const char* family:{"PK6","PK7"})
      for(const size_t length:{G::StoredBytes,G::PartyBytes})
        for(uint32_t sv=0;sv<32;++sv) {
          const uint16_t limit=std::string_view(family)=="PK6"?721:807;
          const uint32_t ec=(0x091bcdefu&~(31u<<13))|(sv<<13);
          const auto name=std::string(family)+"-"+std::to_string(length)+"-"+std::to_string(sv);
          auto plain=make(length,ec,limit);
          auto cipher=encrypt(plain);
          emit(name+"-parsed",family,"encrypted",cipher,G::Status::Parsed);
          emit(name+"-plain",family,"plain",plain,G::Status::Parsed);
          auto bad=cipher;bad[20]^=1;
          emit(name+"-checksum",family,"encrypted",bad,G::Status::InvalidChecksum);
          bad=cipher;bad[4]=1;
          emit(name+"-sanity",family,"encrypted",bad,G::Status::InvalidSanity);
          emit(name+"-species",family,"encrypted",encrypt(make(length,ec,limit+1)),G::Status::InvalidSpecies);
          emit(name+"-nature",family,"encrypted",encrypt(make(length,ec,25,25)),G::Status::InvalidNature);
          emit(name+"-evtotal",family,"encrypted",encrypt(make(length,ec,25,24,511)),G::Status::InvalidEVTotal);
          auto excessive=make(length,ec,25,24,0);
          excessive[0x1e]=253;recalc(excessive);
          emit(name+"-evstat",family,"encrypted",encrypt(excessive),G::Status::InvalidEVPerStat);
          emit(name+"-empty",family,"encrypted",encrypt(make(length,ec,0,0,0)),G::Status::EmptySlot);
        }
    assert(count==1152);
}
