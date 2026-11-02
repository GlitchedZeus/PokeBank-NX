#pragma once

#include "Integration/Encounter/EncounterGuardrails.h"
#include "Pokemon/SpeciesChangeExperience.h"

#include <cstdint>
#include <string_view>

namespace PokeBank::UIModel::SpeciesChangeLevelPolicy {

// The default level below is for a CREATE draft only. Round 13 owner decision 9: an EXISTING
// Pokemon keeps its current level on a species change and its EXP is reconciled to the new
// species' growth rate by Pokemon/SpeciesChangeExperience.h (keepLevel), in every generation.

// Deterministic fallback for an exact game/species whose encounter provider is
// not populated yet. It is never inherited from the previous species.
inline constexpr uint8_t fallbackLevel = 5;

inline uint8_t defaultLevel(std::string_view sourceGameId, uint16_t species) noexcept {
    const auto level =
        PokeVault::Integration::EncounterGuardrails::minimumLevel(sourceGameId, species);
    return level.value_or(fallbackLevel);
}

} // namespace PokeBank::UIModel::SpeciesChangeLevelPolicy
