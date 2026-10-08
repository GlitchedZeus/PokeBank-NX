#ifndef POKEBANK_POKEMON5_READ_ONLY_H
#define POKEBANK_POKEMON5_READ_ONLY_H

#include "Encryption/Encryption5.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Low-level PK5 record reader, NOT a complete Gen V SAV reader. The source
// remains byte-identical. Fail closed on invalid length, sanity, checksum or
// impossible Gen V species. No UI/save-source write authority is exposed.
// Field layout cross-checked against PKHeX.Core/PKM/PK5.cs.
class Pokemon5ReadOnly {
public:
    explicit Pokemon5ReadOnly(std::span<const uint8_t> encrypted)
        : source_(encrypted.begin(), encrypted.end()),
          decoded_(Crypto::decrypt(encrypted)) {}

    [[nodiscard]] bool sizeValid() const noexcept {
        return Crypto::recordSizeValid(source_.size()) && decoded_.size() == source_.size();
    }
    [[nodiscard]] bool checksumValid() const noexcept {
        return sizeValid() && Crypto::read16(decoded_, 6) == Crypto::checksum(decoded_);
    }
    [[nodiscard]] bool sanityValid() const noexcept {
        return sizeValid() && Crypto::read16(decoded_, 4) == 0;
    }
    [[nodiscard]] bool valid() const noexcept {
        return sizeValid() && sanityValid() && checksumValid() &&
               Crypto::read16(decoded_, 8) <= 649;
    }
    [[nodiscard]] bool empty() const noexcept { return valid() && species() == 0; }
    [[nodiscard]] bool partyRecord() const noexcept { return source_.size() == Crypto::PartySize; }
    // PKHeX.Core PK5.Stat_Level: party extension byte 0x8C. Boxed PK5
    // does not store current battle level; zero means unavailable.
    [[nodiscard]] uint8_t partyLevel() const noexcept {
        return valid() && partyRecord() ? decoded_[0x8C] : 0;
    }

    [[nodiscard]] std::span<const uint8_t> originalEncryptedBytes() const noexcept { return source_; }
    // Diagnostic access only: semantic field getters below quarantine invalid records.
    [[nodiscard]] std::span<const uint8_t> decodedBytes() const noexcept { return decoded_; }

    [[nodiscard]] uint32_t pid() const noexcept { return valid() ? Crypto::read32(decoded_, 0) : 0; }
    [[nodiscard]] uint16_t species() const noexcept { return u16(0x08); }
    [[nodiscard]] uint16_t heldItem() const noexcept { return u16(0x0A); }
    [[nodiscard]] uint16_t tid() const noexcept { return u16(0x0C); }
    [[nodiscard]] uint16_t sid() const noexcept { return u16(0x0E); }
    [[nodiscard]] uint32_t experience() const noexcept { return u32(0x10); }
    [[nodiscard]] uint8_t friendship() const noexcept { return at(0x14); }
    [[nodiscard]] uint8_t ability() const noexcept { return at(0x15); }
    [[nodiscard]] uint8_t language() const noexcept { return at(0x17); }
    [[nodiscard]] std::array<uint8_t,6> evs() const noexcept {
        return {at(0x18),at(0x19),at(0x1A),at(0x1B),at(0x1C),at(0x1D)};
    }
    [[nodiscard]] std::array<uint16_t,4> moves() const noexcept {
        return {u16(0x28),u16(0x2A),u16(0x2C),u16(0x2E)};
    }
    [[nodiscard]] std::array<uint8_t,4> pp() const noexcept {
        return {at(0x30),at(0x31),at(0x32),at(0x33)};
    }
    [[nodiscard]] std::array<uint8_t,4> ppUps() const noexcept {
        return {at(0x34),at(0x35),at(0x36),at(0x37)};
    }
    [[nodiscard]] std::array<uint8_t,6> ivs() const noexcept {
        const uint32_t bits = u32(0x38);
        return {static_cast<uint8_t>(bits & 31), static_cast<uint8_t>((bits >> 5) & 31),
                static_cast<uint8_t>((bits >> 10) & 31), static_cast<uint8_t>((bits >> 15) & 31),
                static_cast<uint8_t>((bits >> 20) & 31), static_cast<uint8_t>((bits >> 25) & 31)};
    }
    [[nodiscard]] bool egg() const noexcept { return (u32(0x38) & 0x40000000u) != 0; }
    [[nodiscard]] uint8_t gender() const noexcept { return static_cast<uint8_t>((at(0x40) >> 1) & 3); }
    [[nodiscard]] uint8_t form() const noexcept { return at(0x40) >> 3; }
    [[nodiscard]] uint8_t nature() const noexcept { return at(0x41); }
    [[nodiscard]] bool hiddenAbility() const noexcept { return (at(0x42) & 1) != 0; }
    [[nodiscard]] uint8_t originVersion() const noexcept { return at(0x5F); }
    [[nodiscard]] bool shiny() const noexcept {
        if (!valid() || empty()) return false;
        const uint32_t value = pid();
        return (tid() ^ sid() ^ static_cast<uint16_t>(value) ^
                static_cast<uint16_t>(value >> 16)) < 8;
    }
private:
    [[nodiscard]] uint8_t at(size_t pos) const noexcept {
        return valid() && pos < decoded_.size() ? decoded_[pos] : 0;
    }
    [[nodiscard]] uint16_t u16(size_t pos) const noexcept {
        return static_cast<uint16_t>(at(pos) | (static_cast<uint16_t>(at(pos+1)) << 8));
    }
    [[nodiscard]] uint32_t u32(size_t pos) const noexcept {
        return static_cast<uint32_t>(u16(pos)) | (static_cast<uint32_t>(u16(pos+2)) << 16);
    }
    std::vector<uint8_t> source_;
    std::vector<uint8_t> decoded_;
};

} // namespace PokeVault::Integration::Gen5
#endif
