// Stage 2 audit-owned native-format properties.  Synthetic records exercise
// production Gen I-IV parsers/codecs; no save is opened, changed, or claimed
// to be authentic provenance.
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Pokemon/Pokemon4ReadOnly.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

namespace {
uint32_t next(uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return state;
}
void put16(std::vector<std::byte>& bytes, size_t offset, uint16_t value) {
    bytes[offset] = static_cast<std::byte>(value);
    bytes[offset + 1] = static_cast<std::byte>(value >> 8);
}
void put32(std::vector<std::byte>& bytes, size_t offset, uint32_t value) {
    put16(bytes, offset, static_cast<uint16_t>(value));
    put16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
std::vector<std::byte> pk4Plain(size_t size, uint32_t seed, uint16_t species = 1) {
    std::vector<std::byte> plain(size, std::byte{0});
    put32(plain, 0, seed | 1u);
    put16(plain, 8, species);
    put16(plain, 0x0c, static_cast<uint16_t>(seed));
    put16(plain, 0x0e, static_cast<uint16_t>(seed >> 16));
    put32(plain, 0x10, next(seed));
    plain[0x14] = static_cast<std::byte>(next(seed));
    plain[0x15] = static_cast<std::byte>(next(seed));
    plain[0x17] = std::byte{2};
    for (size_t offset = 0x18; offset != 0x1e; ++offset)
        plain[offset] = static_cast<std::byte>(next(seed));
    for (size_t slot = 0; slot != 4; ++slot) {
        put16(plain, 0x28 + slot * 2, static_cast<uint16_t>(1 + (next(seed) % 467)));
        plain[0x30 + slot] = static_cast<std::byte>(1 + (next(seed) % 63));
        plain[0x34 + slot] = static_cast<std::byte>(next(seed) & 3u);
    }
    put32(plain, 0x38, next(seed) & 0x3fffffffu);
    plain[0x5f] = std::byte{12}; // Platinum native origin byte.
    plain[0x84] = std::byte{50};
    plain[0x86] = std::byte{4};
    if (size == Encryption::SIZE_PARTY4) {
        put32(plain, 0x88, next(seed));
        plain[0x8c] = std::byte{50};
        plain[0x8d] = std::byte{3};
        put16(plain, 0x8e, 80);
        put16(plain, 0x90, 100);
        for (size_t offset = 0x92; offset != 0x9c; offset += 2)
            put16(plain, offset, static_cast<uint16_t>(20 + next(seed) % 200));
    }
    put16(plain, 6, Encryption::checksum4(plain));
    return plain;
}
void testGen12Boundaries(unsigned& checks) {
    using namespace PokeVault::Integration;
    for (size_t size = 0; size != 257; ++size) {
        std::vector<uint8_t> bytes(size, static_cast<uint8_t>(size));
        const auto original = bytes;
        const auto one = Gen1::parse(bytes, Gen1::SourceGame::Red);
        const auto two = Gen2::parse(bytes, Gen2::SourceGame::Gold);
        assert(!one && one.error == Gen1::SaveError::WrongSize);
        assert(!two && two.error == Gen2::SaveError::WrongSize);
        assert(bytes == original);
        checks += 4;
    }
    for (const auto size : {Gen1::kRawSaveSize, Gen2::kRawSaveSize32K, Gen2::kRawSaveSize64K}) {
        std::vector<uint8_t> bytes(size, 0);
        const auto original = bytes;
        if (size == Gen1::kRawSaveSize) {
            const auto parsed = Gen1::parse(bytes, Gen1::SourceGame::Yellow);
            assert(!parsed && parsed.error != Gen1::SaveError::None);
        }
        if (size == Gen2::kRawSaveSize32K || size == Gen2::kRawSaveSize64K) {
            const auto parsed = Gen2::parse(bytes, Gen2::SourceGame::Crystal);
            assert(!parsed && parsed.error != Gen2::SaveError::None);
        }
        assert(bytes == original);
        ++checks;
    }
}
void testPk3Codec(unsigned& checks) {
    uint32_t seed = 0x4a3b2c1du;
    for (const auto size : {Encryption::SIZE_STORED3_FRLG, Encryption::SIZE_PARTY3_FRLG}) {
        for (uint32_t shuffle = 0; shuffle != 24; ++shuffle) {
            std::vector<std::byte> plain(size, std::byte{0});
            put32(plain, 0, shuffle);
            put32(plain, 4, next(seed));
            for (size_t offset = 0x20; offset != 0x50; ++offset)
                plain[offset] = static_cast<std::byte>(next(seed));
            put16(plain, 0x1c, Encryption::checksum3FRLG(plain));
            std::unique_ptr<std::byte[]> encrypted(Encryption::encryptArray3FRLG(plain));
            assert(encrypted);
            std::vector<std::byte> native(encrypted.get(), encrypted.get() + size);
            const auto original = native;
            std::unique_ptr<std::byte[]> decoded(Encryption::decryptArray3FRLG(native));
            assert(decoded);
            assert(std::equal(plain.begin(), plain.end(), decoded.get()));
            assert(native == original);
            ++checks;
        }
    }
    // The codec accepts arbitrary bytes as a transport primitive, but short
    // input must remain byte-preserving and never attempt a PK3 block decode.
    for (size_t size = 0; size != Encryption::SIZE_STORED3_FRLG; ++size) {
        std::vector<std::byte> bytes(size, std::byte{0x5a});
        const auto original = bytes;
        std::unique_ptr<std::byte[]> decoded(Encryption::decryptArray3FRLG(bytes));
        std::unique_ptr<std::byte[]> encoded(Encryption::encryptArray3FRLG(bytes));
        assert(decoded && encoded);
        assert(std::equal(bytes.begin(), bytes.end(), decoded.get()));
        assert(std::equal(bytes.begin(), bytes.end(), encoded.get()));
        assert(bytes == original);
        ++checks;
    }
}
void testPk4Properties(unsigned& checks) {
    uint32_t seed = 0x13579bdfu;
    for (const auto size : {Encryption::SIZE_STORED4, Encryption::SIZE_PARTY4}) {
        for (unsigned caseNo = 0; caseNo != 48; ++caseNo) {
            const auto plain = pk4Plain(size, next(seed));
            const auto encrypted = Encryption::encryptArray4(plain);
            const auto preserved = encrypted;
            Pokemon::Pokemon4ReadOnly record(encrypted, Enums::GameVersion::PT);
            assert(record.valid() && !record.empty() && record.isParty() ==
                   (size == Encryption::SIZE_PARTY4));
            assert(record.species() == 1 && record.originVersion() == 12);
            assert(record.moves()[0] != 0 && record.originalEncryptedBytes().size() == size);
            assert(encrypted == preserved);
            if (size == Encryption::SIZE_PARTY4)
                assert(record.partyLevel() == 50 && record.maxHP() == 100);
            else
                assert(record.partyLevel() == 0 && record.maxHP() == 0);
            ++checks;
        }
    }
    // Exact all-zero stored and party records are valid empty slots.
    for (const auto size : {Encryption::SIZE_STORED4, Encryption::SIZE_PARTY4}) {
        std::vector<std::byte> zero(size, std::byte{0});
        Pokemon::Pokemon4ReadOnly empty(zero, Enums::GameVersion::Invalid);
        assert(empty.valid() && empty.empty() && empty.species() == 0);
        ++checks;
    }
    // Species zero alone must not convert a nonempty checksum-valid record
    // into a silently skipped box slot.
    const auto ghost = Encryption::encryptArray4(pk4Plain(Encryption::SIZE_STORED4, next(seed), 0));
    Pokemon::Pokemon4ReadOnly nonemptyZeroSpecies(ghost, Enums::GameVersion::Invalid);
    assert(nonemptyZeroSpecies.valid() && nonemptyZeroSpecies.species() == 0 &&
           !nonemptyZeroSpecies.empty());
    ++checks;

    // Every non-native length is quarantined and leaves caller bytes intact.
    for (size_t size = 0; size != Encryption::SIZE_PARTY4 + 17; ++size) {
        if (size == Encryption::SIZE_STORED4 || size == Encryption::SIZE_PARTY4) continue;
        std::vector<std::byte> bytes(size, std::byte{0xa5});
        const auto original = bytes;
        Pokemon::Pokemon4ReadOnly malformed(bytes, Enums::GameVersion::PT);
        assert(!malformed.sizeValid() && !malformed.valid() && !malformed.empty());
        assert(malformed.species() == 0 && malformed.moves() == std::array<uint16_t, 4>{});
        assert(bytes == original);
        ++checks;
    }
    // Bounded deterministic encrypted-input mutation: checksum failure must
    // quarantine semantic fields, without mutating the caller's immutable buffer.
    for (unsigned specimen = 0; specimen != 16; ++specimen) {
        const auto encrypted = Encryption::encryptArray4(
            pk4Plain(Encryption::SIZE_STORED4, next(seed)));
        for (unsigned mutation = 0; mutation != 32; ++mutation) {
            auto altered = encrypted;
            const size_t offset = 8 + (next(seed) % (Encryption::SIZE_STORED4 - 8));
            altered[offset] ^= static_cast<std::byte>(1u << (next(seed) & 7u));
            const auto original = altered;
            Pokemon::Pokemon4ReadOnly corrupt(altered, Enums::GameVersion::PT);
            assert(!corrupt.valid() && corrupt.species() == 0 && corrupt.moves() ==
                   std::array<uint16_t, 4>{});
            assert(altered == original);
            ++checks;
        }
    }
}
}

int main() {
    unsigned checks = 0;
    testGen12Boundaries(checks);
    testPk3Codec(checks);
    testPk4Properties(checks);
    assert(checks == 2021u);
    std::cout << "Stage 2 native Gen I-IV boundary/property checks: " << checks
              << " PASS (synthetic immutable inputs)\n";
}
