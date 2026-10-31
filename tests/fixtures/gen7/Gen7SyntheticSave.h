#ifndef POKEBANK_TEST_GEN7_SYNTHETIC_SAVE_H
#define POKEBANK_TEST_GEN7_SYNTHETIC_SAVE_H

// Synthetic SAV7 builder: generated from the cited layout only, then signed
// with the vendored memecrypto exactly as PKHeX MemeCrypto.SignInPlace does
// (SHA256 of the checksum table, zero tail). Safe to commit: no real data.
#include "Integration/Gen7/Gen7Save.h"
#include "Integration/Gen7/MiniGmpAllocationScope.h"
#include "Utils/CRC16.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"

extern "C" {
#include "memecrypto.h"
}

#include <array>
#include <vector>

namespace Gen7Fixture {
namespace Layout = PokeVault::Integration::Gen7::Layout;
using PokeVault::Integration::Gen7::Gen7Save;
namespace PK7 = PokeVault::Integration::Gen7::PK7;

// Recompute every block CRC16Invert with the signature region zeroed.
inline void resignBlocks(std::vector<uint8_t>& save, Layout::Family f) {
    for (size_t id = 0; id < Layout::blocks(f).size(); ++id)
        PK7::write16(save, Layout::checksumOffset(f, id), Gen7Save::computeBlockChecksum(save, f, id));
}

// PKHeX-style signature: payload = SHA256(table) || zeros, RSA-signed in place.
inline void signMeme(std::vector<uint8_t>& save, Layout::Family f) {
    std::array<uint8_t, Layout::SignatureLength> payload{};
    const auto hash = Gen7Save::checksumTableHash(save, f);
    std::copy(hash.begin(), hash.end(), payload.begin());
    std::array<uint8_t, Layout::SignatureLength> signedBytes{};
    {
        const PokeVault::Integration::Gen7::MiniGmpAllocationScope scope;  // rsa_decrypt leaks its mpz temporaries
        memecrypto_sign(payload.data(), signedBytes.data(), static_cast<int>(payload.size()));
    }
    std::copy(signedBytes.begin(), signedBytes.end(),
              save.begin() + static_cast<std::ptrdiff_t>(Layout::signatureOffset(f)));
}

struct SaveSpec {
    Layout::Family family = Layout::Family::SunMoon;
    uint8_t version = Layout::VersionSun;
    size_t partyCount = 2;
    bool resign = true;
    bool sign = true;
};

inline std::vector<uint8_t> makeSave(const SaveSpec& spec = {}) {
    const auto f = spec.family;
    std::vector<uint8_t> save(Layout::saveSize(f), 0);
    for (size_t i = 0; i < save.size(); ++i) save[i] = static_cast<uint8_t>((i * 13 + 5) & 0xFF);
    const size_t meta = Layout::metadataOffset(f);
    std::fill(save.begin() + static_cast<std::ptrdiff_t>(meta), save.end(), uint8_t{0});
    PK7::write32(save, meta + Layout::MagicWithinMetadata, Layout::BeefMagic);
    const auto table = Layout::blocks(f);
    for (size_t id = 0; id < table.size(); ++id) {
        const size_t e = meta + Layout::EntriesWithinMetadata + id * Layout::EntrySize;
        PK7::write32(save, e, table[id].length);
        PK7::write16(save, e + 4, static_cast<uint16_t>(id));
    }
    const size_t ms = table[Layout::MyStatusBlock].offset;
    PK7::write16(save, ms + 0, 1111);
    PK7::write16(save, ms + 2, 2222);
    save[ms + 4] = spec.version;
    save[ms + 5] = 0;
    save[ms + 0x35] = 2;
    const std::u16string name = u"ALOLA";
    for (size_t i = 0; i < 13; ++i) PK7::write16(save, ms + 0x38 + 2 * i, i < name.size() ? name[i] : 0);
    const size_t play = table[Layout::PlayTimeBlock].offset;
    PK7::write16(save, play, 300);
    save[play + 2] = 1;
    save[play + 3] = 2;

    const auto blankParty = PK7::Pokemon6::blank(Layout::PartyRecordSize).serialize();
    for (size_t i = 0; i < Layout::PartySlots; ++i) {
        if (i < spec.partyCount) {
            Gen6Fixture::MonSpec m;
            m.species = static_cast<uint16_t>(722 + i);
            m.ec = 0x700u + static_cast<uint32_t>(i) * 0x3579BDF;
            m.pid = 0x7000u + static_cast<uint32_t>(i) * 0x2468ACE;
            m.version = spec.version;
            Gen6Fixture::put(save, Gen7Save::partyRecordOffset(f, i), Gen6Fixture::encryptedMon(m, true));
        } else {
            Gen6Fixture::put(save, Gen7Save::partyRecordOffset(f, i), blankParty);
        }
    }
    save[Gen7Save::partyCountOffset(f)] = static_cast<uint8_t>(spec.partyCount);
    const auto blankBox = PK7::Pokemon6::blank().serialize();
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s)
            Gen6Fixture::put(save, Gen7Save::boxRecordOffset(f, b, s), blankBox);
    Gen6Fixture::MonSpec boxed;
    boxed.species = 785;
    boxed.version = spec.version;
    Gen6Fixture::put(save, Gen7Save::boxRecordOffset(f, 31, 29), Gen6Fixture::encryptedMon(boxed, false));
    std::fill_n(save.begin() + static_cast<std::ptrdiff_t>(Layout::signatureOffset(f)), Layout::SignatureLength, uint8_t{0});
    if (spec.resign) resignBlocks(save, f);
    if (spec.sign) signMeme(save, f);
    return save;
}

} // namespace Gen7Fixture

#endif
