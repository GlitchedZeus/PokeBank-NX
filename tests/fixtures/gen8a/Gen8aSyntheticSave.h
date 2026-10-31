#ifndef POKEBANK_TEST_GEN8A_SYNTHETIC_SAVE_H
#define POKEBANK_TEST_GEN8A_SYNTHETIC_SAVE_H

// Synthetic Legends: Arceus `main` images (SwishCrypto block list) built in code from the cited
// SAV8LA layout (include/Integration/Gen8a/Gen8aSession.h). No real save is committed.

#include "Integration/Gen8a/Gen8aSession.h"

#include <cstdint>
#include <vector>

namespace Gen8aFixture {
namespace L = PokeVault::Integration::Gen8a::Layout;
namespace PA8 = PokeVault::Integration::Gen8a::PA8;
namespace Swish = PokeVault::Integration::Gen8::Swish;

inline std::vector<uint8_t> plainMon(uint16_t species, uint32_t seed, bool party, bool alpha = false) {
    std::vector<uint8_t> p(party ? PA8::PartySize : PA8::StoredSize, 0);
    PA8::write32(p, 0x00, 0x2545F491u * seed + 0x11u);
    PA8::write16(p, 0x08, species);
    PA8::write16(p, 0x0C, static_cast<uint16_t>(3000 + seed));
    PA8::write16(p, 0x0E, static_cast<uint16_t>(4000 + seed));
    PA8::write32(p, 0x10, 500 + seed);
    PA8::write16(p, 0x14, 34);
    p[0x16] = static_cast<uint8_t>(1 | (alpha ? 0x20 : 0));
    PA8::write32(p, 0x1C, 0x13572468u ^ (seed * 0x01010101u));
    p[0x20] = p[0x21] = static_cast<uint8_t>(seed % 25);
    PA8::write16(p, 0x3E, alpha ? 33 : 0);
    p[0x50] = 120; p[0x51] = 80;
    PA8::write16(p, 0x54, 33);
    const char16_t* nick = u"HISUI";
    for (size_t i = 0; nick[i]; ++i) PA8::write16(p, PA8::NicknameOffset + 2 * i, nick[i]);
    PA8::write16(p, 0x92, 18);
    PA8::write32(p, 0x94, 0x0FFFFFFFu & (seed * 0x01234567u));
    for (size_t i = 0; i < 6; ++i) p[0xA4 + i] = static_cast<uint8_t>(i % 4);
    p[0xEE] = L::VersionPLA;
    p[0xF2] = 2;
    const char16_t* ot = u"Rei";
    for (size_t i = 0; ot[i]; ++i) PA8::write16(p, PA8::OriginalTrainerOffset + 2 * i, ot[i]);
    p[0x12A] = 70;
    p[0x137] = 28;   // Hisuian Poke Ball
    p[0x13D] = 6;
    if (party) {
        p[0x168] = 6;
        for (size_t i = 0; i < 6; ++i) PA8::write16(p, 0x16A + 2 * i, static_cast<uint16_t>(18 + i));
    }
    PA8::write16(p, 0x06, PA8::checksum(p));
    return p;
}
inline std::vector<uint8_t> enc(std::vector<uint8_t> plain) { return PA8::encrypt(plain); }
inline void put(std::vector<uint8_t>& dst, size_t at, const std::vector<uint8_t>& src) {
    std::copy(src.begin(), src.end(), dst.begin() + static_cast<std::ptrdiff_t>(at));
}

struct Options {
    uint8_t game = L::VersionPLA;
    uint8_t partyCount = 2;
    size_t fileSize = L::Sizes[0];
};

inline std::vector<Swish::Block> makeBlocks(const Options& o = {}) {
    std::vector<Swish::Block> blocks;
    const auto object = [&](uint32_t key, std::vector<uint8_t> data) {
        blocks.push_back({key, Swish::TypeCode::Object, Swish::TypeCode::None, std::move(data)});
    };
    std::vector<uint8_t> status(0x70, 0);
    PA8::write16(status, L::MyTid, 22222);
    PA8::write16(status, L::MySid, 33333);
    status[L::MyGame] = o.game;
    status[L::MyGender] = 1;
    status[L::MyLanguage] = 2;
    const char16_t* name = u"Akari";
    for (size_t i = 0; name[i]; ++i) PA8::write16(status, L::MyName + 2 * i, name[i]);
    object(L::KeyMyStatus, status);
    std::vector<uint8_t> party(L::PartyCountOffset + 4, 0);
    const auto blankParty = enc(std::vector<uint8_t>(PA8::PartySize, 0));
    for (size_t i = 0; i < 6; ++i)
        put(party, i * PA8::PartySize, i < o.partyCount ? enc(plainMon(static_cast<uint16_t>(722 + i), 50 + i, true)) : blankParty);
    party[L::PartyCountOffset] = o.partyCount;
    object(L::KeyParty, party);
    std::vector<uint8_t> box(L::BoxCount * L::BoxSlots * PA8::StoredSize, 0);
    const auto blank = enc(std::vector<uint8_t>(PA8::StoredSize, 0));
    for (size_t i = 0; i < L::BoxCount * L::BoxSlots; ++i) put(box, i * PA8::StoredSize, blank);
    put(box, 0, enc(plainMon(399, 1, false, true)));                                       // box 1 slot 1 (alpha)
    put(box, 1 * PA8::StoredSize, enc(plainMon(403, 2, false)));                           // box 1 slot 2
    put(box, (31 * L::BoxSlots + 29) * PA8::StoredSize, enc(plainMon(493, 3, false)));     // box 32 slot 30
    object(L::KeyBox, box);
    blocks.push_back({0x0C0FFEEu, Swish::TypeCode::Array, Swish::TypeCode::UInt32, {1, 0, 0, 0, 2, 0, 0, 0}});
    size_t used = Swish::HashSize;
    for (const auto& b : blocks) used += b.serializedSize();
    std::vector<uint8_t> pad(o.fileSize - used - 9);
    for (size_t i = 0; i < pad.size(); ++i) pad[i] = static_cast<uint8_t>(i * 13);
    object(0x7E57F112u, pad);
    return blocks;
}
inline std::vector<uint8_t> makeSave(const Options& o = {}) { return Swish::encrypt(makeBlocks(o)); }

} // namespace Gen8aFixture

#endif
