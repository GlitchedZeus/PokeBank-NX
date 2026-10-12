// L1 Stage 1: byte-for-byte independent-reference comparisons against the
// actual production Gen III and Gen IV encryption/decryption implementations.
// These are synthetic storage fixtures, NOT original retail acquisition proof.
#include "l1_native_encrypted_formats.h"
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

namespace R = L1NativeEncryptedFormats;

static std::vector<std::byte> bytes(std::span<const uint8_t> input) {
    std::vector<std::byte> out;
    out.reserve(input.size());
    for (uint8_t b : input) out.push_back(static_cast<std::byte>(b));
    return out;
}
static std::vector<uint8_t> octets(std::span<const std::byte> input) {
    std::vector<uint8_t> out;
    out.reserve(input.size());
    for (std::byte b : input) out.push_back(static_cast<uint8_t>(b));
    return out;
}

int main() {
    unsigned pk3Cases = 0, pk4Cases = 0, corruptedChecksums = 0;
    // Every original PK3 substructure order, both valid lengths, multiple
    // independent OT / header / payload patterns. Production does not
    // refresh the PK3 checksum; the fixture explicitly does so beforehand.
    for (uint32_t permutation = 0; permutation < 24; ++permutation) {
        for (size_t length : {size_t{0x50}, size_t{0x64}}) {
            for (uint32_t variant = 0; variant < 4; ++variant) {
                std::vector<uint8_t> canonical(length);
                for (size_t i = 0; i < length; ++i)
                    canonical[i] = static_cast<uint8_t>((i * 31u + variant * 43u + permutation) & 255u);
                R::wr32(canonical, 0, permutation + 24u * (variant + 1u));
                R::wr32(canonical, 4, 0xA5863412u ^ (permutation * 0x10201u + variant));
                R::wr16(canonical, 0x1C, R::checksum(canonical, 0x20, 0x50));
                const auto original = canonical;
                const auto reference = R::encrypt3(canonical);
                assert(R::valid3(reference));
                auto nativeInput = bytes(canonical);
                assert(Encryption::checksum3FRLG(nativeInput) == R::rd16(canonical, 0x1C));
                std::unique_ptr<std::byte[]> nativeCipher{
                    Encryption::encryptArray3FRLG(nativeInput)};
                assert(nativeCipher);
                const std::span<const std::byte> encrypted(nativeCipher.get(), length);
                assert(octets(encrypted) == reference);
                std::unique_ptr<std::byte[]> nativeClear{
                    Encryption::decryptArray3FRLG(encrypted)};
                assert(nativeClear);
                const std::span<const std::byte> clear(nativeClear.get(), length);
                assert(octets(clear) == original);
                assert(R::decrypt3(reference) == original);
                assert(canonical == original); // source record never altered
                ++pk3Cases;
            }
        }
    }
    // Gen IV has 32 PID shuffle indices (24 unique + eight repeated), and
    // stored vs party sizes additionally exercise separately seeded party
    // cryptography. Check the full ciphertext, not just mirror roundtrips.
    for (uint32_t shuffle = 0; shuffle < 32; ++shuffle) {
        for (size_t length : {size_t{0x88}, size_t{0xEC}}) {
            for (uint32_t variant = 0; variant < 4; ++variant) {
                std::vector<uint8_t> canonical(length);
                for (size_t i = 0; i < length; ++i)
                    canonical[i] = static_cast<uint8_t>((i * 13u + variant * 29u + shuffle) & 255u);
                R::wr32(canonical, 0, (shuffle << 13) | (variant << 2) | 1u);
                R::wr16(canonical, 4, 0); // native sanity word
                const auto original = canonical;
                const auto reference = R::encrypt4(canonical);
                assert(R::valid4(reference));
                const auto nativeInput = bytes(canonical);
                const auto nativeCipher = Encryption::encryptArray4(nativeInput);
                assert(octets(nativeCipher) == reference);
                const auto nativeClear = Encryption::decryptArray4(nativeCipher);
                auto expected = original;
                R::wr16(expected, 6, R::checksum(expected, 8, 0x88));
                assert(octets(nativeClear) == expected);
                assert(R::decrypt4(reference) == expected);
                assert(Encryption::checksum4(nativeClear) == R::rd16(expected, 6));
                auto corrupted = nativeCipher;
                corrupted[0x35] ^= std::byte{0x40};
                const auto broken = Encryption::decryptArray4(corrupted);
                assert(Encryption::checksum4(broken) !=
                    static_cast<uint16_t>(static_cast<uint8_t>(broken[6]) |
                    (static_cast<uint16_t>(static_cast<uint8_t>(broken[7])) << 8)));
                ++corruptedChecksums;
                assert(canonical == original);
                ++pk4Cases;
            }
        }
    }
    for (size_t badSize : {size_t{0}, size_t{1}, size_t{0x87},
                           size_t{0x89}, size_t{0xEB}, size_t{0xED}}) {
        std::vector<std::byte> malformed(badSize, std::byte{0});
        assert(!Encryption::validRecordSize4(badSize));
        assert(Encryption::encryptArray4(malformed).empty());
        assert(Encryption::decryptArray4(malformed).empty());
    }
    assert(pk3Cases == 192 && pk4Cases == 256 && corruptedChecksums == 256);
    std::cout << "L1 production codec comparison: PK3=" << pk3Cases
              << " PK4=" << pk4Cases
              << " checksum corruptions=" << corruptedChecksums << " PASS\n";
}
