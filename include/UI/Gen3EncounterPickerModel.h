#pragma once

#include "Integration/Encounter/EncounterGuardrails.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace PokeBank::UIModel::Gen3EncounterPicker {

namespace Encounter = PokeVault::Integration::EncounterGuardrails;

inline std::vector<uint16_t> rowValues(std::size_t count) {
    std::vector<uint16_t> rows;
    rows.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
        rows.push_back(static_cast<uint16_t>(i));
    return rows;
}

template <typename Choice>
inline const Encounter::EncounterTemplate& encounterOf(const Choice& choice) noexcept {
    if constexpr (requires { choice.encounter; }) return choice.encounter;
    else return choice;
}

template <typename Choice>
inline int selectedRow(const std::vector<Choice>& choices,
                       uint16_t currentLocation, uint8_t currentLevel) noexcept {
    int locationFallback = -1;
    for (int i = 0; i < static_cast<int>(choices.size()); ++i) {
        const auto& encounter = encounterOf(choices[static_cast<std::size_t>(i)]);
        if (encounter.location != currentLocation) continue;
        if (locationFallback < 0) locationFallback = i;
        if (currentLevel == 0 || encounter.containsLevel(currentLevel)) return i;
    }
    return locationFallback >= 0 ? locationFallback : 0;
}

template <typename Choice>
inline const Choice* choiceAt(const std::vector<Choice>& choices, uint16_t rowValue) noexcept {
    return rowValue < choices.size() ? &choices[static_cast<std::size_t>(rowValue)] : nullptr;
}

} // namespace PokeBank::UIModel::Gen3EncounterPicker
