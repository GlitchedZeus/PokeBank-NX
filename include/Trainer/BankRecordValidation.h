#ifndef TRAINER_BANK_RECORD_VALIDATION_H
#define TRAINER_BANK_RECORD_VALIDATION_H

#include <cstdint>

namespace Trainer::BankRecordValidation {

inline bool accept(std::uint16_t species,
                   std::uint16_t storedChecksum,
                   std::uint16_t calculatedChecksum) noexcept {
    return species != 0 && storedChecksum == calculatedChecksum;
}

} // namespace Trainer::BankRecordValidation

#endif
