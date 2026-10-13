// L5 Stage 3: deterministic, actual production PK3/PK4 crypto fuzzing.
// Synthetic native-format data; never reads/writes source saves. No claim of
// real-game legality or PKHeX runtime oracle. Requires original .cpp codecs.
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <vector>
#ifdef NDEBUG
#error "L5 Stage 3 QA requires assertions (NDEBUG disables regressions)"
#endif
namespace {
using Bytes=std::vector<std::byte>;
uint8_t u8(std::byte b) {return static_cast<uint8_t>(b);}
uint16_t read16(const Bytes& b,size_t o){return static_cast<uint16_t>(u8(b[o])|(uint16_t(u8(b[o+1]))<<8));}
uint32_t read32(const Bytes& b,size_t o){return uint32_t(read16(b,o))|(uint32_t(read16(b,o+2))<<16);}
void write16(Bytes& b,size_t o,uint16_t v){b[o]=std::byte(v);b[o+1]=std::byte(v>>8);}
void write32(Bytes& b,size_t o,uint32_t v){write16(b,o,uint16_t(v));write16(b,o+2,uint16_t(v>>16));}
uint32_t advance(uint32_t& x){x=x*1664525u+1013904223u;return x;}
std::array<uint32_t,2> orderedPair(uint32_t& x){return {advance(x),advance(x)};}
uint16_t independentChecksum(const Bytes& b,size_t begin,size_t end){
    uint32_t sum=0;for(size_t o=begin;o<end;o+=2)sum+=read16(b,o);
    return uint16_t(sum);
}
struct Stats {uint64_t checks=0, specimens=0, flips=0, mutations=0, sizes=0;};
void expect(bool value,Stats& s){assert(value);++s.checks;}
Bytes plain3(size_t n,uint32_t pid,uint32_t seed){
    Bytes b(n,std::byte{0});
    write32(b,0,pid);write32(b,4,advance(seed));
    for(size_t o=8;o<n;++o)b[o]=std::byte(advance(seed)>>24);
    // Header flags, species/item, moves, IV/egg, ribbons/fateful, native GBA origin.
    b[0x13]=std::byte{2};write16(b,0x20,uint16_t(1+seed%386));
    write16(b,0x22,uint16_t(seed%376));
    for(unsigned m=0;m<4;++m)write16(b,0x2c+2*m,uint16_t(1+advance(seed)%354));
    const uint32_t iv=(advance(seed)&0x3fffffffu)|((seed&1u)<<30)|(((seed>>1)&1u)<<31);
    write32(b,0x48,iv);
    write32(b,0x4c,(advance(seed)&0x7fffffffu)|((seed&2u)?0x80000000u:0));
    write16(b,0x46,uint16_t((4u<<7)|5u));
    write16(b,0x1c,independentChecksum(b,0x20,0x50));
    return b;
}
Bytes plain4(size_t n,uint32_t pid,uint32_t seed){
    Bytes b(n,std::byte{0});write32(b,0,pid);
    for(size_t o=8;o<n;++o)b[o]=std::byte(advance(seed)>>24);
    write16(b,0x04,0); // native sanity
    write16(b,0x08,uint16_t(1+seed%493));
    write16(b,0x0a,uint16_t(seed%537)); // held item
    for(unsigned i=0;i<4;++i)write16(b,0x28+2*i,uint16_t(1+advance(seed)%467));
    write32(b,0x38,(advance(seed)&0x3fffffffu)|((seed&1u)<<30)); // egg bit
    b[0x40]=std::byte(((seed>>3)&31u)<<3 | ((seed>>2)&3u)<<1 | (seed&1u));
    b[0x5f]=std::byte((seed&1u)?12:7); // stored original retail game
    write16(b,6,independentChecksum(b,8,0x88));
    return b;
}
Bytes encrypt3(const Bytes& b){
    std::unique_ptr<std::byte[]> p(Encryption::encryptArray3FRLG(b));
    assert(p);return {p.get(),p.get()+b.size()};
}
Bytes decrypt3(const Bytes& b){
    std::unique_ptr<std::byte[]> p(Encryption::decryptArray3FRLG(b));
    assert(p);return {p.get(),p.get()+b.size()};
}
void one3(size_t size,uint32_t pid,uint32_t seed,Stats& s){
    auto p=plain3(size,pid,seed);const auto immutable=p;
    expect(Encryption::checksum3FRLG(p)==read16(p,0x1c),s);
    const auto cipher=encrypt3(p),ret=decrypt3(cipher);
    expect(ret==p,s);expect(p==immutable,s);expect(cipher.size()==size,s);
    // Re-encoding does not mutate input or corrupt party-only bytes.
    expect(encrypt3(ret)==cipher,s);
    if(size==0x64) expect(std::equal(cipher.begin()+0x50,cipher.end(),p.begin()+0x50),s);
    if(pid%24==0 || pid%24==23) expect(read32(ret,0)==pid,s);
    ++s.specimens;
}
void one4(size_t size,uint32_t pid,uint32_t seed,Stats& s){
    auto p=plain4(size,pid,seed);const auto immutable=p;
    expect(Encryption::checksum4(p)==independentChecksum(p,8,0x88),s);
    expect(Encryption::validRecordSize4(size),s);
    auto cipher=Encryption::encryptArray4(p);const auto preserved=cipher;
    const auto decoded=Encryption::decryptArray4(cipher);
    expect(decoded==p,s);expect(p==immutable,s);expect(cipher==preserved,s);
    expect(Encryption::encryptArray4(decoded)==cipher,s);
    expect(read16(decoded,6)==independentChecksum(decoded,8,0x88),s);
    expect(read32(decoded,0)==pid,s);
    expect(u8(decoded[0x5f])==7||u8(decoded[0x5f])==12,s);
    ++s.specimens;
}
void codecProperties(Stats& s){
    uint32_t state=0x60dcee25u;
    // All 24 PK3 block permutations, all 32 PK4 shuffle orders,
    // box/party for each; fixed endpoints plus 384 pseudo-random runs.
    for(uint32_t shuffle=0;shuffle<24;++shuffle)
      for(size_t size : {size_t{0x50},size_t{0x64}})
        for(unsigned t=0;t<128;++t){
            const auto pair=orderedPair(state);
            one3(size,(pair[0]%100000u)*24u+shuffle,pair[1],s);
        }
    for(uint32_t shuffle=0;shuffle<32;++shuffle)
      for(size_t size : {size_t{0x88},size_t{0xec}})
        for(unsigned t=0;t<128;++t){
            const auto pair=orderedPair(state);
            one4(size,(pair[0]&~(31u<<13))|(shuffle<<13),pair[1],s);
        }
}
void exhaustiveSingleBitCipherFaults(Stats& s){
    uint32_t state=0x9840b67eu;
    // Exactly one flipped stored DATA bit, fixed original checksum.
    // A one-bit payload change alters one 16-bit checksum word by a
    // nonzero power-of-two. No general multi-bit collision claim.
    for(unsigned n=0;n<12;++n){
        const auto p=plain3(0x50+20*(n&1u),n*65791u,advance(state));
        const auto raw=encrypt3(p);
        for(size_t o=0x20;o<0x50;++o)for(unsigned bit=0;bit<8;++bit){
            auto mutated=raw;mutated[o]^=std::byte(1u<<bit);
            const auto original=mutated;
            auto out=decrypt3(mutated);
            expect(Encryption::checksum3FRLG(out)!=read16(out,0x1c),s);
            expect(mutated==original,s);++s.flips;
        }
    }
    for(unsigned n=0;n<12;++n){
        const auto p=plain4((n&1)?0xec:0x88,(n*10101u)<<13,advance(state));
        const auto raw=Encryption::encryptArray4(p);
        for(size_t o=8;o<0x88;++o)for(unsigned bit=0;bit<8;++bit){
            auto mutated=raw;mutated[o]^=std::byte(1u<<bit);
            const auto before=mutated;
            auto out=Encryption::decryptArray4(mutated);
            expect(Encryption::checksum4(out)!=read16(out,6),s);
            expect(mutated==before,s);++s.flips;
        }
    }
}
void mutationAndFormatIdentity(Stats& s){
    uint32_t rng=0x831dd303u;
    for(unsigned i=0;i<4096;++i){
        const bool third=(i&1u)==0;
        const bool party=(i&2u)!=0;
        const auto pair=orderedPair(rng);
        Bytes p=third?plain3(party?0x64:0x50,pair[0],pair[1]):
                plain4(party?0xec:0x88,pair[0],pair[1]);
        const auto original=p;
        // Deliberately change one semantic field and independently restore
        // checksum, without interpreting any mutation as authentic provenance.
        static constexpr std::array<size_t,9> off3{0x13,0x20,0x22,0x2c,0x38,0x46,0x48,0x4c,0x1f};
        static constexpr std::array<size_t,9> off4{0x40,0x08,0x0a,0x28,0x18,0x5f,0x38,0x60,0x85};
        const auto& offsets=third?off3:off4;
        for(size_t slot=0;slot<offsets.size();++slot){
            auto m=p;const auto o=offsets[slot];m[o]^=std::byte(1u<<((i+slot)%8));
            if(third)write16(m,0x1c,independentChecksum(m,0x20,0x50));
            else write16(m,6,independentChecksum(m,8,0x88));
            if(third){
                const auto encrypted=encrypt3(m),decoded=decrypt3(encrypted);
                expect(decoded==m,s);
                expect(Encryption::checksum3FRLG(decoded)==read16(decoded,0x1c),s);
            }else{
                const auto encrypted=Encryption::encryptArray4(m),decoded=Encryption::decryptArray4(encrypted);
                expect(decoded==m,s);
                expect(Encryption::checksum4(decoded)==read16(decoded,6),s);
            }
            expect(p==original,s);
            ++s.mutations;
        }
    }
}
void sizesAndHeaders(Stats& s){
    uint32_t rng=0xcade2026u;
    for(size_t size=0;size<=300;++size){
        Bytes raw(size,std::byte{0x5a}),old=raw;
        // Transport PK3 supports arbitrary lengths and copies short ones.
        auto dec=decrypt3(raw),enc=encrypt3(raw);
        expect(raw==old,s);
        if(size<0x50){expect(dec==raw,s);expect(enc==raw,s);}
        if(size>=0x50){expect(decrypt3(enc)==raw,s);expect(encrypt3(dec)==raw,s);}
        const bool valid=size==0x88||size==0xec;
        expect(Encryption::validRecordSize4(size)==valid,s);
        expect(Encryption::decryptArray4(raw).empty()==!valid,s);
        expect(Encryption::encryptArray4(raw).empty()==!valid,s);
        ++s.sizes;
    }
    for(size_t size : {size_t{0x50},size_t{0x64},size_t{0x88},size_t{0xec}}){
        const bool third=size<=0x64;
        for(unsigned i=0;i<96;++i){
            const auto pair=orderedPair(rng);
            auto p=third?plain3(size,pair[0],pair[1]):
                         plain4(size,pair[0],pair[1]);
            auto raw=third?encrypt3(p):Encryption::encryptArray4(p);
            const auto untouched=raw;
            // Header checksum tamper (PK3 data unaffected; PK4 data seeded by checksum).
            auto bad=raw;
            const size_t at=third?0x1cu:6u;
            bad[at]^=std::byte{1};
            const auto out=third?decrypt3(bad):Encryption::decryptArray4(bad);
            const uint16_t expectedChecksum=third?Encryption::checksum3FRLG(out):Encryption::checksum4(out);
            expect(expectedChecksum!=read16(out,at),s);
            expect(raw==untouched,s);
            // Gen III Bad Egg/HasSpecies/Egg header bits can disagree with
            // independently stored Egg-in-IV32 flag: codec retains raw fields.
            if(third){
                auto flags=raw;flags[0x13]^=std::byte{5};
                auto d=decrypt3(flags);
                expect(u8(d[0x13])==(u8(p[0x13])^5u),s);
                expect(read32(d,0x48)==read32(p,0x48),s);
                expect(read16(d,0x1c)==read16(p,0x1c),s);
            }
        }
    }
}
}
int main(){
    Stats s{};
    codecProperties(s);
    exhaustiveSingleBitCipherFaults(s);
    mutationAndFormatIdentity(s);
    sizesAndHeaders(s);
    assert(s.specimens==14336);
    assert(s.flips==16896);
    assert(s.mutations==36864);
    assert(s.sizes==301);
    std::cout<<"L5 Stage3 actual PK3/PK4 codec fuzz: "<<s.checks
             <<" PASS; specimens="<<s.specimens<<" single-bit="<<s.flips
             <<" metadata="<<s.mutations<<" lengths="<<s.sizes
             <<" (synthetic transport, not complete analyzer or retail proof)\n";
}
