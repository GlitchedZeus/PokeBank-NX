#ifndef POKEBANK_GEN6_PK6_H
#define POKEBANK_GEN6_PK6_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Generation VI PK6 record codec (X/Y/Omega Ruby/Alpha Sapphire).
//
// Every constant below is taken from the pinned references, never inferred:
//   PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
//     PKHeX.Core/PKM/Util/PokeCrypto.cs   SIZE_6STORED=0xE8, SIZE_6PARTY=0x104,
//                                         SIZE_6BLOCK=56, BlockPosition/BlockPositionInvert,
//                                         Decrypt67/Encrypt67, CryptArray (0x41C64E6D/0x6073)
//     PKHeX.Core/PKM/Shared/G6PKM.cs      checksum = Add16 over [0x08, 0xE8), Valid => Sanity==0,
//                                         trash spans 0x40/0x78/0xB0 (26 bytes), PSV/TSV >> 4
//     PKHeX.Core/PKM/PK6.cs               every field offset used by Pokemon6 below
//   PKSM-Core aa22d7a4f87c0351baf7da5962ba5acd01039a7c (vendor/PKSM-Core) PK6.hpp/PK6.cpp
//     (independent second implementation of the same structure).
namespace PokeVault::Integration::Gen6::PK6 {

inline constexpr size_t StoredSize = 0xE8;  // 232
inline constexpr size_t PartySize = 0x104;  // 260
inline constexpr size_t BlockSize = 56;
inline constexpr size_t BlockCount = 4;
inline constexpr size_t HeaderSize = 8;     // EC (4) + sanity (2) + checksum (2), never encrypted

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

// PokeCrypto.BlockPosition rows 0..23 (the 24..31 duplicates exist only so that
// PKHeX can skip a modulus; (pv >> 13) & 31 is reduced with % 24 here instead).
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

[[nodiscard]] constexpr uint32_t shuffleValue(uint32_t pv) noexcept { return ((pv >> 13) & 31) % 24; }

// PokeCrypto.CryptArray: little-endian u16 XOR with the high half of an LCG.
inline void cryptArray(std::span<uint8_t> data, uint32_t seed) noexcept {
    for (size_t i = 0; i + 1 < data.size(); i += 2) {
        seed = 0x41C64E6Du * seed + 0x00006073u;
        const uint16_t x = static_cast<uint16_t>(seed >> 16);
        data[i] ^= static_cast<uint8_t>(x);
        data[i + 1] ^= static_cast<uint8_t>(x >> 8);
    }
}

// out[i] = in[BlockPosition[sv*4+i]] -- the same permutation PKHeX's in-place Shuffle yields.
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

// Decrypt67. Returns empty on an unknown size; never guesses a geometry.
[[nodiscard]] inline std::vector<uint8_t> decrypt(std::span<const uint8_t> encrypted) {
    if (!supportedSize(encrypted.size())) return {};
    std::vector<uint8_t> out(encrypted.begin(), encrypted.end());
    const uint32_t pv = read32(out, 0);
    std::span<uint8_t> body(out.data() + HeaderSize, StoredSize - HeaderSize);
    cryptArray(body, pv);
    if (out.size() > StoredSize) cryptArray(std::span<uint8_t>(out).subspan(StoredSize), pv);
    shuffle(body, shuffleValue(pv));
    return out;
}

// Encrypt67 (exact inverse of decrypt).
[[nodiscard]] inline std::vector<uint8_t> encrypt(std::span<const uint8_t> plain) {
    if (!supportedSize(plain.size())) return {};
    std::vector<uint8_t> out(plain.begin(), plain.end());
    const uint32_t pv = read32(out, 0);
    std::span<uint8_t> body(out.data() + HeaderSize, StoredSize - HeaderSize);
    shuffle(body, BlockPositionInvert[shuffleValue(pv)]);
    cryptArray(body, pv);
    if (out.size() > StoredSize) cryptArray(std::span<uint8_t>(out).subspan(StoredSize), pv);
    return out;
}

// G6PKM.CalculateChecksum: 16-bit sum of little-endian words over [0x08, 0xE8).
[[nodiscard]] inline uint16_t checksum(std::span<const uint8_t> plain) noexcept {
    uint16_t sum = 0;
    for (size_t o = HeaderSize; o + 1 < StoredSize; o += 2)
        sum = static_cast<uint16_t>(sum + read16(plain, o));
    return sum;
}

enum class DecodeStatus : uint8_t { Ok, UnknownSize, BadSanity, BadChecksum };

[[nodiscard]] constexpr const char* decodeStatusText(DecodeStatus status) noexcept {
    switch (status) {
        case DecodeStatus::Ok: return "PK6 valid";
        case DecodeStatus::UnknownSize: return "PK6 record size is neither 232 nor 260 bytes";
        case DecodeStatus::BadSanity: return "PK6 sanity word is nonzero";
        case DecodeStatus::BadChecksum: return "PK6 checksum mismatch";
    }
    return "PK6 invalid";
}

// Gen VI text is UTF-16LE; nickname/OT hold 12 characters plus a terminator (26 bytes).
inline constexpr size_t NameCapacity = 12;
inline constexpr size_t NicknameOffset = 0x40;
inline constexpr size_t HandlerNameOffset = 0x78;
inline constexpr size_t OriginalTrainerOffset = 0xB0;
inline constexpr size_t NameBytes = 26;

// Decrypted PK6 with typed accessors. It only ever represents a strictly
// validated record: construction goes through decode(), and serialize()
// always refreshes the checksum before re-encrypting.
class Pokemon6 {
public:
    [[nodiscard]] static std::optional<Pokemon6> decode(std::span<const uint8_t> encrypted,
                                                        DecodeStatus* status = nullptr) {
        auto report = [&](DecodeStatus s) { if (status) *status = s; };
        if (!supportedSize(encrypted.size())) {
            report(DecodeStatus::UnknownSize);
            return std::nullopt;
        }
        Pokemon6 out;
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

    // An all-zero decrypted record (blank slot as PKHeX writes it).
    [[nodiscard]] static Pokemon6 blank(size_t size = StoredSize) {
        Pokemon6 out;
        out.plain_.assign(supportedSize(size) ? size : StoredSize, 0);
        return out;
    }

    [[nodiscard]] std::span<const uint8_t> plain() const noexcept { return plain_; }
    [[nodiscard]] size_t size() const noexcept { return plain_.size(); }
    [[nodiscard]] bool partyForm() const noexcept { return plain_.size() == PartySize; }

    // Encrypted bytes with a freshly computed checksum.
    [[nodiscard]] std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> copy = plain_;
        write16(copy, 0x06, checksum(copy));
        return encrypt(copy);
    }

