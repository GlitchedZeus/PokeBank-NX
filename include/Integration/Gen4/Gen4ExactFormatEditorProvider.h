#pragma once

#include "UI/ExactFormatEditorProvider.h"

#include <optional>
#include <string_view>

namespace PokeVault::Integration::Gen4EditorProvider {

namespace Exact = PokeBank::UIModel::ExactFormatEditor;

constexpr bool isGen4NdsId(std::string_view id) noexcept {
    return id == "diamond_nds" || id == "pearl_nds" || id == "platinum_nds" ||
           id == "heartgold_nds" || id == "soulsilver_nds";
}

constexpr PokeVault::SaveEdit::Capabilities stagedPokemonCapabilities() noexcept {
    PokeVault::SaveEdit::Capabilities caps;
    caps.add(PokeVault::SaveEdit::Capability::BoxPokemon)
        .add(PokeVault::SaveEdit::Capability::PokemonEditing)
        .add(PokeVault::SaveEdit::Capability::PokemonCreation);
    // G4-03 boxed/party Edit passed exact CI and owner hardware acceptance at
    // 84dae170...; G4-04 now has a native empty-slot Create transaction.
    return caps;
}

inline Exact::MoveCompatibilityResult evaluateMove(
    const Exact::MoveCompatibilityQuery& query) noexcept {
    if (!isGen4NdsId(query.exactGameId) || query.species == 0 || query.species > 493)
        return Exact::MoveCompatibilityResult::Invalid;
    if (query.move == 0) return Exact::MoveCompatibilityResult::Compatible;
    // PK4 can natively represent the complete Generation IV move set, IDs 1-467.
    // Species learnset legality remains advisory; structural/editor compatibility stops
    // exactly where Generation V begins (Hone Claws = 468).
    if (query.move > 467)
        return query.existingSourceMove ? Exact::MoveCompatibilityResult::PreserveExisting
                                        : Exact::MoveCompatibilityResult::Unsupported;
    return Exact::MoveCompatibilityResult::Compatible;
}

inline std::optional<Exact::ExactFormatEditorDescriptor> descriptorForSource(
    std::string_view sourceId, bool stagedWorkspaceAvailable) noexcept {
    if (!isGen4NdsId(sourceId)) return std::nullopt;
    const auto exact =
        PokeBank::UIModel::PokemonEditorFoundation::capabilitiesForSourceId(sourceId);
    if (!exact) return std::nullopt;

    return Exact::ExactFormatEditorDescriptor{
        *exact,
        Exact::StorageSemantics::SparseNative,
        Exact::gen3StatPresentation(),
        {sourceId, evaluateMove},
        {PokeVault::Safety::SourceKind::ExternalLegacy,
         stagedPokemonCapabilities(), stagedWorkspaceAvailable},
    };
}

} // namespace PokeVault::Integration::Gen4EditorProvider
