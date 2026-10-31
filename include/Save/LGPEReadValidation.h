#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace Save::LGPEReadValidation {

inline constexpr std::size_t ACTIVE_SIZE = 0xB8800;
inline constexpr std::size_t FULL_FILE_SIZE = 0x100000;
inline constexpr std::size_t CHECKSUM_BASE = 0xB861A;

struct BlockDef {
    uint32_t key;
    std::size_t offset;
    std::size_t size;
};

inline constexpr std::array<BlockDef, 7> CONSUMED_BLOCKS{{
    {0,  0x00000, 0x00D90},
    {2,  0x01000, 0x00168},
    {4,  0x02A00, 0x020E8},
    {5,  0x04C00, 0x00930},
    {8,  0x05A00, 0x00012},
    {9,  0x05C00, 0x3F7A0},
    {10, 0x45400, 0x00008},
}};

inline constexpr bool supportedSize(std::size_t size) noexcept {
    return size == ACTIVE_SIZE || size == FULL_FILE_SIZE;
}

inline std::span<const uint8_t> activeRegion(std::span<const uint8_t> bytes) noexcept {
    if (!supportedSize(bytes.size())) return {};
    return bytes.first(ACTIVE_SIZE);
}

inline uint16_t crc16Arc(std::span<const uint8_t> bytes) noexcept {
    uint16_t crc = 0;
    for (uint8_t value : bytes) {
        crc ^= value;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 1) ? static_cast<uint16_t>((crc >> 1) ^ 0xA001)
                            : static_cast<uint16_t>(crc >> 1);
    }
    return crc;
}

inline uint16_t read16(std::span<const uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<uint16_t>(bytes[offset]) |
           static_cast<uint16_t>(static_cast<uint16_t>(bytes[offset + 1]) << 8);
}

inline std::string_view validate(std::span<const uint8_t> bytes) noexcept {
    const auto active = activeRegion(bytes);
    if (active.empty())
        return "Let's Go save size does not match the supported active/full-file layout";

    for (const auto& block : CONSUMED_BLOCKS) {
        if (block.offset + block.size > active.size())
            return "Let's Go consumed block is truncated";
        const std::size_t checksumOffset =
            CHECKSUM_BASE + static_cast<std::size_t>(block.key) * 8;
        if (checksumOffset + 2 > active.size())
            return "Let's Go checksum footer is truncated";
        const uint16_t stored = read16(active, checksumOffset);
        const uint16_t actual = crc16Arc(active.subspan(block.offset, block.size));
        if (stored != actual)
            return "Let's Go block checksum verification failed";
    }
    return {};
}

} // namespace Save::LGPEReadValidation
