#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeBank::UIModel::Gen5LanguageCycle {
// Native PK5 language values, in the same order as the existing picker.
// ID 6 is unused; never stage it by pressing A.
inline constexpr std::array<uint8_t,7> kSupported{{1,2,3,4,5,7,8}};

[[nodiscard]] constexpr bool supported(uint8_t value) noexcept {
    for (const auto language : kSupported)
        if (language == value) return true;
    return false;
}

// Advance exactly one place, wrapping Korean -> Japanese. An unknown preserved
// native value is shown until the owner first chooses to cycle, then starts at
// Japanese. Calling this never modifies a record or save.
[[nodiscard]] constexpr uint8_t next(uint8_t current) noexcept {
    for (std::size_t i=0; i<kSupported.size(); ++i)
        if (kSupported[i] == current) return kSupported[(i+1)%kSupported.size()];
    return kSupported.front();
}
} // namespace PokeBank::UIModel::Gen5LanguageCycle
