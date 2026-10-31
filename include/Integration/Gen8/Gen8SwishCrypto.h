#ifndef POKEBANK_GEN8_SWISH_CRYPTO_H
#define POKEBANK_GEN8_SWISH_CRYPTO_H

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

// SwishCrypto + SCBlock container used by Sword/Shield (and Legends: Arceus) `main`.
//
// Constants and algorithms are transcribed from the pinned references, never inferred:
//   PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
//     PKHeX.Core/Saves/Encryption/SwishCrypto/SwishCrypto.cs   IntroHashBytes, OutroHashBytes,
//         StaticXorpad (0x7F significant bytes), CryptStaticXorpadBytes, ComputeHash, Decrypt/Encrypt
//     PKHeX.Core/Saves/Encryption/SwishCrypto/SCBlock.cs       ReadFromOffset / WriteBlock
//     PKHeX.Core/Saves/Encryption/SwishCrypto/SCXorShift32.cs  per-block key stream
//     PKHeX.Core/Saves/Encryption/SwishCrypto/SCTypeCode.cs    type codes and sizes
//   PKSM-Core aa22d7a4 (vendor/PKSM-Core) source/utils/crypto_swsh.cpp
//     (independent implementation; tests/test_gen8_swsh.cpp compares against it).
namespace PokeVault::Integration::Gen8::Swish {

inline constexpr size_t HashSize = 0x20;

inline constexpr std::array<uint8_t, 64> IntroHashBytes = {
    0x9E, 0xC9, 0x9C, 0xD7, 0x0E, 0xD3, 0x3C, 0x44, 0xFB, 0x93, 0x03, 0xDC, 0xEB, 0x39, 0xB4, 0x2A,
    0x19, 0x47, 0xE9, 0x63, 0x4B, 0xA2, 0x33, 0x44, 0x16, 0xBF, 0x82, 0xA2, 0xBA, 0x63, 0x55, 0xB6,
    0x3D, 0x9D, 0xF2, 0x4B, 0x5F, 0x7B, 0x6A, 0xB2, 0x62, 0x1D, 0xC2, 0x1B, 0x68, 0xE5, 0xC8, 0xB5,
    0x3A, 0x05, 0x90, 0x00, 0xE8, 0xA8, 0x10, 0x3D, 0xE2, 0xEC, 0xF0, 0x0C, 0xB2, 0xED, 0x4F, 0x6D,
};
inline constexpr std::array<uint8_t, 64> OutroHashBytes = {
    0xD6, 0xC0, 0x1C, 0x59, 0x8B, 0xC8, 0xB8, 0xCB, 0x46, 0xE1, 0x53, 0xFC, 0x82, 0x8C, 0x75, 0x75,
    0x13, 0xE0, 0x45, 0xDF, 0x32, 0x69, 0x3C, 0x75, 0xF0, 0x59, 0xF8, 0xD9, 0xA2, 0x5F, 0xB2, 0x17,
    0xE0, 0x80, 0x52, 0xDB, 0xEA, 0x89, 0x73, 0x99, 0x75, 0x79, 0xAF, 0xCB, 0x2E, 0x80, 0x07, 0xE6,
    0xF1, 0x26, 0xE0, 0x03, 0x0A, 0xE6, 0x6F, 0xF6, 0x41, 0xBF, 0x7E, 0x59, 0xC2, 0xAE, 0x55, 0xFD,
};
// The 0x7F significant xorpad bytes. PKHeX pads its copy to 0x80 with a zero and advances
// by 0x7F per chunk, which is exactly payload[i] ^= pad[i % 0x7F] (as PKSM-Core writes it).
inline constexpr std::array<uint8_t, 0x7F> StaticXorpad = {
    0xA0, 0x92, 0xD1, 0x06, 0x07, 0xDB, 0x32, 0xA1, 0xAE, 0x01, 0xF5, 0xC5, 0x1E, 0x84, 0x4F, 0xE3,
    0x53, 0xCA, 0x37, 0xF4, 0xA7, 0xB0, 0x4D, 0xA0, 0x18, 0xB7, 0xC2, 0x97, 0xDA, 0x5F, 0x53, 0x2B,
    0x75, 0xFA, 0x48, 0x16, 0xF8, 0xD4, 0x8A, 0x6F, 0x61, 0x05, 0xF4, 0xE2, 0xFD, 0x04, 0xB5, 0xA3,
    0x0F, 0xFC, 0x44, 0x92, 0xCB, 0x32, 0xE6, 0x1B, 0xB9, 0xB1, 0x2E, 0x01, 0xB0, 0x56, 0x53, 0x36,
    0xD2, 0xD1, 0x50, 0x3D, 0xDE, 0x5B, 0x2E, 0x0E, 0x52, 0xFD, 0xDF, 0x2F, 0x7B, 0xCA, 0x63, 0x50,
    0xA4, 0x67, 0x5D, 0x23, 0x17, 0xC0, 0x52, 0xE1, 0xA6, 0x30, 0x7C, 0x2B, 0xB6, 0x70, 0x36, 0x5B,
    0x2A, 0x27, 0x69, 0x33, 0xF5, 0x63, 0x7B, 0x36, 0x3F, 0x26, 0x9B, 0xA3, 0xED, 0x7A, 0x53, 0x00,
    0xA4, 0x48, 0xB3, 0x50, 0x9E, 0x14, 0xA0, 0x52, 0xDE, 0x7E, 0x10, 0x2B, 0x1B, 0x77, 0x6E,
};

// SCTypeCode values.
enum class TypeCode : uint8_t {
    None = 0, Bool1 = 1, Bool2 = 2, Bool3 = 3, Object = 4, Array = 5,
    Byte = 8, UInt16 = 9, UInt32 = 10, UInt64 = 11,
    SByte = 12, Int16 = 13, Int32 = 14, Int64 = 15, Single = 16, Double = 17,
};

// SCTypeCode.GetTypeSize; 0 for codes that have no element size.
[[nodiscard]] constexpr size_t typeSize(TypeCode t) noexcept {
    switch (t) {
        case TypeCode::Bool3: case TypeCode::Byte: case TypeCode::SByte: return 1;
        case TypeCode::UInt16: case TypeCode::Int16: return 2;
        case TypeCode::UInt32: case TypeCode::Int32: case TypeCode::Single: return 4;
        case TypeCode::UInt64: case TypeCode::Int64: case TypeCode::Double: return 8;
        default: return 0;
    }
}
[[nodiscard]] constexpr bool isValueType(TypeCode t) noexcept {
    return static_cast<uint8_t>(t) >= 8 && static_cast<uint8_t>(t) <= 17;
}

// SCXorShift32: seed advanced popcount(seed) times, then bytes are taken LSB-first.
class XorShift32 {
public:
    explicit constexpr XorShift32(uint32_t seed) noexcept : state_(seed) {
        const int count = std::popcount(seed);
        for (int i = 0; i < count; ++i) state_ = advance(state_);
    }
    constexpr uint8_t next() noexcept {
        const auto out = static_cast<uint8_t>(state_ >> (counter_ << 3));
        if (counter_ == 3) {
            state_ = advance(state_);
            counter_ = 0;
        } else {
            ++counter_;
        }
        return out;
    }
    constexpr uint32_t next32() noexcept {
        uint32_t v = next();
        v |= static_cast<uint32_t>(next()) << 8;
        v |= static_cast<uint32_t>(next()) << 16;
        v |= static_cast<uint32_t>(next()) << 24;
        return v;
    }

private:
    static constexpr uint32_t advance(uint32_t s) noexcept {
        s ^= s << 2;
        s ^= s >> 15;
        s ^= s << 13;
        return s;
    }
    uint32_t state_;
    uint32_t counter_ = 0;
};

// One decrypted block. `data` is the decrypted payload (empty for Bool1/2/3).
struct Block {
    uint32_t key = 0;
    TypeCode type = TypeCode::None;
    TypeCode subtype = TypeCode::None;  // Array only
    std::vector<uint8_t> data;

