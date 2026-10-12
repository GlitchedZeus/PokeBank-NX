#include "Save/LGPEReadValidation.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

namespace {
void write16(std::vector<uint8_t>& bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}

std::vector<uint8_t> makeValid(std::size_t size) {
    using namespace Save::LGPEReadValidation;
    std::vector<uint8_t> bytes(size, 0);
    for (const auto& block : CONSUMED_BLOCKS) {
        const auto active = std::span<const uint8_t>(bytes.data(), ACTIVE_SIZE);
        const uint16_t crc = crc16Arc(active.subspan(block.offset, block.size));
        write16(bytes, CHECKSUM_BASE + static_cast<std::size_t>(block.key) * 8, crc);
    }
    return bytes;
}
}

int main() {
    using namespace Save::LGPEReadValidation;

    auto active = makeValid(ACTIVE_SIZE);
    assert(validate(active).empty());

    auto full = makeValid(FULL_FILE_SIZE);
    full[ACTIVE_SIZE] = 0x5A; // trailing backup/padding is outside Beluga CRC coverage.
    assert(validate(full).empty());

    // AUDIT-018: modifying a consumed block without its footer CRC must fail closed.
    auto corrupt = full;
    corrupt[CONSUMED_BLOCKS[0].offset] ^= 0x80;
    assert(!validate(corrupt).empty());

    // A checksum-footer mutation must also be rejected.
    corrupt = full;
    corrupt[CHECKSUM_BASE + static_cast<std::size_t>(CONSUMED_BLOCKS[4].key) * 8] ^= 0x01;
    assert(!validate(corrupt).empty());

    assert(!validate(std::span<const uint8_t>(full.data(), ACTIVE_SIZE + 1)).empty());

    std::cout << "LGPE pre-open block CRC validation: PASS\n";
    return 0;
}
