#ifndef SAVE_BDSP_READ_VALIDATION_H
#define SAVE_BDSP_READ_VALIDATION_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Utils/MD5.h"

namespace PokeBank::SaveValidation::BDSP {

// SAV8BDSP v1.x is a fixed-offset flat blob. The whole-file MD5 field is the
// highest fixed region currently consumed by Trainer8BDSP, so a buffer shorter
// than hashOffset + hashBytes cannot safely enter any parser.
inline constexpr std::size_t partyCountOffset = 0x148A8;
inline constexpr std::size_t romCodeOffset     = 0x79BDF;
inline constexpr std::size_t hashOffset        = 0xE9818;
inline constexpr std::size_t hashBytes         = 16;
inline constexpr std::size_t minimumLayoutBytes = hashOffset + hashBytes;

[[nodiscard]] constexpr bool hasMinimumLayout(std::size_t bytes) noexcept {
    return bytes >= minimumLayoutBytes;
}

[[nodiscard]] inline bool wholeFileHashValid(std::span<const uint8_t> bytes) {
    if (!hasMinimumLayout(bytes.size())) return false;

    std::array<uint8_t, hashBytes> stored{};
    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(hashOffset),
                hashBytes, stored.begin());

    std::vector<uint8_t> probe(bytes.begin(), bytes.end());
    std::fill_n(probe.begin() + static_cast<std::ptrdiff_t>(hashOffset),
                hashBytes, uint8_t{0});

    std::array<uint8_t, hashBytes> computed{};
    Utils::md5(probe.data(), probe.size(), computed.data());
    return computed == stored;
}

} // namespace PokeBank::SaveValidation::BDSP

#endif
