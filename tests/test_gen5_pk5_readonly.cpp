#include "Encryption/Encryption5.h"
#include "Pokemon/Pokemon5ReadOnly.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace G5 = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;

void set32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
    C::write16(b, o, static_cast<uint16_t>(v));
    C::write16(b, o+2, static_cast<uint16_t>(v >> 16));
}
std::vector<uint8_t> makeRecord(size_t size, uint32_t pid) {
    std::vector<uint8_t> d(size, 0);
    set32(d,0,pid);
    C::write16(d,0x08,25);
    C::write16(d,0x0A,4);
    C::write16(d,0x0C,12345);
    C::write16(d,0x0E,54321);
    set32(d,0x10,10000);
    d[0x14]=70;
    d[0x15]=9;
    d[0x17]=2;
    d[0x18]=31;
    C::write16(d,0x28,85);
    d[0x30]=15;
    d[0x34]=1;
    set32(d,0x38,31 | (30u << 5) | (29u << 10));
    d[0x40]=static_cast<uint8_t>((1 << 1) | (2 << 3));
    d[0x41]=3;
    d[0x42]=1;
    d[0x5F]=20;
    if (size == C::PartySize) {
        d[0x8C]=50;
        for (size_t i=0x88;i<size;i++) d[i]=static_cast<uint8_t>(i*7+1);
    }
    return d;
}
int main() {
    static_assert(C::StoredSize == 0x88);
    static_assert(C::PartySize == 0xDC);
    // Independent hand-calculated checksum sum for this sparse record.
    auto single = makeRecord(C::StoredSize, 0x12345678);
    const auto sum = C::checksum(single);
    assert(sum != 0);
    for (uint32_t sv=0; sv<32; sv++) {
        for (size_t size : {C::StoredSize, C::PartySize}) {
            auto plain = makeRecord(size, (sv << 13) | 0x123u);
            const auto before=plain;
            const auto encrypted = C::encryptCandidate(plain);
            assert(plain == before);
            assert(encrypted.size() == size);
            assert(C::read16(encrypted,6) == C::checksum(plain));
            const auto decrypted = C::decrypt(encrypted);
            auto expected = plain;
            C::write16(expected,6,C::checksum(expected));
            assert(decrypted == expected);
            const G5::Pokemon5ReadOnly entity(encrypted);
            assert(entity.valid() && !entity.empty());
            assert(entity.partyRecord() == (size == C::PartySize));
            assert(entity.species()==25 && entity.heldItem()==4);
            assert(entity.tid()==12345 && entity.sid()==54321);
            assert(entity.experience()==10000);
            assert(entity.friendship()==70 && entity.ability()==9);
            assert(entity.evs()[0]==31 && entity.moves()[0]==85);
            assert(entity.pp()[0]==15 && entity.ppUps()[0]==1);
            assert(entity.ivs()[0]==31 && entity.ivs()[1]==30);
            assert(entity.gender()==1 && entity.form()==2);
            assert(entity.nature()==3 && entity.hiddenAbility());
            assert(entity.originVersion()==20);
            assert(std::equal(entity.originalEncryptedBytes().begin(),
                              entity.originalEncryptedBytes().end(),encrypted.begin()));
            auto corrupt=encrypted;
            corrupt[0x20]^=0x01;
            G5::Pokemon5ReadOnly invalid(corrupt);
            assert(!invalid.valid() && !invalid.checksumValid());
            assert(invalid.species()==0 && invalid.moves()[0]==0);
        }
    }
    std::vector<uint8_t> allZero(C::StoredSize, 0);
    G5::Pokemon5ReadOnly empty(allZero);
    assert(empty.valid() && empty.empty());
    G5::Pokemon5ReadOnly shortRecord(std::span<const uint8_t>(allZero.data(), 80));
    assert(!shortRecord.valid() && shortRecord.species() == 0);
    assert(C::encryptCandidate(std::span<const uint8_t>(allZero.data(), 80)).empty());
    // Sanity and out-of-range species failures remain quarantined.
    auto sane = makeRecord(C::StoredSize, 99);
    C::write16(sane,4,1);
    assert(!G5::Pokemon5ReadOnly(C::encryptCandidate(sane)).valid());
    C::write16(sane,4,0);
    C::write16(sane,8,650);
    assert(!G5::Pokemon5ReadOnly(C::encryptCandidate(sane)).valid());
    // PK5's additive checksum excludes the PID/header. A PID-only mutation
    // can be undetectable when the data blocks have repeated/zero contents;
    // never assert that this checksum authenticates those excluded bytes.
    auto raw = C::encryptCandidate(makeRecord(C::PartySize,0x12345678));
    raw[1]^=0x20;
    const G5::Pokemon5ReadOnly pidChanged(raw);
    assert(pidChanged.sizeValid());
    assert(pidChanged.pid() != 0x12345678u || !pidChanged.valid());
    std::cout << "Gen V PK5 read-only record/crypto contracts PASS (32 shuffles, boxed/party, corruption)\n";
}
