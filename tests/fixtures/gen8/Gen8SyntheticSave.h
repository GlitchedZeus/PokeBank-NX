#ifndef POKEBANK_TESTS_GEN8_SYNTHETIC_SAVE_H
#define POKEBANK_TESTS_GEN8_SYNTHETIC_SAVE_H

// Synthetic Sword/Shield `main` images built in code. No real save, ROM or key
// material is committed: every byte here is generated from the cited layout
// (include/Integration/Gen8/Gen8SaveLayout.h) and the SwishCrypto container.

#include "Integration/Gen8/Gen8PK8.h"
#include "Integration/Gen8/Gen8SaveLayout.h"
#include "Integration/Gen8/Gen8SwishCrypto.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace Gen8Fixture {
namespace L = PokeVault::Integration::Gen8::Layout;
namespace PK8 = PokeVault::Integration::Gen8::PK8;
namespace Swish = PokeVault::Integration::Gen8::Swish;

// A decrypted, checksum-valid party-format PK8 with deterministic contents.
inline std::vector<uint8_t> plainMon(uint16_t species, uint32_t seed) {
    std::vector<uint8_t> p(PK8::PartySize, 0);
    PK8::write32(p, 0x00, 0x9E3779B9u * seed + 0x1234567u);  // encryption constant
    PK8::write16(p, 0x08, species);
    PK8::write16(p, 0x0A, 0);
    PK8::write16(p, 0x0C, static_cast<uint16_t>(1000 + seed));
    PK8::write16(p, 0x0E, static_cast<uint16_t>(2000 + seed));
    PK8::write32(p, 0x10, 1000 + seed * 10);
    PK8::write16(p, 0x14, 65);
    PK8::write32(p, 0x1C, 0x2468ACE1u ^ (seed * 0x01010101u));
    p[0x20] = static_cast<uint8_t>(seed % 25);
    p[0x21] = p[0x20];
    for (size_t i = 0; i < 6; ++i) p[0x26 + i] = static_cast<uint8_t>((seed + i) * 3);
    const char16_t* name = u"SYNTH";
    for (size_t i = 0; name[i]; ++i) PK8::write16(p, PK8::NicknameOffset + 2 * i, name[i]);
    PK8::write16(p, 0x72, 33);
    PK8::write16(p, 0x8A, 20);
    PK8::write32(p, 0x8C, 0x0FFFFFFFu & (seed * 0x01234567u));
    p[0xDE] = 44;
    p[0xE2] = 2;
    const char16_t* ot = u"TESTER";
    for (size_t i = 0; ot[i]; ++i) PK8::write16(p, PK8::OriginalTrainerOffset + 2 * i, ot[i]);
    p[0x112] = 70;
    p[0x124] = 4;
    p[0x125] = 5;
    p[0x148] = 5;
    for (size_t i = 0; i < 6; ++i) PK8::write16(p, 0x14A + 2 * i, static_cast<uint16_t>(20 + i));
    PK8::write16(p, 0x06, PK8::checksum(p));
    return p;
}
inline std::vector<uint8_t> encryptedMon(uint16_t species, uint32_t seed) {
    return PK8::encrypt(plainMon(species, seed));
}
inline std::vector<uint8_t> blankMon() { return PK8::encrypt(std::vector<uint8_t>(PK8::PartySize, 0)); }

inline void put(std::vector<uint8_t>& dst, size_t at, const std::vector<uint8_t>& src) {
    for (size_t i = 0; i < src.size(); ++i) dst[at + i] = src[i];
}

struct Options {
    uint8_t version = L::VersionSword;
    uint8_t partyCount = 2;
    size_t fileSize = L::SwordShieldSizes[0];
    bool includeTeams = true;
    // Real-save block shapes PKHeX also reads by raw bytes (owner hardware G69-HW-007): the team index
    // table as Array<Int16>, the lock flags as one Byte value instead of an Object.
    bool teamIndexesInt16Array = false;
    bool teamLocksByteValue = false;
    bool lockTeam0 = false;
};