    // Native boxed form of a party record: the 0xE8 stored prefix only.
    [[nodiscard]] Pokemon6 boxed() const {
        Pokemon6 out;
        out.plain_.assign(plain_.begin(), plain_.begin() + static_cast<std::ptrdiff_t>(StoredSize));
        return out;
    }

    [[nodiscard]] bool empty() const noexcept { return species() == 0; }
    [[nodiscard]] bool allZero() const noexcept {
        for (uint8_t b : plain_) if (b) return false;
        return true;
    }

    // Block A
    [[nodiscard]] uint32_t encryptionConstant() const noexcept { return read32(plain_, 0x00); }
    [[nodiscard]] uint16_t sanity() const noexcept { return read16(plain_, 0x04); }
    [[nodiscard]] uint16_t storedChecksum() const noexcept { return read16(plain_, 0x06); }
    [[nodiscard]] uint16_t species() const noexcept { return read16(plain_, 0x08); }
    [[nodiscard]] uint16_t heldItem() const noexcept { return read16(plain_, 0x0A); }
    [[nodiscard]] uint16_t tid() const noexcept { return read16(plain_, 0x0C); }
    [[nodiscard]] uint16_t sid() const noexcept { return read16(plain_, 0x0E); }
    [[nodiscard]] uint32_t experience() const noexcept { return read32(plain_, 0x10); }
    [[nodiscard]] uint8_t ability() const noexcept { return plain_[0x14]; }
    [[nodiscard]] uint8_t abilityNumber() const noexcept { return plain_[0x15]; }
    [[nodiscard]] uint32_t pid() const noexcept { return read32(plain_, 0x18); }
    [[nodiscard]] uint8_t nature() const noexcept { return plain_[0x1C]; }
    [[nodiscard]] bool fatefulEncounter() const noexcept { return (plain_[0x1D] & 1) != 0; }
    [[nodiscard]] uint8_t gender() const noexcept { return (plain_[0x1D] >> 1) & 3; }
    [[nodiscard]] uint8_t form() const noexcept { return plain_[0x1D] >> 3; }
    [[nodiscard]] uint8_t ev(size_t stat) const noexcept { return stat < 6 ? plain_[0x1E + stat] : 0; }
    [[nodiscard]] uint8_t pokerus() const noexcept { return plain_[0x2B]; }
    // Block B
    [[nodiscard]] uint16_t move(size_t i) const noexcept { return i < 4 ? read16(plain_, 0x5A + 2 * i) : 0; }
    [[nodiscard]] uint8_t movePP(size_t i) const noexcept { return i < 4 ? plain_[0x62 + i] : 0; }
    [[nodiscard]] uint8_t movePPUps(size_t i) const noexcept { return i < 4 ? plain_[0x66 + i] : 0; }
    [[nodiscard]] uint32_t iv32() const noexcept { return read32(plain_, 0x74); }
    [[nodiscard]] uint8_t iv(size_t stat) const noexcept {
        return stat < 6 ? static_cast<uint8_t>((iv32() >> (5 * stat)) & 0x1F) : 0;
    }
    [[nodiscard]] bool isEgg() const noexcept { return ((iv32() >> 30) & 1) != 0; }
    [[nodiscard]] bool isNicknamed() const noexcept { return ((iv32() >> 31) & 1) != 0; }
    // Block C/D
    [[nodiscard]] uint8_t currentHandler() const noexcept { return plain_[0x93]; }
    [[nodiscard]] uint8_t originalTrainerFriendship() const noexcept { return plain_[0xCA]; }
    [[nodiscard]] uint8_t eggYear() const noexcept { return plain_[0xD1]; }
    [[nodiscard]] uint8_t metYear() const noexcept { return plain_[0xD4]; }
    [[nodiscard]] uint8_t metMonth() const noexcept { return plain_[0xD5]; }
    [[nodiscard]] uint8_t metDay() const noexcept { return plain_[0xD6]; }
    [[nodiscard]] uint16_t eggLocation() const noexcept { return read16(plain_, 0xD8); }
    [[nodiscard]] uint16_t metLocation() const noexcept { return read16(plain_, 0xDA); }
    [[nodiscard]] uint8_t ball() const noexcept { return plain_[0xDC]; }
    [[nodiscard]] uint8_t metLevel() const noexcept { return plain_[0xDD] & 0x7F; }
    [[nodiscard]] uint8_t originalTrainerGender() const noexcept { return plain_[0xDD] >> 7; }
    [[nodiscard]] uint8_t originVersion() const noexcept { return plain_[0xDF]; }
    [[nodiscard]] uint8_t language() const noexcept { return plain_[0xE3]; }
    // Party-only battle stats (0xE8..0x104). Zero for a boxed record.
    [[nodiscard]] uint8_t statLevel() const noexcept { return partyForm() ? plain_[0xEC] : 0; }
    [[nodiscard]] uint16_t stat(size_t i) const noexcept {
        return partyForm() && i < 7 ? read16(plain_, 0xF0 + 2 * i) : 0;  // HPcur, HPmax, Atk..SpD
    }

