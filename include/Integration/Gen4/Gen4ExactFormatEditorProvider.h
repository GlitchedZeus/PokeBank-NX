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
        .add(PokeVault::SaveEdit::Capability::PokemonEditing);
    // Create remains deliberately absent until boxed PK4 Edit has passed exact
    // serialize/reparse and owner hardware acceptance.
    return caps;
}

inline Exact::MoveCompatibilityResult evaluateMove(
    const Exact::MoveCompatibilityQuery& query) noexcept {
    if (!isGen4NdsId(query.exactGameId) || query.species == 0 || query.species > 493)
        return Exact::MoveCompatibilityResult::Invalid;
    if (query.move == 0) return Exact::MoveCompatibilityResult::Compatible;
    // G4-03 starts fail-closed: existing native moves are preserved, but a new move
    // is not offered as compatible until an exact-game Gen IV learnset provider is pinned.
    return query.existingSourceMove ? Exact::MoveCompatibilityResult::PreserveExisting
                                    : Exact::MoveCompatibilityResult::Unsupported;
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
