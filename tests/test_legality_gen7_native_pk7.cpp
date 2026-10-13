#include "Legality/Gen7NativePK7Evidence.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
#ifdef NDEBUG
#error assertions required
#endif
namespace G=Legality::Gen6NativePK6;
namespace P=Legality::Gen7NativePK7;
static void put16(std::vector<uint8_t>& b,size_t n,uint16_t v){b[n]=uint8_t(v);b[n+1]=uint8_t(v>>8);}
static std::vector<uint8_t> make(size_t len,uint32_t ec,uint16_t species=807,
                                 uint8_t nature=24,unsigned ev=510) {
    std::vector<uint8_t> b(len,0);
    put16(b,0,uint16_t(ec));put16(b,2,uint16_t(ec>>16));
    put16(b,8,species);put16(b,0x18,0x789a);put16(b,0x1a,0x4567);
    b[0x1c]=nature;
    b[0x1e]=uint8_t(std::min(ev,252u));
    ev-=b[0x1e];b[0x1f]=uint8_t(std::min(ev,252u));
    ev-=b[0x1f];b[0x20]=uint8_t(ev);
    uint32_t checksum=0;
    for(size_t i=8;i<G::StoredBytes;i+=2)checksum+=G::read16(b,i);
    put16(b,6,uint16_t(checksum));return b;
}
// Independent test-only canonical -> shuffled Encrypt67 mapping.
static std::vector<uint8_t> encrypt(std::vector<uint8_t> plain){
    const auto ec=G::read32(plain,0);
    const auto shuffled=plain;
    const auto& positions=G::BlockPosition[((ec>>13)&31u)%24u];
    for(size_t block=0;block<4;++block)
        for(size_t j=0;j<G::BlockBytes;++j)
            plain[8+size_t(positions[block])*G::BlockBytes+j]=
                shuffled[8+block*G::BlockBytes+j];
    G::xorWords(std::span<uint8_t>(plain).subspan(8,G::StoredPayload),ec);
    if(plain.size()==G::PartyBytes)
        G::xorWords(std::span<uint8_t>(plain).subspan(G::StoredBytes,28),ec);
    return plain;
}
int main(){
    uint64_t valid=0,bitflips=0;
    for(auto len:{G::StoredBytes,G::PartyBytes})for(uint32_t sv=0;sv<32;++sv){
        const uint32_t ec=(0x24680135u&~(31u<<13))|(sv<<13);
        const auto plain=make(len,ec),raw=encrypt(plain),original=raw;
        const auto parsed=P::inspect(raw,true);
        assert(parsed.status==P::Status::Parsed && parsed.checksumVerified);
        assert(parsed.species==807 && parsed.nature==24 && parsed.evTotal==510);
        assert(parsed.pid==0x4567789au && parsed.encryptionConstant==ec);
        assert(!parsed.originalSourceAuthenticated && original==raw);
        assert(P::inspect(plain,false).status==P::Status::Parsed);
        assert(G::inspect(raw,true).status==G::Status::InvalidSpecies);
        for(size_t pos=8;pos<G::StoredBytes;++pos)for(unsigned bit=0;bit<8;++bit){
            auto broken=raw;broken[pos]^=uint8_t(1u<<bit);
            assert(P::inspect(broken,true).status==P::Status::InvalidChecksum);
            ++bitflips;
        }
        assert(P::inspect(encrypt(make(len,ec,808)),true).status==P::Status::InvalidSpecies);
        assert(P::inspect(encrypt(make(len,ec,25,25)),true).status==P::Status::InvalidNature);
        assert(P::inspect(encrypt(make(len,ec,25,24,511)),true).status==P::Status::InvalidEVTotal);
        auto highEv=make(len,ec,25);highEv[0x1e]=253;
        uint32_t sum=0;for(size_t k=8;k<G::StoredBytes;k+=2)sum+=G::read16(highEv,k);
        put16(highEv,6,uint16_t(sum));
        assert(P::inspect(encrypt(highEv),true).status==P::Status::InvalidEVPerStat);
        auto sanity=raw;sanity[4]=1;
        assert(P::inspect(sanity,true).status==P::Status::InvalidSanity);
        ++valid;
    }
    for(auto len:{G::StoredBytes,G::PartyBytes}){
        const auto empty=encrypt(make(len,0x12345678,0));
        assert(P::inspect(empty,true).status==P::Status::EmptySlot);
    }
    for(size_t len=0;len<301;++len)if(len!=G::StoredBytes && len!=G::PartyBytes){
        std::vector<uint8_t> raw(len,0xfe);
        assert(P::inspect(raw,true).status==P::Status::UnknownLength);
        assert(P::inspect(raw,false).status==P::Status::UnknownLength);
    }
    assert(valid==64 && bitflips==114688);
    std::cout<<"PK7 native: "<<valid<<" records and "<<bitflips
             <<" ciphertext tamper mutations PASS; no native format confusion\n";
}