    // G6PKM: shiny when PSV == TSV, i.e. the four-half XOR is below 16.
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
    void setNature(uint8_t v) noexcept { plain_[0x1C] = v; }
    void setFatefulEncounter(bool v) noexcept {
        plain_[0x1D] = static_cast<uint8_t>((plain_[0x1D] & ~1) | (v ? 1 : 0));
    }
    void setEv(size_t stat, uint8_t v) noexcept { if (stat < 6) plain_[0x1E + stat] = v; }
    void setPokerus(uint8_t v) noexcept { plain_[0x2B] = v; }
    void setIv(size_t stat, uint8_t v) noexcept {
        if (stat >= 6) return;
        const uint32_t shift = static_cast<uint32_t>(5 * stat);
        const uint32_t next = (iv32() & ~(0x1Fu << shift)) | ((static_cast<uint32_t>(v) & 0x1F) << shift);
        write32(plain_, 0x74, next);
    }
    void setNicknamedFlag(bool v) noexcept {
        write32(plain_, 0x74, (iv32() & 0x7FFFFFFFu) | (v ? 0x80000000u : 0u));
    }
    void setExperience(uint32_t v) noexcept { write32(plain_, 0x10, v); }
    // Party-only cached battle data: level byte 0xEC, current HP 0xF0, then
    // HP max/Atk/Def/Spe/SpA/SpD at 0xF2..0xFD (PKHeX PK6.cs, PKSM-Core PK6::partyStat).
    void setPartyStats(uint8_t level, uint16_t currentHp, const std::array<uint16_t, 6>& stats) noexcept {
        if (!partyForm()) return;
        plain_[0xEC] = level;
        write16(plain_, 0xF0, currentHp);
        for (size_t i = 0; i < 6; ++i) write16(plain_, 0xF2 + 2 * i, stats[i]);
    }
    void setOriginalTrainerFriendship(uint8_t v) noexcept { plain_[0xCA] = v; }
    void setBall(uint8_t v) noexcept { plain_[0xDC] = v; }
    // PK7-ONLY (PKHeX PK7.cs HyperTrainFlags 0xDE). In PK6 the same byte is the Gen IV encounter type, so
    // Gen VI code never calls these; Gen7StagedSession is the only writer.
    [[nodiscard]] uint8_t pk7HyperTrainFlags() const noexcept { return plain_[0xDE]; }
    void setPk7HyperTrainFlags(uint8_t v) noexcept { plain_[0xDE] = v; }
    void setOriginalTrainerGender(uint8_t v) noexcept {
        plain_[0xDD] = static_cast<uint8_t>((plain_[0xDD] & 0x7F) | ((v & 1) << 7));
    }
    // Writes up to 12 UTF-16 units, then zero-fills the rest of the 26-byte
    // span, so the final terminator word (0x58 / 0xC8) stays zero.
    void setRawName(size_t offset, std::u16string_view name) noexcept {
        for (size_t i = 0; i < NameBytes; ++i) plain_[offset + i] = 0;
        for (size_t i = 0; i < name.size() && i < NameCapacity; ++i)
            write16(plain_, offset + 2 * i, static_cast<uint16_t>(name[i]));
    }

private:
    Pokemon6() = default;
    std::vector<uint8_t> plain_;
};

} // namespace PokeVault::Integration::Gen6::PK6

#endif
