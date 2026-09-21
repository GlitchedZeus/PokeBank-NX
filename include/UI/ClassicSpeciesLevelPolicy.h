#pragma once
#include <cstdint>
namespace PokeBank::UIModel {
inline constexpr uint8_t classicCreateDefaultLevel = 5;
constexpr uint8_t classicSpeciesChangeLevel(bool create, uint8_t existingLevel) noexcept {
    return create ? classicCreateDefaultLevel : existingLevel;
}
}
