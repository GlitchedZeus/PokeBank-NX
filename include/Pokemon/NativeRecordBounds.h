#pragma once
// Shared overflow-safe range checking for native PK3/PK4 field accessors.
// Evaluate the subtraction only after establishing offset <= extent.
#include <cstddef>

namespace Pokemon::NativeRecordBounds {
[[nodiscard]] constexpr bool contains(std::size_t extent,
                                      std::size_t offset,
                                      std::size_t width = 1) noexcept {
    return offset <= extent && width <= extent - offset;
}
} // namespace Pokemon::NativeRecordBounds
