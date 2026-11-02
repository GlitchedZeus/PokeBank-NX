#ifndef POKEBANK_GEN8_PK8_H
#define POKEBANK_GEN8_PK8_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Generation VIII PK8 record codec (Sword/Shield).
//
// Every constant below is taken from the pinned references, never inferred:
//   PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
//     PKHeX.Core/PKM/Util/PokeCrypto.cs  SIZE_8STORED=0x148, SIZE_8PARTY=0x158, SIZE_8BLOCK=80,
//                                        BlockPosition/BlockPositionInvert, Decrypt8/Encrypt8
//                                        (shuffle by (EC >> 13) & 31, CryptArray 0x41C64E6D/0x6073)
//     PKHeX.Core/PKM/Shared/G8PKM.cs     checksum = Add16 over [0x08, 0x148), Valid => Sanity==0,
//                                        trash spans 0x58/0xA8/0xF8 (26 bytes), every field offset
//     PKHeX.Core/Saves/SAV8SWSH.cs       boxes hold the party-size record ("party format in boxes!")
//   PKSM-Core aa22d7a4 (vendor/PKSM-Core) include/pkx/PK8.hpp, source/pkx/PK8.cpp
//     (independent second implementation; tests/test_gen8_pk8.cpp compares against it).
namespace PokeVault::Integration::Gen8::PK8 {

inline constexpr size_t StoredSize = 0x148;  // 328
inline constexpr size_t PartySize = 0x158;   // 344
inline constexpr size_t BlockSize = 80;
inline constexpr size_t BlockCount = 4;
inline constexpr size_t HeaderSize = 8;      // EC (4) + sanity (2) + checksum (2), never encrypted

[[nodiscard]] inline uint16_t read16(std::span<const uint8_t> b, size_t o) noexcept {
    return static_cast<uint16_t>(b[o] | (b[o + 1] << 8));
}
[[nodiscard]] inline uint32_t read32(std::span<const uint8_t> b, size_t o) noexcept {
    return static_cast<uint32_t>(b[o]) | (static_cast<uint32_t>(b[o + 1]) << 8) |
           (static_cast<uint32_t>(b[o + 2]) << 16) | (static_cast<uint32_t>(b[o + 3]) << 24);
}
inline void write16(std::span<uint8_t> b, size_t o, uint16_t v) noexcept {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
inline void write32(std::span<uint8_t> b, size_t o, uint32_t v) noexcept {
    for (size_t i = 0; i < 4; ++i) b[o + i] = static_cast<uint8_t>(v >> (8 * i));
}

// PokeCrypto.BlockPosition rows 0..23 (rows 24..31 duplicate 0..7 so PKHeX can index
// with (EC >> 13) & 31 directly; % 24 selects the same row).
inline constexpr std::array<uint8_t, 24 * 4> BlockPosition = {
    0, 1, 2, 3,  0, 1, 3, 2,  0, 2, 1, 3,  0, 3, 1, 2,
    0, 2, 3, 1,  0, 3, 2, 1,  1, 0, 2, 3,  1, 0, 3, 2,
    2, 0, 1, 3,  3, 0, 1, 2,  2, 0, 3, 1,  3, 0, 2, 1,
    1, 2, 0, 3,  1, 3, 0, 2,  2, 1, 0, 3,  3, 1, 0, 2,
    2, 3, 0, 1,  3, 2, 0, 1,  1, 2, 3, 0,  1, 3, 2, 0,
    2, 1, 3, 0,  3, 1, 2, 0,  2, 3, 1, 0,  3, 2, 1, 0,
};
inline constexpr std::array<uint8_t, 24> BlockPositionInvert = {
    0, 1, 2, 4, 3, 5, 6, 7, 12, 18, 13, 19, 8, 10, 14, 20, 16, 22, 9, 11, 15, 21, 17, 23,
};

[[nodiscard]] constexpr uint32_t shuffleValue(uint32_t ec) noexcept { return ((ec >> 13) & 31) % 24; }

// PokeCrypto.CryptArray: little-endian u16 XOR with the high half of an LCG.
inline void cryptArray(std::span<uint8_t> data, uint32_t seed) noexcept {
    for (size_t i = 0; i + 1 < data.size(); i += 2) {
        seed = 0x41C64E6Du * seed + 0x00006073u;
        const uint16_t x = static_cast<uint16_t>(seed >> 16);
        data[i] ^= static_cast<uint8_t>(x);
        data[i + 1] ^= static_cast<uint8_t>(x >> 8);
    }
}

inline void shuffle(std::span<uint8_t> blocks, uint32_t sv) noexcept {
    std::array<uint8_t, BlockSize * BlockCount> copy{};
    for (size_t i = 0; i < copy.size(); ++i) copy[i] = blocks[i];
    for (size_t i = 0; i < BlockCount; ++i) {
        const size_t from = BlockPosition[sv * BlockCount + i];
        for (size_t j = 0; j < BlockSize; ++j) blocks[i * BlockSize + j] = copy[from * BlockSize + j];
    }
}

[[nodiscard]] constexpr bool supportedSize(size_t size) noexcept {
    return size == StoredSize || size == PartySize;
}

// Decrypt8. Returns empty on an unknown size.
[[nodiscard]] inline std::vector<uint8_t> decrypt(std::span<const uint8_t> encrypted) {
    if (!supportedSize(encrypted.size())) return {};
    std::vector<uint8_t> out(encrypted.begin(), encrypted.end());
    const uint32_t ec = read32(out, 0);
    std::span<uint8_t> body(out.data() + HeaderSize, StoredSize - HeaderSize);
    cryptArray(body, ec);
    shuffle(body, shuffleValue(ec));
    if (out.size() > StoredSize) cryptArray(std::span<uint8_t>(out).subspan(StoredSize), ec);
    return out;
}

// Encrypt8 (exact inverse of decrypt).
[[nodiscard]] inline std::vector<uint8_t> encrypt(std::span<const uint8_t> plain) {
    if (!supportedSize(plain.size())) return {};
    std::vector<uint8_t> out(plain.begin(), plain.end());
    const uint32_t ec = read32(out, 0);
    std::span<uint8_t> body(out.data() + HeaderSize, StoredSize - HeaderSize);
    shuffle(body, BlockPositionInvert[shuffleValue(ec)]);
    cryptArray(body, ec);
    if (out.size() > StoredSize) cryptArray(std::span<uint8_t>(out).subspan(StoredSize), ec);
    return out;
}

// G8PKM.CalculateChecksum: 16-bit sum of little-endian words over [0x08, 0x148).
[[nodiscard]] inline uint16_t checksum(std::span<const uint8_t> plain) noexcept {
    uint16_t sum = 0;
    for (size_t o = HeaderSize; o + 1 < StoredSize; o += 2)
        sum = static_cast<uint16_t>(sum + read16(plain, o));
    return sum;
}

enum class DecodeStatus : uint8_t { Ok, UnknownSize, BadSanity, BadChecksum };

[[nodiscard]] constexpr const char* decodeStatusText(DecodeStatus status) noexcept {
    switch (status) {
        case DecodeStatus::Ok: return "PK8 valid";
        case DecodeStatus::UnknownSize: return "PK8 record size is neither 328 nor 344 bytes";
        case DecodeStatus::BadSanity: return "PK8 sanity word is nonzero";
        case DecodeStatus::BadChecksum: return "PK8 checksum mismatch";
    }
    return "PK8 invalid";
}

// Gen VIII text is UTF-16LE; nickname/OT hold 12 characters plus a terminator (26 bytes).
inline constexpr size_t NameCapacity = 12;
inline constexpr size_t NicknameOffset = 0x58;
inline constexpr size_t HandlerNameOffset = 0xA8;
inline constexpr size_t OriginalTrainerOffset = 0xF8;
inline constexpr size_t NameBytes = 26;

// Decrypted PK8 with typed accessors; only ever a strictly validated record.
class Pokemon8 {
public:
    [[nodiscard]] static std::optional<Pokemon8> decode(std::span<const uint8_t> encrypted,
                                                        DecodeStatus* status = nullptr) {
        auto report = [&](DecodeStatus s) { if (status) *status = s; };
        if (!supportedSize(encrypted.size())) {
            report(DecodeStatus::UnknownSize);
            return std::nullopt;
        }
        Pokemon8 out;
        out.plain_ = decrypt(encrypted);
        if (out.sanity() != 0) {
            report(DecodeStatus::BadSanity);
            return std::nullopt;
        }
        if (checksum(out.plain_) != out.storedChecksum()) {
            report(DecodeStatus::BadChecksum);
            return std::nullopt;
        }
        report(DecodeStatus::Ok);
        return out;
    }

    // An all-zero decrypted record: PKHeX's BlankPKM, written encrypted to clear a slot.
    [[nodiscard]] static Pokemon8 blank(size_t size = PartySize) {
        Pokemon8 out;
        out.plain_.assign(supportedSize(size) ? size : PartySize, 0);
        return out;
    }

    [[nodiscard]] std::span<const uint8_t> plain() const noexcept { return plain_; }
    [[nodiscard]] size_t size() const noexcept { return plain_.size(); }

    // Encrypted bytes with a freshly computed checksum.
    [[nodiscard]] std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> copy = plain_;
        write16(copy, 0x06, checksum(copy));
        return encrypt(copy);
    }

    [[nodiscard]] bool empty() const noexcept { return species() == 0; }
    [[nodiscard]] bool allZero() const noexcept {
        for (uint8_t b : plain_) if (b) return false;
        return true;
    }

    [[nodiscard]] uint32_t encryptionConstant() const noexcept { return read32(plain_, 0x00); }
    [[nodiscard]] uint16_t sanity() const noexcept { return read16(plain_, 0x04); }
    [[nodiscard]] uint16_t storedChecksum() const noexcept { return read16(plain_, 0x06); }
    [[nodiscard]] uint16_t species() const noexcept { return read16(plain_, 0x08); }
    [[nodiscard]] uint16_t heldItem() const noexcept { return read16(plain_, 0x0A); }
    [[nodiscard]] uint16_t tid() const noexcept { return read16(plain_, 0x0C); }
    [[nodiscard]] uint16_t sid() const noexcept { return read16(plain_, 0x0E); }
    [[nodiscard]] uint32_t experience() const noexcept { return read32(plain_, 0x10); }
    [[nodiscard]] uint16_t ability() const noexcept { return read16(plain_, 0x14); }
    [[nodiscard]] uint8_t abilityNumber() const noexcept { return plain_[0x16] & 7; }
    [[nodiscard]] uint32_t pid() const noexcept { return read32(plain_, 0x1C); }
    [[nodiscard]] uint8_t nature() const noexcept { return plain_[0x20]; }
    [[nodiscard]] uint8_t statNature() const noexcept { return plain_[0x21]; }
    [[nodiscard]] bool fatefulEncounter() const noexcept { return (plain_[0x22] & 1) != 0; }
    [[nodiscard]] uint8_t gender() const noexcept { return (plain_[0x22] >> 2) & 3; }
    [[nodiscard]] uint16_t form() const noexcept { return read16(plain_, 0x24); }
    [[nodiscard]] uint8_t ev(size_t stat) const noexcept { return stat < 6 ? plain_[0x26 + stat] : 0; }
    [[nodiscard]] uint8_t pokerus() const noexcept { return plain_[0x32]; }
    [[nodiscard]] uint16_t move(size_t i) const noexcept { return i < 4 ? read16(plain_, 0x72 + 2 * i) : 0; }
    [[nodiscard]] uint16_t currentHp() const noexcept { return read16(plain_, 0x8A); }
    [[nodiscard]] uint32_t iv32() const noexcept { return read32(plain_, 0x8C); }
    [[nodiscard]] uint8_t iv(size_t stat) const noexcept {
        return stat < 6 ? static_cast<uint8_t>((iv32() >> (5 * stat)) & 0x1F) : 0;
    }
    [[nodiscard]] bool isEgg() const noexcept { return ((iv32() >> 30) & 1) != 0; }
    [[nodiscard]] bool isNicknamed() const noexcept { return ((iv32() >> 31) & 1) != 0; }
    [[nodiscard]] uint8_t dynamaxLevel() const noexcept { return plain_[0x90]; }
    [[nodiscard]] uint8_t currentHandler() const noexcept { return plain_[0xC4]; }
    [[nodiscard]] uint8_t handlingTrainerFriendship() const noexcept { return plain_[0xC8]; }
    [[nodiscard]] uint8_t originVersion() const noexcept { return plain_[0xDE]; }
    [[nodiscard]] uint8_t language() const noexcept { return plain_[0xE2]; }
    [[nodiscard]] uint8_t originalTrainerFriendship() const noexcept { return plain_[0x112]; }
    [[nodiscard]] uint8_t ball() const noexcept { return plain_[0x124]; }
    [[nodiscard]] uint8_t metLevel() const noexcept { return plain_[0x125] & 0x7F; }
    [[nodiscard]] uint8_t originalTrainerGender() const noexcept { return plain_[0x125] >> 7; }
    [[nodiscard]] uint8_t hyperTrainFlags() const noexcept { return plain_[0x126]; }
    // Cached battle stats (0x148..0x156); zero for a 0x148-byte stored record.
    [[nodiscard]] uint8_t statLevel() const noexcept { return plain_.size() == PartySize ? plain_[0x148] : 0; }
    [[nodiscard]] uint16_t stat(size_t i) const noexcept {  // HPmax, Atk, Def, Spe, SpA, SpD
        return plain_.size() == PartySize && i < 6 ? read16(plain_, 0x14A + 2 * i) : 0;
    }

    // G8PKM.IsShiny: (TID ^ SID ^ PID-high ^ PID-low) < 16.
    [[nodiscard]] bool shiny() const noexcept {
        const uint32_t p = pid();
        return ((tid() ^ sid() ^ (p >> 16) ^ (p & 0xFFFF)) & 0xFFFF) < 16;
    }

    [[nodiscard]] std::u16string rawName(size_t offset) const {
        std::u16string out;
        for (size_t i = 0; i < NameCapacity + 1; ++i) {
            const uint16_t ch = read16(plain_, offset + 2 * i);
            if (ch == 0) break;
            out.push_back(static_cast<char16_t>(ch));
        }
        return out;
    }
    [[nodiscard]] std::u16string nickname() const { return rawName(NicknameOffset); }
    [[nodiscard]] std::u16string originalTrainerName() const { return rawName(OriginalTrainerOffset); }

    // ---- Raw mutators: the staged session validates every value before calling these. ----
    void setHeldItem(uint16_t v) noexcept { write16(plain_, 0x0A, v); }
    void setTid(uint16_t v) noexcept { write16(plain_, 0x0C, v); }
    void setSid(uint16_t v) noexcept { write16(plain_, 0x0E, v); }
    void setFatefulEncounter(bool v) noexcept {
        plain_[0x22] = static_cast<uint8_t>((plain_[0x22] & ~1) | (v ? 1 : 0));
    }
    void setPokerus(uint8_t v) noexcept { plain_[0x32] = v; }
    void setNicknamedFlag(bool v) noexcept {
        write32(plain_, 0x8C, (iv32() & 0x7FFFFFFFu) | (v ? 0x80000000u : 0u));
    }
    void setOriginalTrainerFriendship(uint8_t v) noexcept { plain_[0x112] = v; }
    void setBall(uint8_t v) noexcept { plain_[0x124] = v; }
    void setOriginalTrainerGender(uint8_t v) noexcept {
        plain_[0x125] = static_cast<uint8_t>((plain_[0x125] & 0x7F) | ((v & 1) << 7));
    }
    // Writes up to 12 UTF-16 units, then zero-fills the rest of the 26-byte span.
    void setRawName(size_t offset, std::u16string_view name) noexcept {
        for (size_t i = 0; i < NameBytes; ++i) plain_[offset + i] = 0;
        for (size_t i = 0; i < name.size() && i < NameCapacity; ++i)
            write16(plain_, offset + 2 * i, static_cast<uint16_t>(name[i]));
    }

private:
    Pokemon8() = default;
    std::vector<uint8_t> plain_;
};

} // namespace PokeVault::Integration::Gen8::PK8

#endif
