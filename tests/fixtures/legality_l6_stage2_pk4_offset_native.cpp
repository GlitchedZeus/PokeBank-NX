// L6 Stage 2: actual PK4 read-only entity offset-overflow regressions.
// Synthetic native encrypted PK4 only. No original provenance or write path.
// White-box access is confined to this test translation unit; production
// access modifiers/ABI remain unchanged.
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

#include "Enums/GameVersion.h"
#include "Pokemon/PersonalRecord.h"
#define private public
#include "Pokemon/Pokemon4ReadOnly.h"
#undef private
#include "Encryption/Encryption4.h"

namespace {
using Size = std::size_t;
constexpr Size MAX = std::numeric_limits<Size>::max();
void put16(std::vector<std::byte>& v, Size o, uint16_t x) {
    v[o] = static_cast<std::byte>(x);
    v[o+1] = static_cast<std::byte>(x >> 8);
}
void put32(std::vector<std::byte>& v, Size o, uint32_t x) {
    put16(v,o,static_cast<uint16_t>(x));
    put16(v,o+2,static_cast<uint16_t>(x>>16));
}
std::vector<std::byte> encrypted(uint32_t pid, bool party) {
    std::vector<std::byte> canonical(party ? Encryption::SIZE_PARTY4 :
        Encryption::SIZE_STORED4, std::byte{0});
    put32(canonical, 0, pid);
    put16(canonical, 8, 1);           // Native Bulbasaur; never claimed original.
    put16(canonical, 0x0c, 1234);
    put16(canonical, 0x0e, 5678);
    canonical[0x5f] = std::byte{12};  // Platinum origin label.
    if (party) {
        canonical[0x8c] = std::byte{5};
        put16(canonical, 0x90, 21);
    }
    put16(canonical, 6, Encryption::checksum4(canonical));
    return Encryption::encryptArray4(canonical);
}
}
int main() {
    unsigned checks = 0;
    const std::array<Size,9> beyond{MAX, MAX-1, MAX-2, MAX-3, MAX-4,
        MAX-5, MAX/2, Encryption::SIZE_PARTY4, Encryption::SIZE_PARTY4+1};
    for (bool party : {false,true}) {
        for (uint32_t i=0;i<128;++i) {
            const uint32_t pid=0x13450001u+i*0x101u;
            const auto native=encrypted(pid,party);
            const auto original=native;
            Pokemon::Pokemon4ReadOnly pk(native, Enums::GameVersion::PT);
            assert(pk.valid() && !pk.empty() && pk.pid()==pid);
            assert(pk.u16At(0)==static_cast<uint16_t>(pid & 0xffffu));
            assert(pk.u32At(0)==pid);
            for (Size o : beyond) {
                assert(pk.u16At(o)==0);
                assert(pk.u32At(o)==0);
                checks+=2;
            }
            // Near-end reads cannot partly read or use a wrapped second byte.
            const Size extent=native.size();
            assert(pk.u16At(extent-1)==0);
            assert(pk.u32At(extent-3)==0);
            assert(pk.u32At(extent-2)==0);
            assert(pk.u32At(extent-1)==0);
            assert(native==original && pk.originalEncryptedBytes().size()==extent);
            checks+=9;
        }
    }
    // Earlier Stage 2 intake fix must remain live: never retain an attacker-
    // sized malformed span merely for a later checksum rejection.
    for (Size size : {Size{1},Size{135},Size{137},Size{235},Size{237},Size{2*1024*1024}}) {
        std::vector<std::byte> malformed(size,std::byte{0xa5});
        const auto original=malformed;
        Pokemon::Pokemon4ReadOnly pk(malformed,Enums::GameVersion::PT);
        assert(!pk.sizeValid() && !pk.valid());
        assert(pk.originalEncryptedBytes().empty() && pk.decryptedBytes().empty());
        assert(pk.u16At(MAX)==0 && pk.u32At(MAX)==0);
        assert(malformed==original);
        checks+=6;
    }
    assert(checks==6956u);
    std::cout<<"L6 Stage 2 linked PK4 boundary: "<<checks
             <<" PASS (real accessor overflow and bounded malformed intake)\n";
}
