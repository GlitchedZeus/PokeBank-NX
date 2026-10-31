#ifndef POKEBANK_GEN5_NATIVE_CREATE_H
#define POKEBANK_GEN5_NATIVE_CREATE_H

#include "Integration/Gen5/Gen5CreateValidation.h"
#include "Integration/Gen5/Gen5NativeText.h"
#include "Pokemon/Pokemon5ReadOnly.h"

namespace PokeVault::Integration::Gen5 {

struct CreateResult5 {
    CreateValidation5 validation;
    // App-owned encrypted boxed PK5. Empty on every failure. No save handles,
    // paths, source bytes, staging authority or legality conclusion are accepted.
    std::vector<uint8_t> encrypted;
    explicit operator bool() const noexcept {
        return static_cast<bool>(validation) && encrypted.size() == Crypto::StoredSize;
    }
};

// Native PK5 layout checked against PKHeX PK5.cs at tree
// 66ef5983a5d349efa2dd615f2926a1b1ee97306e. This is structural creation,
// NOT encounter generation or proof of obtainability. Caller identity/history
// are encoded literally. All unrequested flags/ribbons/history remain zero.
[[nodiscard]] inline CreateResult5 createNative5(const CreateRequest5& r) {
    const auto validation = validateCreate5(r);
    if (!validation) return {validation, {}};
    const auto p = personal5(r.species, r.form, static_cast<uint32_t>(r.game) >= 22);
    std::vector<uint8_t> plain(Crypto::StoredSize, 0);
    const auto put32 = [&](size_t pos, uint32_t value) {
        Crypto::write16(plain, pos, static_cast<uint16_t>(value));
        Crypto::write16(plain, pos + 2, static_cast<uint16_t>(value >> 16));
    };
    put32(0, *r.pid);
    Crypto::write16(plain, 0x08, static_cast<uint16_t>(r.species));
    Crypto::write16(plain, 0x0A, static_cast<uint16_t>(r.heldItem));
    Crypto::write16(plain, 0x0C, static_cast<uint16_t>(r.trainer->tid));
    Crypto::write16(plain, 0x0E, static_cast<uint16_t>(r.trainer->sid));
    put32(0x10, createExperience5(p.info->growthRate, r.level));
    plain[0x14] = static_cast<uint8_t>(r.friendship);
    plain[0x15] = abilityForSlot5(*p.info, r.abilitySlot);
    plain[0x17] = static_cast<uint8_t>(r.trainer->language);
    uint32_t ivWord = 0;
    for (size_t i = 0; i < 6; ++i) {
        plain[0x18 + i] = static_cast<uint8_t>(r.evs[i]);
        ivWord |= r.ivs[i] << (5 * i);
    }
    put32(0x38, ivWord); // Native name encoder sets only the nickname flag.
    for (size_t i = 0; i < 4; ++i) {
        const auto& m = r.moves[i];
        Crypto::write16(plain, 0x28 + 2 * i, static_cast<uint16_t>(m.id));
        plain[0x30 + i] = static_cast<uint8_t>(m.pp.value_or(maxPP5(m.id, m.ppUps)));
        plain[0x34 + i] = static_cast<uint8_t>(m.ppUps);
    }
    plain[0x40] = static_cast<uint8_t>((r.form << 3) | (r.gender << 1));
    plain[0x41] = static_cast<uint8_t>(r.nature); // Gen V nature is NOT PID % 25.
    plain[0x42] = r.abilitySlot == 2 ? 1 : 0;
    if (!encodeAsciiName5(plain, true, r.nickname) ||
        !encodeAsciiName5(plain, false, r.trainer->name))
        return {{CreateError5::Verification, "Validated native text failed encoding"}, {}};
    plain[0x5F] = static_cast<uint8_t>(r.game);
    plain[0x7B] = static_cast<uint8_t>(r.encounter.year - 2000);
    plain[0x7C] = static_cast<uint8_t>(r.encounter.month);
    plain[0x7D] = static_cast<uint8_t>(r.encounter.day);
    Crypto::write16(plain, 0x80, static_cast<uint16_t>(r.encounter.location));
    plain[0x83] = static_cast<uint8_t>(r.encounter.ball);
    plain[0x84] = static_cast<uint8_t>(r.encounter.level | (r.trainer->gender << 7));
    Crypto::write16(plain, 6, Crypto::checksum(plain));
    auto encrypted = Crypto::encryptCandidate(plain);
    const Pokemon5ReadOnly check(encrypted);
    if (!check.valid() || check.empty() || check.partyRecord() ||
        check.species() != r.species || check.pid() != *r.pid ||
        Crypto::decrypt(encrypted) != plain ||
        !std::equal(plain.begin(), plain.end(), check.decodedBytes().begin(), check.decodedBytes().end()))
        return {{CreateError5::Verification, "Created PK5 failed checksum, reparse or exact byte verification"}, {}};
    return {{}, std::move(encrypted)};
}
} // namespace PokeVault::Integration::Gen5
#endif
