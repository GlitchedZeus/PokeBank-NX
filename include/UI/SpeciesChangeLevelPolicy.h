#pragma once

#include "Integration/Encounter/EncounterGuardrails.h"

#include <cstdint>
#include <string_view>

namespace PokeBank::UIModel::SpeciesChangeLevelPolicy {

// Deterministic fallback for an exact game/species whose encounter provider is
// not populated yet. It is never inherited from the previous species.
inline constexpr uint8_t fallbackLevel = 5;

inline uint8_t defaultLevel(std::string_view sourceGameId, uint16_t species) noexcept {
    const auto level =
        PokeVault::Integration::EncounterGuardrails::minimumLevel(sourceGameId, species);
    return level.value_or(fallbackLevel);
}

} // namespace PokeBank::UIModel::SpeciesChangeLevelPolicy
