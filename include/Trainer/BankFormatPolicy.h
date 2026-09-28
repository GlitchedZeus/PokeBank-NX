#ifndef TRAINER_BANK_FORMAT_POLICY_H
#define TRAINER_BANK_FORMAT_POLICY_H

#include <cstdint>

namespace Trainer::BankFormatPolicy {

inline constexpr std::uint32_t currentBoxCount = 100;
inline constexpr std::uint32_t maximumReasonableBoxCount = 4096;

enum class Disposition : std::uint8_t {
    Supported,
    MigrationRequired,
    Invalid,
};

[[nodiscard]] constexpr Disposition classifyBoxCount(std::uint32_t fileBoxes) noexcept {
    if (fileBoxes == 0 || fileBoxes > maximumReasonableBoxCount)
        return Disposition::Invalid;
    if (fileBoxes > currentBoxCount)
        return Disposition::MigrationRequired;
    return Disposition::Supported;
}

} // namespace Trainer::BankFormatPolicy

#endif
