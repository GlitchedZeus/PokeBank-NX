#pragma once

#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "UI/ExactFormatEditorProvider.h"

#include <optional>
#include <string_view>

namespace PokeVault::Integration::ClassicEditorProvider {

namespace Exact = PokeBank::UIModel::ExactFormatEditor;

inline std::optional<Gen1::SourceGame> gen1Game(std::string_view gameId) noexcept {
    if (gameId == "red_gb") return Gen1::SourceGame::Red;
    if (gameId == "blue_gb") return Gen1::SourceGame::Blue;
    if (gameId == "yellow_gb") return Gen1::SourceGame::Yellow;
    return std::nullopt;
}

inline std::optional<Gen2::SourceGame> gen2Game(std::string_view gameId) noexcept {
    if (gameId == "gold_gbc") return Gen2::SourceGame::Gold;
    if (gameId == "silver_gbc") return Gen2::SourceGame::Silver;
    if (gameId == "crystal_gbc") return Gen2::SourceGame::Crystal;
    return std::nullopt;
}

inline Exact::MoveCompatibilityResult evaluateGen1Move(
    const Exact::MoveCompatibilityQuery& query) noexcept {
    const auto game = gen1Game(query.exactGameId);
    if (!game || query.species == 0 || query.species > 151 || query.move > 165)
        return Exact::MoveCompatibilityResult::Invalid;
    if (query.move == 0) return Exact::MoveCompatibilityResult::Compatible;
    if (Gen1::MoveCompatibility::canLearnMove(*game, query.species, query.move))
        return Exact::MoveCompatibilityResult::Compatible;
    return query.existingSourceMove ? Exact::MoveCompatibilityResult::PreserveExisting
                                    : Exact::MoveCompatibilityResult::Unsupported;
}

inline Exact::MoveCompatibilityResult evaluateGen2Move(
    const Exact::MoveCompatibilityQuery& query) noexcept {
    const auto game = gen2Game(query.exactGameId);
    if (!game || query.species == 0 || query.species > 251 || query.move > 251)
        return Exact::MoveCompatibilityResult::Invalid;
    if (query.move == 0) return Exact::MoveCompatibilityResult::Compatible;
    if (Gen2::MoveCompatibility::canLearnMove(*game, query.species, query.move))
        return Exact::MoveCompatibilityResult::Compatible;
    return query.existingSourceMove ? Exact::MoveCompatibilityResult::PreserveExisting
                                    : Exact::MoveCompatibilityResult::Unsupported;
}

inline std::optional<Exact::ExactFormatEditorDescriptor> descriptorForAcceptedClassicSource(
    std::string_view sourceId,
    PokeVault::SaveEdit::Capabilities saveOperations,
    PokeVault::Safety::SourceKind sourceKind,
    bool stagedWorkspaceAvailable) noexcept {
    if (gen1Game(sourceId)) {
        return Exact::descriptorForAcceptedClassicSource(
            sourceId, saveOperations, sourceKind, stagedWorkspaceAvailable, evaluateGen1Move);
    }
    if (gen2Game(sourceId)) {
        return Exact::descriptorForAcceptedClassicSource(
            sourceId, saveOperations, sourceKind, stagedWorkspaceAvailable, evaluateGen2Move);
    }
    return std::nullopt;
}

} // namespace PokeVault::Integration::ClassicEditorProvider
