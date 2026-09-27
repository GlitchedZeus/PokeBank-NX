#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Pokemon {
inline constexpr std::size_t GEN3_STORED_RECORD_SIZE = 80;
inline constexpr std::size_t GEN3_PARTY_RECORD_SIZE = 100;
inline constexpr std::size_t GEN3_PARTY_MAIL_ID_OFFSET = 0x55;
inline constexpr uint8_t GEN3_NO_MAIL_ID = 0xFF;
struct Gen3PartyDerivedStats {
    uint8_t level = 0;
    uint16_t hp = 0, attack = 0, defense = 0, speed = 0, specialAttack = 0, specialDefense = 0;
};
inline void writeGen3Party16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) noexcept {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
inline bool hasStoredGen3PartyTail(std::span<const uint8_t> bytes,
                                   std::size_t sourceByteCount) noexcept {
    if (sourceByteCount < GEN3_PARTY_RECORD_SIZE || bytes.size() < GEN3_PARTY_RECORD_SIZE) return false;
    return std::any_of(bytes.begin() + GEN3_STORED_RECORD_SIZE,
                       bytes.begin() + GEN3_PARTY_RECORD_SIZE,
                       [](uint8_t value) { return value != 0; });
}
inline void initializeGen3PartyTail(std::span<uint8_t> bytes,
                                    const Gen3PartyDerivedStats& stats) noexcept {
    if (bytes.size() < GEN3_PARTY_RECORD_SIZE) return;
    bytes[0x50] = 0;
    bytes[0x54] = stats.level;
    bytes[GEN3_PARTY_MAIL_ID_OFFSET] = GEN3_NO_MAIL_ID;
    writeGen3Party16(bytes,0x56,stats.hp); writeGen3Party16(bytes,0x58,stats.hp);
    writeGen3Party16(bytes,0x5A,stats.attack); writeGen3Party16(bytes,0x5C,stats.defense);
    writeGen3Party16(bytes,0x5E,stats.speed); writeGen3Party16(bytes,0x60,stats.specialAttack);
    writeGen3Party16(bytes,0x62,stats.specialDefense);
}
inline void makeEmptyGen3PartySlot(std::span<uint8_t> bytes) noexcept {
    std::fill(bytes.begin(), bytes.end(), 0);
    if (bytes.size() > GEN3_PARTY_MAIL_ID_OFFSET) bytes[GEN3_PARTY_MAIL_ID_OFFSET] = GEN3_NO_MAIL_ID;
}
} // namespace Pokemon
