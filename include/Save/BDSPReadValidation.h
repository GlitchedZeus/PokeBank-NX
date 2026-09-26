#ifndef SAVE_BDSP_READ_VALIDATION_H
#define SAVE_BDSP_READ_VALIDATION_H

#include <cstddef>

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

} // namespace PokeBank::SaveValidation::BDSP

#endif
