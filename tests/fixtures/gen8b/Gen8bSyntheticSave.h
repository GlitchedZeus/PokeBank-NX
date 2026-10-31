#ifndef POKEBANK_TEST_GEN8B_SYNTHETIC_SAVE_H
#define POKEBANK_TEST_GEN8B_SYNTHETIC_SAVE_H

// Synthetic BD/SP SaveData images built in code from the cited SAV8BS layout
// (include/Integration/Gen8b/Gen8bSave.h). No real save is committed. PB8 records reuse the PK8
// builder (tests/fixtures/gen8) because both are the G8PKM layout.

#include "Integration/Gen8b/Gen8bSave.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"

#include <cstdint>
#include <vector>

namespace Gen8bFixture {
namespace L = PokeVault::Integration::Gen8b::Layout;
namespace PK8 = PokeVault::Integration::Gen8::PK8;

struct Options {
    uint8_t romCode = 0;          // 0 = Brilliant Diamond, 1 = Shining Pearl
    size_t revision = 0;          // index into Layout::Revisions
    uint8_t partyCount = 2;
    bool teamReferencesBox1Slot2 = true;
    bool lockTeam0 = false;
    bool invalidTeam = false;     // a team entry pointing outside storage
};

inline std::vector<uint8_t> makeSave(const Options& o = {}) {
    const auto& rev = L::Revisions[o.revision];
    std::vector<uint8_t> s(rev.size, 0);
    PK8::write32(s, 0, rev.version);
    const char16_t* name = u"Lucas";
    for (size_t i = 0; name[i]; ++i) PK8::write16(s, L::MyStatus + 2 * i, name[i]);
    PK8::write16(s, L::Tid, 31337);
    PK8::write16(s, L::Sid, 4242);
    s[L::Male] = 1;
    s[L::RomCode] = o.romCode;
    for (size_t i = 0; i < L::PartySlots; ++i)
        Gen8Fixture::put(s, L::partyOffset(i), i < o.partyCount ? Gen8Fixture::encryptedMon(static_cast<uint16_t>(387 + i), 200 + i)
                                                                 : Gen8Fixture::blankMon());
    s[L::PartyCount] = o.partyCount;
    const auto blank = Gen8Fixture::blankMon();
    for (size_t b = 0; b < L::BoxCount; ++b)
        for (size_t k = 0; k < L::BoxSlots; ++k) Gen8Fixture::put(s, L::boxOffset(b, k), blank);
    Gen8Fixture::put(s, L::boxOffset(0, 0), Gen8Fixture::encryptedMon(393, 11));  // box 1 slot 1
    Gen8Fixture::put(s, L::boxOffset(0, 1), Gen8Fixture::encryptedMon(396, 12));  // box 1 slot 2 (team)
    Gen8Fixture::put(s, L::boxOffset(39, 29), Gen8Fixture::encryptedMon(483, 13)); // box 40 slot 30
    const char16_t* box1 = u"Underground";
    for (size_t i = 0; box1[i] && i < 16; ++i) PK8::write16(s, L::BoxLayout + 2 * i, box1[i]);
    for (size_t i = 0; i < L::TeamCount * L::TeamSlotCount; ++i) PK8::write16(s, L::TeamPositions + 2 * i, 0xFFFF);
    if (o.teamReferencesBox1Slot2) PK8::write16(s, L::TeamPositions, (0 << 8) | 1);
    if (o.invalidTeam) PK8::write16(s, L::TeamPositions + 2, (41 << 8) | 1);
    s[L::LockedTeam] = o.lockTeam0 ? 1 : 0;
    s[0x6000] = 0x77;  // a recognisable byte in an unrelated region
    PokeVault::Integration::Gen8b::Gen8bSave::refreshHash(s);
    return s;
}

} // namespace Gen8bFixture

#endif
