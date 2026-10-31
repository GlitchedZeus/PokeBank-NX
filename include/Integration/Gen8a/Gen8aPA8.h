#ifndef POKEBANK_GEN8A_PA8_H
#define POKEBANK_GEN8A_PA8_H

// PA8 (Legends: Arceus) record codec. Cited from PKHeX 6501f0ab:
//   PKM/Util/PokeCrypto.cs  SIZE_8ASTORED = 8 + 4 * 0x58 = 0x168, SIZE_8APARTY = 0x178; the same
//                           block-position table and LCRNG stream as PK8, with 0x58-byte blocks;
//                           party stats (0x168..0x177) are crypted with the same seed.
//   PKM/PA8.cs              field offsets below (only those this lane reads or writes).
// The checksum is the u16 sum over 0x08..0x167, stored at 0x06; the sanity word at 0x04 is zero.

#include "Integration/Gen8/Gen8PK8.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen8a::PA8 {

using Gen8::PK8::read16;
using Gen8::PK8::read32;
using Gen8::PK8::write16;
using Gen8::PK8::write32;

inline constexpr size_t StoredSize = 0x168, PartySize = 0x178, BlockSize = 0x58, BlockCount = 4, HeaderSize = 8;
inline constexpr size_t NicknameOffset = 0x60, OriginalTrainerOffset = 0x110, NameBytes = 26, NameCapacity = 12;

[[nodiscard]] constexpr uint32_t shuffleValue(uint32_t ec) noexcept { return ((ec >> 13) & 31) % 24; }

inline void shuffle(std::span<uint8_t> blocks, uint32_t sv) noexcept {
    std::array<uint8_t, BlockSize * BlockCount> copy{};
    for (size_t i = 0; i < copy.size(); ++i) copy[i] = blocks[i];
    for (size_t i = 0; i < BlockCount; ++i) {
        const size_t from = Gen8::PK8::BlockPosition[sv * BlockCount + i];
        for (size_t j = 0; j < BlockSize; ++j) blocks[i * BlockSize + j] = copy[from * BlockSize + j];
    }
}

[[nodiscard]] constexpr bool supportedSize(size_t n) noexcept { return n == StoredSize || n == PartySize; }

[[nodiscard]] inline std::vector<uint8_t> decrypt(std::span<const uint8_t> encrypted) {
    if (!supportedSize(encrypted.size())) return {};
    std::vector<uint8_t> out(encrypted.begin(), encrypted.end());
    const uint32_t ec = read32(out, 0);
    std::span<uint8_t> body(out.data() + HeaderSize, StoredSize - HeaderSize);
    Gen8::PK8::cryptArray(body, ec);
    shuffle(body, shuffleValue(ec));
    if (out.size() > StoredSize) Gen8::PK8::cryptArray(std::span<uint8_t>(out).subspan(StoredSize), ec);
    return out;
}

[[nodiscard]] inline std::vector<uint8_t> encrypt(std::span<const uint8_t> plain) {
    if (!supportedSize(plain.size())) return {};
    std::vector<uint8_t> out(plain.begin(), plain.end());
    const uint32_t ec = read32(out, 0);
    std::span<uint8_t> body(out.data() + HeaderSize, StoredSize - HeaderSize);
    shuffle(body, Gen8::PK8::BlockPositionInvert[shuffleValue(ec)]);
    Gen8::PK8::cryptArray(body, ec);
    if (out.size() > StoredSize) Gen8::PK8::cryptArray(std::span<uint8_t>(out).subspan(StoredSize), ec);
    return out;
}

[[nodiscard]] inline uint16_t checksum(std::span<const uint8_t> plain) noexcept {
    uint16_t sum = 0;
    for (size_t o = HeaderSize; o + 1 < StoredSize; o += 2) sum = static_cast<uint16_t>(sum + read16(plain, o));
    return sum;
}

class Pokemon8a {
public:
    [[nodiscard]] static std::optional<Pokemon8a> decode(std::span<const uint8_t> encrypted) {
        if (!supportedSize(encrypted.size())) return std::nullopt;
        Pokemon8a out;
        out.plain_ = decrypt(encrypted);
        if (read16(out.plain_, 0x04) != 0 || checksum(out.plain_) != read16(out.plain_, 0x06)) return std::nullopt;
        return out;
    }
    [[nodiscard]] static Pokemon8a blank(size_t size) {
        Pokemon8a out;
        out.plain_.assign(supportedSize(size) ? size : StoredSize, 0);
        return out;
    }
    [[nodiscard]] std::span<const uint8_t> plain() const noexcept { return plain_; }
    [[nodiscard]] size_t size() const noexcept { return plain_.size(); }
    [[nodiscard]] std::vector<uint8_t> serialize() const {
        auto copy = plain_;
        write16(copy, 0x06, checksum(copy));
        return encrypt(copy);
    }
    [[nodiscard]] bool empty() const noexcept { return species() == 0; }
    [[nodiscard]] bool allZero() const noexcept {
        for (const uint8_t b : plain_) if (b) return false;
        return true;
    }

