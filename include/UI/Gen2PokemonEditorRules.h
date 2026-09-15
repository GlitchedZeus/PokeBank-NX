#pragma once

#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2HeldItems.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::Gen2PokemonEditor {

namespace Gen2 = PokeVault::Integration::Gen2;

enum class CompatibilityState : uint8_t {
    Ok,
    UnusualPreserved,
    NeedsCorrection,
};

constexpr const char* compatibilityText(CompatibilityState state) noexcept {
    switch (state) {
        case CompatibilityState::Ok: return "OK";
        case CompatibilityState::UnusualPreserved: return "Unusual preserved";
        case CompatibilityState::NeedsCorrection: return "Needs correction";
    }
    return "Needs correction";
}

constexpr CompatibilityState passiveViewCompatibility(bool compatible) noexcept {
    return compatible ? CompatibilityState::Ok : CompatibilityState::UnusualPreserved;
}

constexpr CompatibilityState editMoveCompatibility(bool compatible,
                                                     bool exactMovePreservationCase) noexcept {
    if (compatible) return CompatibilityState::Ok;
    return exactMovePreservationCase
        ? CompatibilityState::UnusualPreserved
        : CompatibilityState::NeedsCorrection;
}

constexpr CompatibilityState createMoveCompatibility(bool compatible) noexcept {
    return compatible ? CompatibilityState::Ok : CompatibilityState::NeedsCorrection;
}

constexpr bool mayCommit(CompatibilityState state) noexcept {
    return state != CompatibilityState::NeedsCorrection;
}

inline std::optional<Gen2::SourceGame> exactGameFromSourceId(std::string_view id) noexcept {
    if (id == "gold_gbc") return Gen2::SourceGame::Gold;
    if (id == "silver_gbc") return Gen2::SourceGame::Silver;
    if (id == "crystal_gbc") return Gen2::SourceGame::Crystal;
    return std::nullopt;
}

inline bool allMovesCompatible(Gen2::SourceGame game, uint16_t species,
                               const std::array<uint8_t, 4>& moves) noexcept {
    return !Gen2::MoveCompatibility::firstIncompatible(game, species, moves).has_value();
}

inline bool usableHeldItem(uint8_t item) noexcept {
    return Gen2::selectableHeldItem(item);
}

inline std::vector<uint8_t> heldItemChoices() {
    std::vector<uint8_t> result;
    result.reserve(192);
    result.push_back(0);
    for (unsigned item = 1; item <= 255; ++item) {
        if (usableHeldItem(static_cast<uint8_t>(item)))
            result.push_back(static_cast<uint8_t>(item));
    }
    return result;
}

constexpr const char* growthGroupName(uint8_t group) noexcept {
    switch (group) {
        case 0: return "Medium Fast";
        case 1: return "Erratic";
        case 2: return "Fluctuating";
        case 3: return "Medium Slow";
        case 4: return "Fast";
        case 5: return "Slow";
        default: return "Unknown";
    }
}

} // namespace PokeBank::UIModel::Gen2PokemonEditor