// The decrypted block list for a synthetic save; tests mutate it before encrypting.
inline std::vector<Swish::Block> makeBlocks(const Options& o = {}) {
    std::vector<Swish::Block> blocks;
    const auto object = [&](uint32_t key, std::vector<uint8_t> data) {
        blocks.push_back({key, Swish::TypeCode::Object, Swish::TypeCode::None, std::move(data)});
    };

    std::vector<uint8_t> status(0x110, 0);
    PK8::write16(status, L::MyStatusTid, 12345);
    PK8::write16(status, L::MyStatusSid, 54321);
    status[L::MyStatusGame] = o.version;
    status[L::MyStatusGender] = 1;
    status[L::MyStatusLanguage] = 2;
    const char16_t* name = u"Hop";
    for (size_t i = 0; name[i]; ++i) PK8::write16(status, L::MyStatusName + 2 * i, name[i]);
    object(L::KeyMyStatus, status);

    std::vector<uint8_t> party(L::PartyCountOffset + 8, 0);
    for (size_t i = 0; i < L::PartySlots; ++i)
        put(party, i * L::RecordSize, i < o.partyCount ? encryptedMon(static_cast<uint16_t>(810 + i), 100 + i)
                                                       : blankMon());
    party[L::PartyCountOffset] = o.partyCount;
    object(L::KeyParty, party);

    std::vector<uint8_t> box(L::BoxBlockMinimum, 0);
    const auto blank = blankMon();
    for (size_t b = 0; b < L::BoxCount; ++b)
        for (size_t s = 0; s < L::BoxSlots; ++s)
            put(box, (b * L::BoxSlots + s) * L::RecordSize, blank);
    put(box, 0, encryptedMon(25, 1));                       // box 1 slot 1
    put(box, 1 * L::RecordSize, encryptedMon(133, 2));      // box 1 slot 2
    put(box, (31 * L::BoxSlots + 29) * L::RecordSize, encryptedMon(888, 3));  // box 32 slot 30
    object(L::KeyBox, box);

    blocks.push_back({L::KeyCurrentBox, Swish::TypeCode::Byte, Swish::TypeCode::None, {0}});
    blocks.push_back({0x0BADF00Du, Swish::TypeCode::Bool2, Swish::TypeCode::None, {}});
    blocks.push_back({0x0C0FFEEu, Swish::TypeCode::Array, Swish::TypeCode::UInt32, {1, 0, 0, 0, 2, 0, 0, 0}});

    if (o.includeTeams) {
        std::vector<uint8_t> idx(L::TeamIndexesMinimum, 0xFF);  // -1 = none
        PK8::write16(idx, 0, static_cast<uint16_t>((0 << 8) | 1));  // team 1 holds box 1 slot 2
        if (o.teamIndexesInt16Array) blocks.push_back({L::KeyTeamIndexes, Swish::TypeCode::Array, Swish::TypeCode::Int16, idx});
        else object(L::KeyTeamIndexes, idx);
        if (o.teamLocksByteValue) blocks.push_back({L::KeyTeamLocks, Swish::TypeCode::Byte, Swish::TypeCode::None, {static_cast<uint8_t>(o.lockTeam0 ? 1 : 0)}});
        else object(L::KeyTeamLocks, {static_cast<uint8_t>(o.lockTeam0 ? 1 : 0), 0, 0, 0});
    }

    // Filler Object so the encrypted file has an accepted SWSH size.
    size_t used = Swish::HashSize;
    for (const auto& b : blocks) used += b.serializedSize();
    const size_t filler = o.fileSize - used - 9;
    std::vector<uint8_t> pad(filler);
    for (size_t i = 0; i < pad.size(); ++i) pad[i] = static_cast<uint8_t>(i * 7);
    object(0x7E57F111u, pad);
    return blocks;
}

inline std::vector<uint8_t> makeSave(const Options& o = {}) { return Swish::encrypt(makeBlocks(o)); }

inline Swish::Block* findBlock(std::vector<Swish::Block>& blocks, uint32_t key) {
    for (auto& b : blocks) if (b.key == key) return &b;
    return nullptr;
}

} // namespace Gen8Fixture

#endif
