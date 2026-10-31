#ifndef POKEBANK_TEST_GEN7B_SYNTHETIC_SAVE_H
#define POKEBANK_TEST_GEN7B_SYNTHETIC_SAVE_H

// Synthetic Let's Go `main` images built in code from the cited SAV7b layout
// (include/Integration/Gen7b/Gen7bSave.h). No real save is committed. PB7 records reuse the
// PK6/PK7 builder because PB7 shares its encryption and every offset this lane touches.

#include "Integration/Gen7b/Gen7bSave.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"

#include <array>
#include <cstdint>
#include <vector>

namespace Gen7bFixture {
namespace L = PokeVault::Integration::Gen7b::Layout;
namespace PK6 = PokeVault::Integration::Gen6::PK6;
namespace V = Save::LGPEReadValidation;

struct Options {
    uint8_t version = L::VersionPikachu;
    uint16_t count = 6;                                   // stored Pokemon (gapless)
    std::array<uint16_t, 6> party{0, 1, 1001, 1001, 1001, 1001};
    uint16_t starter = 0;                                 // partner = storage index 0 (also in party)
    bool fullFile = false;                                // 0x100000 instead of 0xB8800
};

inline Gen6Fixture::MonSpec spec(size_t i, uint8_t version) {
    Gen6Fixture::MonSpec m;
    m.species = static_cast<uint16_t>(i == 0 ? 25 : 1 + (i * 7) % 151);
    m.ec = 0x51000000u + static_cast<uint32_t>(i) * 0x01020304u;
    m.pid = 0x0A000000u + static_cast<uint32_t>(i) * 0x00FF00FFu;
    m.version = version;
    return m;
}

inline void refreshCrcs(std::vector<uint8_t>& save) {
    for (const auto& b : V::CONSUMED_BLOCKS)
        PK6::write16(save, V::CHECKSUM_BASE + b.key * 8, V::crc16Arc(std::span<const uint8_t>(save).subspan(b.offset, b.size)));
}

inline std::vector<uint8_t> makeSave(const Options& o = {}) {
    std::vector<uint8_t> save(o.fullFile ? L::FullSize : L::ActiveSize, 0);
    PK6::write16(save, L::Tid, 24680);
    PK6::write16(save, L::Sid, 13579);
    save[L::Version] = o.version;
    save[L::Gender] = 0;
    save[L::Language] = 2;
    const char16_t* name = u"Red";
    for (size_t i = 0; name[i]; ++i) PK6::write16(save, L::OtName + 2 * i, name[i]);
    for (size_t i = 0; i < L::Slots; ++i) {
        const auto rec = i < o.count ? Gen6Fixture::encryptedMon(spec(i, o.version), true)
                                     : PK6::encrypt(std::vector<uint8_t>(L::RecordSize, 0));
        Gen6Fixture::put(save, L::recordOffset(i), rec);
    }
    for (size_t i = 0; i < 6; ++i) PK6::write16(save, L::Header + 2 * i, o.party[i]);
    PK6::write16(save, L::Header + L::StarterPointer, o.starter);
    PK6::write16(save, L::Header + L::CountField, o.count);
    // A recognisable byte in an unrelated block (MyItem) that must never change.
    save[0x10] = 0x5A;
    refreshCrcs(save);
    return save;
}

} // namespace Gen7bFixture

#endif