    [[nodiscard]] uint32_t encryptionConstant() const noexcept { return read32(plain_, 0x00); }
    [[nodiscard]] uint16_t species() const noexcept { return read16(plain_, 0x08); }
    [[nodiscard]] uint16_t heldItem() const noexcept { return read16(plain_, 0x0A); }
    [[nodiscard]] uint16_t tid() const noexcept { return read16(plain_, 0x0C); }
    [[nodiscard]] uint16_t sid() const noexcept { return read16(plain_, 0x0E); }
    [[nodiscard]] uint32_t experience() const noexcept { return read32(plain_, 0x10); }
    [[nodiscard]] uint16_t ability() const noexcept { return read16(plain_, 0x14); }
    [[nodiscard]] uint8_t abilityNumber() const noexcept { return plain_[0x16] & 7; }
    [[nodiscard]] bool alpha() const noexcept { return (plain_[0x16] & 0x20) != 0; }
    [[nodiscard]] bool noble() const noexcept { return (plain_[0x16] & 0x40) != 0; }
    [[nodiscard]] uint32_t pid() const noexcept { return read32(plain_, 0x1C); }
    [[nodiscard]] uint8_t nature() const noexcept { return plain_[0x20]; }
    [[nodiscard]] uint8_t statNature() const noexcept { return plain_[0x21]; }
    [[nodiscard]] bool fatefulEncounter() const noexcept { return (plain_[0x22] & 1) != 0; }
    [[nodiscard]] uint8_t gender() const noexcept { return (plain_[0x22] >> 2) & 3; }
    [[nodiscard]] uint8_t form() const noexcept { return plain_[0x24]; }
    [[nodiscard]] uint16_t alphaMove() const noexcept { return read16(plain_, 0x3E); }
    [[nodiscard]] uint8_t heightScalar() const noexcept { return plain_[0x50]; }
    [[nodiscard]] uint8_t weightScalar() const noexcept { return plain_[0x51]; }
    [[nodiscard]] uint16_t move(size_t i) const noexcept { return i < 4 ? read16(plain_, 0x54 + 2 * i) : 0; }
    [[nodiscard]] uint16_t currentHp() const noexcept { return read16(plain_, 0x92); }
    [[nodiscard]] uint32_t iv32() const noexcept { return read32(plain_, 0x94); }
    [[nodiscard]] uint8_t iv(size_t s) const noexcept { return s < 6 ? static_cast<uint8_t>((iv32() >> (5 * s)) & 0x1F) : 0; }
    [[nodiscard]] bool isEgg() const noexcept { return ((iv32() >> 30) & 1) != 0; }
    [[nodiscard]] bool isNicknamed() const noexcept { return ((iv32() >> 31) & 1) != 0; }
    [[nodiscard]] uint8_t effortLevel(size_t s) const noexcept { return s < 6 ? plain_[0xA4 + s] : 0; }
    [[nodiscard]] uint8_t currentHandler() const noexcept { return plain_[0xD4]; }
    [[nodiscard]] uint8_t originVersion() const noexcept { return plain_[0xEE]; }
    [[nodiscard]] uint8_t language() const noexcept { return plain_[0xF2]; }
    [[nodiscard]] uint8_t originalTrainerFriendship() const noexcept { return plain_[0x12A]; }
    [[nodiscard]] uint8_t ball() const noexcept { return plain_[0x137]; }
    [[nodiscard]] uint8_t metLevel() const noexcept { return plain_[0x13D] & 0x7F; }
    [[nodiscard]] uint8_t originalTrainerGender() const noexcept { return plain_[0x13D] >> 7; }
    [[nodiscard]] uint8_t statLevel() const noexcept { return plain_.size() == PartySize ? plain_[0x168] : 0; }
    [[nodiscard]] uint16_t stat(size_t i) const noexcept {  // HPmax, Atk, Def, Spe, SpA, SpD
        return plain_.size() == PartySize && i < 6 ? read16(plain_, 0x16A + 2 * i) : 0;
    }
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

    void setHeldItem(uint16_t v) noexcept { write16(plain_, 0x0A, v); }
    void setTid(uint16_t v) noexcept { write16(plain_, 0x0C, v); }
    void setSid(uint16_t v) noexcept { write16(plain_, 0x0E, v); }
    void setFatefulEncounter(bool v) noexcept { plain_[0x22] = static_cast<uint8_t>((plain_[0x22] & ~1) | (v ? 1 : 0)); }
    void setNicknamedFlag(bool v) noexcept { write32(plain_, 0x94, (iv32() & 0x7FFFFFFFu) | (v ? 0x80000000u : 0u)); }
    void setOriginalTrainerFriendship(uint8_t v) noexcept { plain_[0x12A] = v; }
    void setBall(uint8_t v) noexcept { plain_[0x137] = v; }
    void setOriginalTrainerGender(uint8_t v) noexcept {
        plain_[0x13D] = static_cast<uint8_t>((plain_[0x13D] & 0x7F) | ((v & 1) << 7));
    }
    void setRawName(size_t offset, std::u16string_view name) noexcept {
        for (size_t i = 0; i < NameBytes; ++i) plain_[offset + i] = 0;
        for (size_t i = 0; i < name.size() && i < NameCapacity; ++i) write16(plain_, offset + 2 * i, static_cast<uint16_t>(name[i]));
    }

private:
    Pokemon8a() = default;
    std::vector<uint8_t> plain_;
};

} // namespace PokeVault::Integration::Gen8a::PA8

#endif