    [[nodiscard]] bool bytePayload() const noexcept {
        return type == TypeCode::Object || (type == TypeCode::Array && subtype == TypeCode::Byte);
    }
    [[nodiscard]] size_t serializedSize() const noexcept {
        return 5 + (type == TypeCode::Object ? 4 : type == TypeCode::Array ? 5 : 0) + data.size();
    }
};

// SHA-256(Intro || payload || Outro) over the xorpad-encrypted payload.
[[nodiscard]] std::array<uint8_t, HashSize> computeHash(std::span<const uint8_t> payload);
[[nodiscard]] bool hashValid(std::span<const uint8_t> file);
void cryptStaticXorpad(std::span<uint8_t> payload) noexcept;

enum class ParseStatus : uint8_t { Ok, TooSmall, HashMismatch, UnknownType, Truncated, DuplicateKey };
[[nodiscard]] const char* parseStatusText(ParseStatus status) noexcept;

// Strict decrypt: hash first, then every block must be a known SCTypeCode whose
// declared length stays inside the payload, the blocks must consume the payload
// exactly, and keys must be unique (PKHeX's SCBlockAccessor looks keys up by value).
[[nodiscard]] std::optional<std::vector<Block>> decrypt(std::span<const uint8_t> file,
                                                        ParseStatus* status = nullptr);

// Exact inverse of decrypt (SwishCrypto.Encrypt): serialize, xorpad, append hash.
[[nodiscard]] std::vector<uint8_t> encrypt(const std::vector<Block>& blocks);

// One block serialized with its key stream (SCBlock.WriteBlock).
void writeBlock(const Block& block, std::vector<uint8_t>& out);

} // namespace PokeVault::Integration::Gen8::Swish

#endif
