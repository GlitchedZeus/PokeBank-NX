#ifndef POKEBANK_TEST_GEN9SV_SYNTHETIC_SAVE_H
#define POKEBANK_TEST_GEN9SV_SYNTHETIC_SAVE_H

// Synthetic Scarlet/Violet `main` images (SwishCrypto block list) built in code from the cited SAV9SV
// layout (include/Integration/Gen9/Gen9SvSession.h). Records reuse the PK8 builder: PK9 shares the
// size, encryption and every offset the shared editor writes. No real save is committed.

#include "Integration/Gen9/Gen9SvSession.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"

#include <cstdint>
#include <vector>

namespace Gen9SvFixture {
namespace L = PokeVault::Integration::Gen9Sv::Layout;
namespace PK8 = PokeVault::Integration::Gen8::PK8;
namespace Swish = PokeVault::Integration::Gen8::Swish;

struct Options {
    uint8_t game = L::VersionScarlet;
    uint8_t partyCount = 2;
    size_t fileSize = L::FixedSizes[0];
    bool includeTeams = true;
    // Real-save block shapes PKHeX also reads by raw bytes (owner hardware G69-HW-007): the team index
    // table as Array<Int16>, the lock flags as one Byte value instead of an Object.
    bool teamIndexesInt16Array = false;
    bool teamLocksByteValue = false;
    bool lockTeam0 = false;
};

inline std::vector<Swish::Block> makeBlocks(const Options& o = {}) {
    std::vector<Swish::Block> blocks;
    const auto object = [&](uint32_t key, std::vector<uint8_t> data) {
        blocks.push_back({key, Swish::TypeCode::Object, Swish::TypeCode::None, std::move(data)});
    };
    std::vector<uint8_t> status(0x80, 0);
    PK8::write16(status, L::MyTid, 11223);
    PK8::write16(status, L::MySid, 44556);
    status[L::MyGame] = o.game;
    status[L::MyGender] = 1;
    status[L::MyLanguage] = 2;
    const char16_t* name = u"Juliana";
    for (size_t i = 0; name[i]; ++i) PK8::write16(status, L::MyName + 2 * i, name[i]);
    object(L::KeyMyStatus, status);
    std::vector<uint8_t> party(L::PartyCountOffset + 4, 0);
    for (size_t i = 0; i < 6; ++i)
        Gen8Fixture::put(party, i * PK8::PartySize, i < o.partyCount ? Gen8Fixture::encryptedMon(static_cast<uint16_t>(906 + i), 300 + i)
                                                                    : Gen8Fixture::blankMon());
    party[L::PartyCountOffset] = o.partyCount;
    object(L::KeyParty, party);
    std::vector<uint8_t> box(L::BoxCount * L::BoxSlots * PK8::PartySize, 0);
    const auto blank = Gen8Fixture::blankMon();
    for (size_t i = 0; i < L::BoxCount * L::BoxSlots; ++i) Gen8Fixture::put(box, i * PK8::PartySize, blank);
    Gen8Fixture::put(box, 0, Gen8Fixture::encryptedMon(921, 21));                                   // box 1 slot 1
    Gen8Fixture::put(box, 1 * PK8::PartySize, Gen8Fixture::encryptedMon(194, 22));                 // box 1 slot 2 (team)
    Gen8Fixture::put(box, (31 * L::BoxSlots + 29) * PK8::PartySize, Gen8Fixture::encryptedMon(1007, 23));  // box 32 slot 30
    object(L::KeyBox, box);
    if (o.includeTeams) {
        std::vector<uint8_t> idx(72, 0xFF);
        PK8::write16(idx, 0, static_cast<uint16_t>((0 << 8) | 1));
        if (o.teamIndexesInt16Array) blocks.push_back({L::KeyTeamIndexes, Swish::TypeCode::Array, Swish::TypeCode::Int16, idx});
        else object(L::KeyTeamIndexes, idx);
        if (o.teamLocksByteValue) blocks.push_back({L::KeyTeamLocks, Swish::TypeCode::Byte, Swish::TypeCode::None, {static_cast<uint8_t>(o.lockTeam0 ? 1 : 0)}});
        else object(L::KeyTeamLocks, {static_cast<uint8_t>(o.lockTeam0 ? 1 : 0), 0, 0, 0});
    }
    size_t used = Swish::HashSize;
    for (const auto& b : blocks) used += b.serializedSize();
    std::vector<uint8_t> pad(o.fileSize - used - 9);
    for (size_t i = 0; i < pad.size(); ++i) pad[i] = static_cast<uint8_t>(i * 5);
    object(0x7E57F113u, pad);
    return blocks;
}
inline std::vector<uint8_t> makeSave(const Options& o = {}) { return Swish::encrypt(makeBlocks(o)); }

} // namespace Gen9SvFixture

#endif
