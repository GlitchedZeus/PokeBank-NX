#ifndef POKEBANK_ENCRYPTION5_H
#define POKEBANK_ENCRYPTION5_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace PokeVault::Integration::Gen5::Crypto {

// Gen V PK5: 136 bytes in PC boxes, 220 bytes in party; both have four
// 32-byte shuffled blocks. This deliberately does not reuse Gen IV's 236-byte
// party geometry. References: PKHeX PokeCrypto.{Decrypt45,Encrypt45} and PK5.
inline constexpr size_t StoredSize = 136;
inline constexpr size_t PartySize = 220;
inline constexpr size_t DataStart = 8;
inline constexpr size_t BlockSize = 32;

inline bool recordSizeValid(size_t size) noexcept {
    return size == StoredSize || size == PartySize;
}

inline uint16_t read16(std::span<const uint8_t> bytes, size_t pos) noexcept {
    return pos + 1 < bytes.size()
        ? static_cast<uint16_t>(bytes[pos] | (static_cast<uint16_t>(bytes[pos+1]) << 8))
        : 0;
}
inline uint32_t read32(std::span<const uint8_t> bytes, size_t pos) noexcept {
    return static_cast<uint32_t>(read16(bytes, pos)) |
           (static_cast<uint32_t>(read16(bytes, pos+2)) << 16);
}
inline void write16(std::span<uint8_t> bytes, size_t pos, uint16_t v) noexcept {
    bytes[pos] = static_cast<uint8_t>(v);
    bytes[pos+1] = static_cast<uint8_t>(v >> 8);
}

// Same 24 possible block orders as PKHeX PokeCrypto.Shuffle45.
// Shuffle values 24..31 repeat 0..7.
inline constexpr std::array<std::array<uint8_t, 4>, 24> BlockOrder {{
    {{0,1,2,3}}, {{0,1,3,2}}, {{0,2,1,3}}, {{0,3,1,2}},
    {{0,2,3,1}}, {{0,3,2,1}}, {{1,0,2,3}}, {{1,0,3,2}},
    {{2,0,1,3}}, {{3,0,1,2}}, {{2,0,3,1}}, {{3,0,2,1}},
    {{1,2,0,3}}, {{1,3,0,2}}, {{2,1,0,3}}, {{3,1,0,2}},
    {{2,3,0,1}}, {{3,2,0,1}}, {{1,2,3,0}}, {{1,3,2,0}},
    {{2,1,3,0}}, {{3,1,2,0}}, {{2,3,1,0}}, {{3,2,1,0}}
}};

inline void cryptWords(std::span<uint8_t> bytes, uint32_t seed) noexcept {
    for (size_t pos = 0; pos < bytes.size(); pos += 2) {
        seed = seed * 0x41C64E6Du + 0x6073u;
        write16(bytes, pos, static_cast<uint16_t>(read16(bytes, pos) ^ (seed >> 16)));
    }
}
inline uint16_t checksum(std::span<const uint8_t> decrypted) noexcept {
    if (!recordSizeValid(decrypted.size())) return 0;
    uint32_t sum = 0;
    for (size_t pos = DataStart; pos < StoredSize; pos += 2)
        sum += read16(decrypted, pos);
    return static_cast<uint16_t>(sum);
}

// Pure record transform. Source bytes and any emulator save are immutable.
inline std::vector<uint8_t> decrypt(std::span<const uint8_t> encrypted) {
    if (!recordSizeValid(encrypted.size())) return {};
    // Empty physical slots can be all-zero, not an encrypted blank PK5.
    if (std::all_of(encrypted.begin(), encrypted.end(), [](uint8_t b){ return b == 0; }))
        return {encrypted.begin(), encrypted.end()};

    const uint32_t pid = read32(encrypted, 0);
    const uint16_t seed = read16(encrypted, 6);
    std::vector<uint8_t> scratch(encrypted.begin(), encrypted.end());
    cryptWords(std::span<uint8_t>(scratch).subspan(DataStart, 4*BlockSize), seed);
    if (scratch.size() == PartySize)
        cryptWords(std::span<uint8_t>(scratch).subspan(StoredSize), pid);
    std::vector<uint8_t> result(scratch);
    const auto& order = BlockOrder[((pid >> 13) & 31u) % BlockOrder.size()];
    for (size_t block = 0; block < 4; ++block)
        std::copy_n(scratch.begin() + static_cast<std::ptrdiff_t>(DataStart + order[block]*BlockSize),
                    BlockSize, result.begin() + static_cast<std::ptrdiff_t>(DataStart + block*BlockSize));
    return result;
}

// Candidate-generation primitive, NOT a save writer. Only used with app-owned
// or synthetic records. Refreshes PK5 checksum before encrypting.
inline std::vector<uint8_t> encryptCandidate(std::span<const uint8_t> decrypted) {
    if (!recordSizeValid(decrypted.size())) return {};
    std::vector<uint8_t> result(decrypted.begin(), decrypted.end());
    const uint32_t pid = read32(result, 0);
    const uint16_t sum = checksum(decrypted);
    write16(result, 6, sum);
    std::vector<uint8_t> reordered(result);
    const auto& order = BlockOrder[((pid >> 13) & 31u) % BlockOrder.size()];
    for (size_t block = 0; block < 4; ++block)
        std::copy_n(result.begin() + static_cast<std::ptrdiff_t>(DataStart + block*BlockSize),
                    BlockSize, reordered.begin() + static_cast<std::ptrdiff_t>(DataStart + order[block]*BlockSize));
    cryptWords(std::span<uint8_t>(reordered).subspan(DataStart, 4*BlockSize), sum);
    if (reordered.size() == PartySize)
        cryptWords(std::span<uint8_t>(reordered).subspan(StoredSize), pid);
    return reordered;
}

} // namespace PokeVault::Integration::Gen5::Crypto
#endif
