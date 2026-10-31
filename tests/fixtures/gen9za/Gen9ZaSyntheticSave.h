#ifndef POKEBANK_TEST_GEN9ZA_SYNTHETIC_SAVE_H
#define POKEBANK_TEST_GEN9ZA_SYNTHETIC_SAVE_H

// Synthetic Legends: Z-A `main` images built in code from the cited SAV9ZA layout
// (include/Integration/Gen9/Gen9ZaSession.h): 0x198 box stride, 0x1E0 party stride, optional DLC
// revision block with at-rest "present" flags. Records reuse the PK8 builder. No real save committed.

#include "Integration/Gen9/Gen9ZaSession.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"

#include <cstdint>
#include <vector>

namespace Gen9ZaFixture {
namespace L = PokeVault::Integration::Gen9Za::Layout;
namespace PK8 = PokeVault::Integration::Gen8::PK8;
namespace Swish = PokeVault::Integration::Gen8::Swish;

struct Options {
    uint8_t game = L::VersionZA;
    uint8_t partyCount = 2;
    bool dlc = false;          // revision 1 + present flags (size 0x309FA6), else revision 0 (0x2F3284)
    bool includeTeams = true;
};

inline std::vector<Swish::Block> makeBlocks(const Options& o = {}) {
    std::vector<Swish::Block> blocks;
    const auto object = [&](uint32_t key, std::vector<uint8_t> data) {
        blocks.push_back({key, Swish::TypeCode::Object, Swish::TypeCode::None, std::move(data)});
    };
    std::vector<uint8_t> status(0x80, 0);
    PK8::write16(status, L::MyTid, 52000);
    PK8::write16(status, L::MySid, 1234);
    status[L::MyGame] = o.game;
    status[L::MyGender] = 0;
    const char16_t* name = u"Taunie";
    for (size_t i = 0; name[i]; ++i) PK8::write16(status, L::MyName + 2 * i, name[i]);
    object(L::KeyMyStatus, status);
    std::vector<uint8_t> party(L::PartySlots * L::PartyStride, 0);
    for (size_t i = 0; i < L::PartySlots; ++i) {
        Gen8Fixture::put(party, i * L::PartyStride, i < o.partyCount ? Gen8Fixture::encryptedMon(static_cast<uint16_t>(650 + i), 400 + i)
                                                                    : Gen8Fixture::blankMon());
        party[i * L::PartyStride + 0x1A0] = 1;  // Party9a second footer
        if (o.dlc) party[i * L::PartyStride + L::PresentFlag] = 1;
    }
    object(L::KeyParty, party);
    std::vector<uint8_t> box(L::BoxCount * L::BoxSlots * L::BoxStride, 0);
    const auto blank = Gen8Fixture::blankMon();
    for (size_t i = 0; i < L::BoxCount * L::BoxSlots; ++i) {
        Gen8Fixture::put(box, i * L::BoxStride, blank);
        box[i * L::BoxStride + L::PresentFlag + 1] = static_cast<uint8_t>(i);  // gap bytes that must survive
        if (o.dlc) box[i * L::BoxStride + L::PresentFlag] = 1;
    }
    Gen8Fixture::put(box, 0, Gen8Fixture::encryptedMon(25, 31));                                       // box 1 slot 1
    Gen8Fixture::put(box, 1 * L::BoxStride, Gen8Fixture::encryptedMon(380, 32));                      // box 1 slot 2 (team)
    Gen8Fixture::put(box, (31 * L::BoxSlots + 29) * L::BoxStride, Gen8Fixture::encryptedMon(718, 33)); // box 32 slot 30
    object(L::KeyBox, box);
    if (o.includeTeams) {
        std::vector<uint8_t> idx(72, 0xFF);
        PK8::write16(idx, 0, static_cast<uint16_t>((0 << 8) | 1));
        object(L::KeyTeamIndexes, idx);
        object(L::KeyTeamLocks, {0, 0, 0, 0});
    }
    if (o.dlc) blocks.push_back({L::KeySaveRevision, Swish::TypeCode::UInt64, Swish::TypeCode::None, {1, 0, 0, 0, 0, 0, 0, 0}});
    size_t used = Swish::HashSize;
    for (const auto& b : blocks) used += b.serializedSize();
    std::vector<uint8_t> pad((o.dlc ? L::Sizes[2] : L::Sizes[0]) - used - 9);
    for (size_t i = 0; i < pad.size(); ++i) pad[i] = static_cast<uint8_t>(i * 3);
    object(0x7E57F114u, pad);
    return blocks;
}
inline std::vector<uint8_t> makeSave(const Options& o = {}) { return Swish::encrypt(makeBlocks(o)); }

} // namespace Gen9ZaFixture

#endif
