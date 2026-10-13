#include "Encryption/Encryption5.h"
#include "Pokemon/Pokemon5ReadOnly.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"

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
    d[0x82]=0x21; // Existing Pokemon: strain 2, 1 contagious day.
    d[0x83]=4;    // Poke Ball.
    d[0x84]=0x80 | 45; // Met level, female original trainer.
    if (size == C::PartySize) {
        for (size_t i=0x88;i<size;i++) d[i]=static_cast<uint8_t>(i*7+1);
        d[0x8C]=50; // fill first; then restore the source-backed level field
    }
    return d;
}
int main() {
    static_assert(C::StoredSize == 0x88);
    static_assert(G5::NativeMovePP5.size()==560);
    static_assert(G5::basePP5(0)==0);
    static_assert(G5::basePP5(1)==35);
    static_assert(G5::basePP5(559)==5);
    static_assert(G5::maxPP5(1,3)==56);
    static_assert(!G5::validNativeMove5(560));
    static_assert(G5::NativeMetLocations5.size()==276);
    static_assert(G5::isNativeMetLocation5(30001));
    static_assert(G5::isNativeMetLocation5(40001));
    static_assert(G5::isNativeMetLocation5(60003));
    static_assert(!G5::isNativeMetLocation5(3));
    static_assert(!G5::isNativeMetLocation5(138));
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
            assert(entity.partyLevel() == (size == C::PartySize ? 50 : 0));
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
            assert(entity.pokerusState()==0x21 && entity.pokerusMode()==2);
            assert(entity.ball()==4 && entity.metLevel()==45);
            assert(entity.originalTrainerGender()==1);
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
    // App-owned entity mutation is atomic and never touches external saves.
    for (const size_t size : {C::StoredSize, C::PartySize}) {
        const auto source=C::encryptCandidate(makeRecord(size,0x13579BDF));
        std::string error;
        auto staged=G5::StagedPokemon5Record::create(source,&error);
        assert(staged && !staged->dirty());
        assert(staged->stage(G5::StagedPokemon5Record::Field::Nature,0,24,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::Friendship,0,255,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::IV,0,0,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::IV,5,31,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::EV,0,255,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::EV,1,255,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::HeldItem,0,638,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::Ball,0,25,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::MetLevel,0,100,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::TrainerId,0,65535,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::SecretId,0,0,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::OriginalTrainerGender,0,0,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::FatefulEncounter,0,1,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::MetLocation,0,30001,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::Pokerus,0,1,&error));
        assert(staged->current().pokerusState()==0x20);
        // Undo infection toggle recovers original strain AND native duration.
        assert(staged->stage(G5::StagedPokemon5Record::Field::Pokerus,0,2,&error));
        assert(staged->current().pokerusState()==0x21);
        assert(staged->stage(G5::StagedPokemon5Record::Field::Pokerus,0,1,&error));
        assert(staged->stage(G5::StagedPokemon5Record::Field::Move,0,559,&error));
        assert(staged->current().moves()[0]==559);
        assert(staged->current().pp()[0]==5);
        assert(staged->current().ppUps()[0]==0);
        assert(staged->stage(G5::StagedPokemon5Record::Field::MovePPUps,0,3,&error));
        assert(staged->current().pp()[0]==8);
        assert(staged->current().ppUps()[0]==3);
        assert(staged->stage(G5::StagedPokemon5Record::Field::MovePP,0,4,&error));
        assert(staged->dirty());
        const auto current=staged->current();
        assert(current.valid() && current.nature()==24 && current.friendship()==255);
        assert(current.ivs()[0]==0 && current.ivs()[5]==31);
        assert(current.evs()[0]==255 && current.evs()[1]==255);
        assert(current.heldItem()==638 && current.ball()==25);
        assert(current.metLevel()==100 && current.originalTrainerGender()==0);
        assert(current.tid()==65535 && current.sid()==0);
        assert(current.fatefulEncounter() && current.gender()==1 && current.form()==2);
        assert(current.metLocation()==30001);
        assert(current.pokerusState()==0x20 && current.pokerusMode()==1 &&
               current.hiddenAbility());
        assert(current.moves()[0]==559 && current.pp()[0]==4 &&
               current.ppUps()[0]==3);
        assert(current.moves()[1]==0 && current.pp()[1]==0);
        assert(current.moves()[2]==0 && current.moves()[3]==0);
        assert(std::equal(staged->originalBytes().begin(),staged->originalBytes().end(),source.begin()));
        const auto beforeInvalid=std::vector<uint8_t>(
            staged->stagedBytes().begin(),staged->stagedBytes().end());
        assert(!staged->stage(G5::StagedPokemon5Record::Field::Nature,0,25,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::Friendship,0,256,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::IV,6,12,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::IV,0,32,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::EV,2,1,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::EV,0,256,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::HeldItem,0,639,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::HeldItem,1,25,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::Ball,0,26,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MetLevel,0,101,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::TrainerId,0,65536,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::SecretId,1,1,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::OriginalTrainerGender,0,2,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::FatefulEncounter,0,2,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::Pokerus,0,3,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MetLocation,0,3,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MetLocation,0,138,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MetLocation,0,65535,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::Move,0,560,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::Move,4,85,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MovePPUps,0,4,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MovePP,0,9,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MovePP,4,1,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MovePPUps,1,1,&error));
        assert(!staged->stage(G5::StagedPokemon5Record::Field::MovePP,1,1,&error));
        assert(std::equal(staged->stagedBytes().begin(),staged->stagedBytes().end(),
                          beforeInvalid.begin()));
        staged->rollback();
        assert(!staged->dirty());
        assert(std::equal(staged->stagedBytes().begin(),staged->stagedBytes().end(),source.begin()));
    }
    const auto invalidEntity = std::vector<uint8_t>(C::StoredSize,0);
    assert(!G5::StagedPokemon5Record::create(invalidEntity).has_value());
    std::cout << "Gen V PK5 read-only record/crypto contracts PASS (32 shuffles, boxed/party, corruption)\n";
}
