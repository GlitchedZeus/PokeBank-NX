#pragma once
#include "UI/SpeciesChangeLevelPolicy.h"

#include <cstdint>

namespace PokeBank::UIModel {

// Compatibility names for older classic-editor call sites. New code must use
// SpeciesChangeLevelPolicy with exact game + species context.
inline constexpr uint8_t classicCreateDefaultLevel =
    SpeciesChangeLevelPolicy::fallbackLevel;

constexpr uint8_t classicSpeciesChangeLevel(bool, uint8_t) noexcept {
    return classicCreateDefaultLevel;
}

} // namespace PokeBank::UIModel
