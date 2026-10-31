#ifndef ENCRYPTION_ENCRYPTION4_H
#define ENCRYPTION_ENCRYPTION4_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Encryption {
    inline constexpr size_t SIZE_STORED4 = 0x88;
    inline constexpr size_t SIZE_PARTY4 = 0xEC;
    inline constexpr size_t SIZE_BLOCK4 = 0x20;
    inline constexpr size_t BLOCK_COUNT4 = 4;

    [[nodiscard]] bool validRecordSize4(size_t size) noexcept;
    [[nodiscard]] uint16_t checksum4(std::span<const std::byte> decrypted) noexcept;
    [[nodiscard]] std::vector<std::byte> decryptArray4(std::span<const std::byte> encrypted);
    [[nodiscard]] std::vector<std::byte> encryptArray4(std::span<const std::byte> decrypted);
    [[nodiscard]] std::vector<std::byte> blankRecord4(size_t size);
}

#endif
