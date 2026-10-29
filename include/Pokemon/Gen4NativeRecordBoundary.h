#pragma once
// L6: guard the native PK4 entity boundary before allocating or decrypting.
// A transport span can be arbitrarily large; a PK4 entity cannot.
#include "Encryption/Encryption4.h"
#include <cstddef>
#include <span>
#include <vector>

namespace Pokemon::Gen4NativeRecordBoundary {
[[nodiscard]] inline std::vector<std::byte> copyExact(
    std::span<const std::byte> input) {
    if (input.size() != Encryption::SIZE_STORED4 &&
        input.size() != Encryption::SIZE_PARTY4) return {};
    return {input.begin(), input.end()};
}
} // namespace Pokemon::Gen4NativeRecordBoundary
