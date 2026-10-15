#ifndef POKEBANK_TEST_GEN6_SYNTHETIC_SAVE_H
#define POKEBANK_TEST_GEN6_SYNTHETIC_SAVE_H

// Synthetic SAV6/PK6 builders for host tests. Nothing here is derived from a
// real save: every byte is generated from the cited layout constants, so the
// fixtures can be committed safely. All records/blocks are internally valid.
#include "Integration/Gen6/Gen6PK6.h"
#include "Integration/Gen6/Gen6Save.h"
#include "Integration/Gen6/Gen6SaveLayout.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace Gen6Fixture {
namespace PK6 = PokeVault::Integration::Gen6::PK6;
namespace Layout = PokeVault::Integration::Gen6::Layout;
using PokeVault::Integration::Gen6::Gen6Save;

struct MonSpec {
    uint16_t species = 25;
    uint32_t ec = 0x12345678;
    uint32_t pid = 0x9ABCDEF0;
    uint16_t tid = 12345, sid = 54321;
    uint8_t version = Layout::VersionX;
    uint8_t form = 0;
    bool egg = false;
    uint8_t handler = 0;
    std::u16string nickname = u"SYNTH";
    std::u16string ot = u"TESTER";
};

// Decrypted record with a valid checksum (party records get plausible stats).
inline std::vector<uint8_t> plainMon(const MonSpec& s, bool party) {
    std::vector<uint8_t> p(party ? PK6::PartySize : PK6::StoredSize, 0);
    PK6::write32(p, 0x00, s.ec);
    PK6::write16(p, 0x08, s.species);
    PK6::write16(p, 0x0A, 1);                 // held item
    PK6::write16(p, 0x0C, s.tid);
    PK6::write16(p, 0x0E, s.sid);
    PK6::write32(p, 0x10, 1000);              // EXP
    p[0x14] = 9; p[0x15] = 1;
    PK6::write32(p, 0x18, s.pid);
    p[0x1C] = 3;                              // nature
    p[0x1D] = static_cast<uint8_t>(s.form << 3);
    p[0x1E] = 4; p[0x1F] = 8;                 // EVs
    for (size_t i = 0; i < s.nickname.size() && i < 12; ++i) PK6::write16(p, 0x40 + 2 * i, s.nickname[i]);
    PK6::write16(p, 0x5A, 33); p[0x62] = 35;  // one move + PP
    uint32_t iv = 0x1F | (0x10u << 5) | (0x0Fu << 10) | (0x1Fu << 15) | (0x02u << 20) | (0x1Eu << 25);
    if (s.egg) iv |= 0x40000000u;
    PK6::write32(p, 0x74, iv);
    p[0x93] = s.handler;
    for (size_t i = 0; i < s.ot.size() && i < 12; ++i) PK6::write16(p, 0xB0 + 2 * i, s.ot[i]);
    p[0xCA] = 70;                             // OT friendship
    p[0xD4] = 14; p[0xD5] = 1; p[0xD6] = 2;   // met date
    PK6::write16(p, 0xDA, 6);                 // met location
    p[0xDC] = 4;                              // ball
    p[0xDD] = 5;                              // met level
    p[0xDF] = s.version;
    p[0xE3] = 2;                              // language
    if (party) {
        p[0xEC] = 12;
        for (size_t i = 0; i < 7; ++i) PK6::write16(p, 0xF0 + 2 * i, static_cast<uint16_t>(20 + i));
    }
    PK6::write16(p, 0x06, PK6::checksum(p));
    return p;
}

inline std::vector<uint8_t> encryptedMon(const MonSpec& s, bool party) {
    return PK6::encrypt(plainMon(s, party));
}

