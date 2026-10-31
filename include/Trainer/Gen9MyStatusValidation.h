#ifndef POKEBANK_TRAINER_GEN9_MY_STATUS_VALIDATION_H
#define POKEBANK_TRAINER_GEN9_MY_STATUS_VALIDATION_H

#include <cstddef>

namespace Trainer::Gen9MyStatus {

inline constexpr std::size_t kCoreFieldBytes = 0x06;

constexpr bool hasCoreFields(std::size_t size) noexcept {
    return size >= kCoreFieldBytes;
}

} // namespace Trainer::Gen9MyStatus

#endif
