#include "Encryption/Encryption4.h"

#include "Encryption/Encryption.h"

#include <algorithm>
#include <cstring>

namespace Encryption {
namespace {
    uint16_t read16(const std::byte* bytes) noexcept {
        const auto* p = reinterpret_cast<const uint8_t*>(bytes);
        return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
    }
    uint32_t read32(const std::byte* bytes) noexcept {
        const auto* p = reinterpret_cast<const uint8_t*>(bytes);
        return static_cast<uint32_t>(p[0]) |
               (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) |
               (static_cast<uint32_t>(p[3]) << 24);
    }

    void shuffle4(std::span<const std::byte> input, std::span<std::byte> output,
                  uint32_t shuffle, bool invert) {
        constexpr size_t start = 8;
        const size_t end = start + SIZE_BLOCK4 * BLOCK_COUNT4;
        std::memcpy(output.data(), input.data(), start);
        if (end < input.size())
            std::memcpy(output.data() + end, input.data() + end, input.size() - end);

        const size_t table = static_cast<size_t>(shuffle & 31u) * BLOCK_COUNT4;
        for (size_t block = 0; block < BLOCK_COUNT4; ++block) {
            const size_t other = start + SIZE_BLOCK4 * blockPosition[table + block];
            const size_t natural = start + SIZE_BLOCK4 * block;
            const size_t source = invert ? natural : other;
            const size_t destination = invert ? other : natural;
            std::memcpy(output.data() + destination, input.data() + source, SIZE_BLOCK4);
        }
    }

    void cryptWords(std::span<std::byte> data, uint32_t seed) noexcept {
        for (size_t offset = 0; offset + 1 < data.size(); offset += 2) {
            seed = seed * 0x41C64E6Du + 0x6073u;
            const uint16_t mask = static_cast<uint16_t>(seed >> 16);
            auto* p = reinterpret_cast<uint8_t*>(data.data() + offset);
            uint16_t value = static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
            value ^= mask;
            p[0] = static_cast<uint8_t>(value);
            p[1] = static_cast<uint8_t>(value >> 8);
        }
    }

    void crypt4(std::span<std::byte> bytes, uint32_t pid, uint16_t checksum) {
        cryptWords(bytes.subspan(8, SIZE_BLOCK4 * BLOCK_COUNT4), checksum);
        if (bytes.size() > SIZE_STORED4)
            cryptWords(bytes.subspan(SIZE_STORED4), pid);
    }
}

bool validRecordSize4(size_t size) noexcept {
    return size == SIZE_STORED4 || size == SIZE_PARTY4;
}

uint16_t checksum4(std::span<const std::byte> decrypted) noexcept {
    if (decrypted.size() < SIZE_STORED4) return 0;
    uint32_t sum = 0;
    for (size_t offset = 8; offset < SIZE_STORED4; offset += 2)
        sum += read16(decrypted.data() + offset);
    return static_cast<uint16_t>(sum);
}

std::vector<std::byte> decryptArray4(std::span<const std::byte> encrypted) {
    if (!validRecordSize4(encrypted.size())) return {};
    const uint32_t pid = read32(encrypted.data());
    const uint16_t checksum = read16(encrypted.data() + 6);

    std::vector<std::byte> scratch(encrypted.begin(), encrypted.end());
    crypt4(scratch, pid, checksum);

    std::vector<std::byte> result(encrypted.size());
    shuffle4(scratch, result, (pid >> 13) & 31u, false);
    return result;
}

std::vector<std::byte> encryptArray4(std::span<const std::byte> decrypted) {
    if (!validRecordSize4(decrypted.size())) return {};
    const uint32_t pid = read32(decrypted.data());
    // This operates on a copy. Parsing never repairs a source record.
    const uint16_t checksum = checksum4(decrypted);
    std::vector<std::byte> refreshed(decrypted.begin(), decrypted.end());
    refreshed[6] = static_cast<std::byte>(checksum);
    refreshed[7] = static_cast<std::byte>(checksum >> 8);
    std::vector<std::byte> shuffled(decrypted.size());
    shuffle4(refreshed, shuffled, (pid >> 13) & 31u, true);
    crypt4(shuffled, pid, checksum);
    return shuffled;
}

std::vector<std::byte> blankRecord4(size_t size) {
    if (!validRecordSize4(size)) return {};
    std::vector<std::byte> zero(size, std::byte{0});
    return encryptArray4(zero);
}
}