inline void put(std::vector<uint8_t>& save, size_t offset, const std::vector<uint8_t>& bytes) {
    std::copy(bytes.begin(), bytes.end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
}

inline void resignAll(std::vector<uint8_t>& save, Layout::Family family) {
    for (size_t id = 0; id < Layout::blocks(family).size(); ++id)
        Gen6Save::refreshBlockChecksum(save, family, id);
}

struct SaveSpec {
    Layout::Family family = Layout::Family::XY;
    uint8_t version = Layout::VersionX;
    size_t partyCount = 2;
    bool resign = true;
};

// Party: `partyCount` occupied records; Box 1 slots 1-3 occupied (slot 3 a
// Furfrou in form 2); every other box slot holds the encrypted blank record.
inline std::vector<uint8_t> makeSave(const SaveSpec& spec = {}) {
    const auto family = spec.family;
    std::vector<uint8_t> save(Layout::saveSize(family), 0);
    // Fill non-block filler with a pattern so whitelist tests see real bytes.
    for (size_t i = 0; i < save.size(); ++i) save[i] = static_cast<uint8_t>((i * 7 + 3) & 0xFF);
    const size_t meta = Layout::metadataOffset(family);
    std::fill(save.begin() + static_cast<std::ptrdiff_t>(meta), save.end(), uint8_t{0});
    PK6::write32(save, meta + Layout::MagicWithinMetadata, Layout::BeefMagic);
    const auto table = Layout::blocks(family);
    for (size_t id = 0; id < table.size(); ++id) {
        const size_t e = meta + Layout::EntriesWithinMetadata + id * Layout::EntrySize;
        PK6::write32(save, e, table[id].length);
        PK6::write16(save, e + 4, static_cast<uint16_t>(id));
    }
    // MyStatus
    const size_t ms = Layout::MyStatusOffset;
    PK6::write16(save, ms + 0, 12345);
    PK6::write16(save, ms + 2, 54321);
    save[ms + 4] = spec.version;
    save[ms + 5] = 1;
    save[ms + 0x2D] = 2;
    const std::u16string name = u"TESTER";
    for (size_t i = 0; i < 13; ++i) PK6::write16(save, ms + 0x48 + 2 * i, i < name.size() ? name[i] : 0);
    PK6::write16(save, Layout::PlayTimeOffset, 42);
    save[Layout::PlayTimeOffset + 2] = 7;
    save[Layout::PlayTimeOffset + 3] = 9;

    // Party: occupied slots, then encrypted blank party records, then count.
    const auto blankParty = PK6::Pokemon6::blank(PK6::PartySize).serialize();
    for (size_t i = 0; i < Layout::PartySlots; ++i) {
        if (i < spec.partyCount) {
            MonSpec m;
            m.species = static_cast<uint16_t>(1 + i);
            m.ec = 0x1000u + static_cast<uint32_t>(i) * 0x2468ACE;
            m.pid = 0x8000u + static_cast<uint32_t>(i) * 0x13579BD;
            m.version = spec.version;
            put(save, Gen6Save::partyRecordOffset(i), encryptedMon(m, true));
        } else {
            put(save, Gen6Save::partyRecordOffset(i), blankParty);
        }
    }
    save[Gen6Save::partyCountOffset()] = static_cast<uint8_t>(spec.partyCount);
    // Bytes between the count and the end of block 18 stay as zero padding.
    for (size_t i = Gen6Save::partyCountOffset() + 1; i < Layout::PartyOffset + 0x61C; ++i) save[i] = 0;

    const auto blankBox = PK6::Pokemon6::blank().serialize();
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s)
            put(save, Gen6Save::boxRecordOffset(family, b, s), blankBox);
    for (size_t s = 0; s < 3; ++s) {
        MonSpec m;
        m.species = s == 2 ? 676 : static_cast<uint16_t>(100 + s);
        m.form = s == 2 ? 2 : 0;
        m.ec = 0xA0000000u + static_cast<uint32_t>(s) * 0x01010101u;
        m.pid = 0x0B000000u + static_cast<uint32_t>(s) * 0x00FF00FFu;
        m.version = spec.version;
        put(save, Gen6Save::boxRecordOffset(family, 0, s), encryptedMon(m, false));
    }
    if (spec.resign) resignAll(save, family);
    return save;
}

// A native MyItem block: zero-filled, with the given (pocket offset, item, count)
// slots written in order per pocket. Caller re-signs the save.
struct BagSlot { size_t pocketOffset; uint16_t item; uint16_t count; };
inline void writeBag(std::vector<uint8_t>& save, Layout::Family family, const std::vector<BagSlot>& slots) {
    const auto& block = Layout::blocks(family)[1];
    std::fill_n(save.begin() + block.offset, block.length, uint8_t{0});
    std::vector<size_t> used;
    for (const auto& s : slots) {
        size_t n = 0;
        for (const size_t u : used) if (u == s.pocketOffset) ++n;
        PK6::write16(save, block.offset + s.pocketOffset + n * 4, s.item);
        PK6::write16(save, block.offset + s.pocketOffset + n * 4 + 2, s.count);
        used.push_back(s.pocketOffset);
    }
}

} // namespace Gen6Fixture

#endif
