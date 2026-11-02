#include "Legality/Gen6NativePK6Evidence.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
#ifdef NDEBUG
#error Do not disable assertions in legality fuzz regressions
#endif
namespace G=Legality::Gen6NativePK6;
static void put16(std::vector<std::uint8_t>& b,std::size_t p,std::uint16_t v) {
    b[p]=static_cast<std::uint8_t>(v);
    b[p+1]=static_cast<std::uint8_t>(v>>8);
}
static std::vector<std::uint8_t> make(std::size_t length,std::uint32_t ec,
    std::uint16_t species=721,std::uint8_t nature=24,unsigned ev=510) {
    std::vector<std::uint8_t> raw(length,0);
    put16(raw,0,static_cast<std::uint16_t>(ec));
    put16(raw,2,static_cast<std::uint16_t>(ec>>16));
    put16(raw,8,species);
    put16(raw,0x18,0x2345);put16(raw,0x1a,0x6789);
    raw[0x1c]=nature;
    raw[0x1e]=static_cast<std::uint8_t>(std::min(ev,252u));
    unsigned remain=ev-raw[0x1e];
    raw[0x1f]=static_cast<std::uint8_t>(std::min(remain,252u));
    remain-=raw[0x1f];raw[0x20]=static_cast<std::uint8_t>(remain);
    std::uint32_t sum=0;
    for(std::size_t p=8;p<G::StoredBytes;p+=2)sum+=G::read16(raw,p);
    put16(raw,6,static_cast<std::uint16_t>(sum));
    return raw;
}
// Test-only PKHeX Encrypt67 counterpart, no encryption facility in production.
static std::vector<std::uint8_t> encrypt(std::vector<std::uint8_t> plain) {
    const auto ec=G::read32(plain,0);
    const auto index=static_cast<std::size_t>((ec>>13)&31u)%24;
    const auto original=plain;
    for(std::size_t slot=0;slot<4;++slot)
        for(std::size_t p=0;p<G::BlockBytes;++p)
            plain[8+static_cast<std::size_t>(G::BlockPosition[index][slot])*G::BlockBytes+p]=
                original[8+slot*G::BlockBytes+p];
    G::xorWords(std::span<std::uint8_t>(plain).subspan(8,G::StoredPayload),ec);
    if(plain.size()==G::PartyBytes)
        G::xorWords(std::span<std::uint8_t>(plain).subspan(G::StoredBytes,28),ec);
    return plain;
}
int main() {
    std::uint64_t mutations=0,verified=0;
    for(const auto length:{G::StoredBytes,G::PartyBytes}) {
      for(unsigned shuffle=0;shuffle<32;++shuffle) {
        const std::uint32_t ec=(0x92345678u & ~(31u<<13)) | (shuffle<<13);
        auto plaintext=make(length,ec);
        assert(G::inspect(plaintext,false).status==G::Status::Parsed);
        const auto cipher=encrypt(plaintext);
        assert(cipher!=plaintext);
        const auto original=cipher;
        const auto good=G::inspect(cipher,true);
        assert(good.status==G::Status::Parsed && good.checksumVerified);
        assert(good.species==721 && good.nature==24 && good.evTotal==510);
        assert(good.pid==0x67892345u && good.encryptionConstant==ec);
        assert(!good.originalSourceAuthenticated && cipher==original);
        for(std::size_t offset=8;offset<G::StoredBytes;++offset)
          for(unsigned bit=0;bit<8;++bit) {
            auto damaged=cipher;damaged[offset]^=static_cast<std::uint8_t>(1u<<bit);
            const auto bad=G::inspect(damaged,true);
            assert(bad.status==G::Status::InvalidChecksum);
            ++mutations;
          }
        auto sanity=cipher;sanity[4]=1;
        assert(G::inspect(sanity,true).status==G::Status::InvalidSanity);
        assert(G::inspect(encrypt(make(length,ec,722)),true).status==G::Status::InvalidSpecies);
        assert(G::inspect(encrypt(make(length,ec,721,25)),true).status==G::Status::InvalidNature);
        assert(G::inspect(encrypt(make(length,ec,721,24,511)),true).status==G::Status::InvalidEVTotal);
        auto overCap=make(length,ec);overCap[0x1e]=253;
        std::uint32_t total=0;
        for(std::size_t p=8;p<G::StoredBytes;p+=2)total+=G::read16(overCap,p);
        put16(overCap,6,static_cast<std::uint16_t>(total));
        assert(G::inspect(encrypt(overCap),true).status==G::Status::InvalidEVPerStat);
        ++verified;
      }
      auto empty=make(length,0x12344321u,0,0,0);
      assert(G::inspect(encrypt(empty),true).status==G::Status::EmptySlot);
      if(length==G::PartyBytes) {
          auto party=encrypt(make(length,0x24682346u));party.back()^=0x40;
          assert(G::inspect(party,true).status==G::Status::Parsed);
      }
    }
    for(std::size_t size=0;size<280;++size) {
        if(size==G::StoredBytes || size==G::PartyBytes)continue;
        std::vector<std::uint8_t> fake(size,0xaa);
        assert(G::inspect(fake,true).status==G::Status::UnknownLength);
        assert(G::inspect(fake,false).status==G::Status::UnknownLength);
    }
    assert(verified==64 && mutations==64u*224u*8u);
    std::cout<<"PK6 native evidence: "<<verified<<" stored/party permutation records, "
             <<mutations<<" mutated ciphertext bits, malformed size/fields and source immutability PASS\n";
}
