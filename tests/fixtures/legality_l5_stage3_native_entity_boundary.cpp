// L5 Stage 3 full-checkout gate, NOT standalone crypto success.
// Link actual immutable Pokemon4ReadOnly and Pokemon3FRLG entity classes.
// No real retail records, file writes or historical legality assertions.
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#ifdef NDEBUG
#error "Native entity QA assertions must be enabled"
#endif
namespace Trainer {
const char* getSpeciesName(uint16_t id) {return Names::getSpeciesName(id);}
const char* getItemName(uint16_t id) {return Names::getItemName(id);}
}
namespace {
using Bytes=std::vector<std::byte>;
void put16(Bytes& b,size_t offset,uint16_t v){
    b[offset]=std::byte(v);b[offset+1]=std::byte(v>>8);
}
void put32(Bytes& b,size_t offset,uint32_t v){
    put16(b,offset,uint16_t(v));put16(b,offset+2,uint16_t(v>>16));
}
Bytes pk4(uint32_t pid,bool party,bool egg){
    Bytes b(party?Encryption::SIZE_PARTY4:Encryption::SIZE_STORED4,std::byte{0});
    put32(b,0,pid);
    put16(b,8,1); // Bulbasaur; synthetic native species
    put16(b,0x0a,1);
    put16(b,0x28,33); // Tackle
    b[0x30]=std::byte{35};
    put32(b,0x38,egg?0x40000000u:0u);
    b[0x5f]=std::byte{12};
    if(party) {b[0x8c]=std::byte{5};put16(b,0x90,20);}
    put16(b,6,Encryption::checksum4(b));
    return Encryption::encryptArray4(b);
}
void test3(unsigned& checks){
    for(bool party:{false,true}){
        for(uint32_t seed=0;seed!=128;++seed){
            const size_t size=party?Encryption::SIZE_PARTY3_FRLG:Encryption::SIZE_STORED3_FRLG;
            Bytes plain(size,std::byte{0});
            put32(plain,0,seed*24u);
            put32(plain,4,0x00aacfcfu);
            put16(plain,0x20,1); // internal Bulbasaur
            put16(plain,0x2c,33);
            put32(plain,0x48,(seed&1u)?0x40000000u:0u);
            plain[0x13]=std::byte((seed&1u)?6u:2u);
            put16(plain,0x46,0x0205); // GBA game/level persisted
            put16(plain,0x1c,Encryption::checksum3FRLG(plain));
            std::unique_ptr<std::byte[]> enc(Encryption::encryptArray3FRLG(plain));
            const Bytes raw(enc.get(),enc.get()+size),saved=raw;
            Pokemon::Pokemon3FRLG pk(raw);
            assert(pk.checksumValid() && pk.speciesID()==1);
            assert(pk.move(0)==33 && pk.isEgg()==((seed&1u)!=0));
            assert(pk.getDataSize()==size && pk.iv32()==((seed&1u)?0x40000000u:0u));
            assert(raw==saved);
            // The Bad Egg header flag can be changed without the IV32 Egg flag.
            // The codec preserves it, and must not silently invent provenance.
            auto flag=raw;flag[0x13]^=std::byte{1};
            Pokemon::Pokemon3FRLG flagged(flag);
            assert(flagged.checksumValid() && flagged.isEgg()==pk.isEgg());
            assert(flagged.speciesID()==1 && raw==saved);
            checks+=8;
        }
    }
}
void test4(unsigned& checks){
    for(bool party:{false,true})for(uint32_t seed=0;seed!=128;++seed){
        const auto encrypted=pk4(seed*0x10101u+1u,party,seed%2!=0);
        const auto original=encrypted;
        Pokemon::Pokemon4ReadOnly good(encrypted,Enums::GameVersion::PT);
        assert(good.valid() && !good.empty());
        assert(good.species()==1 && good.moves()[0]==33);
        assert(good.heldItem()==1 && good.originVersion()==12);
        assert(good.isEgg()==(seed%2!=0));
        assert(good.isParty()==party);
        assert(good.originalEncryptedBytes().size()==encrypted.size());
        for(size_t offset:{size_t{8},size_t{0x28},size_t{0x38},size_t{0x5f}}){
            auto altered=encrypted;
            altered[offset]^=std::byte{1};
            const auto before=altered;
            Pokemon::Pokemon4ReadOnly rejected(altered,Enums::GameVersion::PT);
            assert(!rejected.valid() && !rejected.empty());
            assert(rejected.species()==0 && rejected.moves()==std::array<uint16_t,4>{});
            assert(altered==before);
            checks+=4;
        }
        assert(encrypted==original);
        checks+=8;
    }
    for(size_t size=0;size<Encryption::SIZE_PARTY4+9;++size){
        if(Encryption::validRecordSize4(size))continue;
        Bytes invalid(size,std::byte{0x5a}),immutable=invalid;
        Pokemon::Pokemon4ReadOnly rejected(invalid,Enums::GameVersion::PT);
        assert(!rejected.valid());
        assert(!rejected.sizeValid() && rejected.species()==0);
        assert(invalid==immutable);
        checks+=3;
    }
}
}
int main(){
    unsigned checks=0;
    test3(checks);
    test4(checks);
    assert(checks==8921u);
    std::cout<<"L5 Stage3 actual PK3/PK4 entity quarantine: "<<checks
             <<" PASS (synthetic; full-checkout production linkage)\n";
}
