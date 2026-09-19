#pragma once

#include "UI/ExactFormatEditorProvider.h"

#include <optional>
#include <string_view>

namespace PokeVault::Integration::Gen3EditorProvider {

namespace Exact = PokeBank::UIModel::ExactFormatEditor;

constexpr bool isGen3GbaId(std::string_view id) noexcept {
    return id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba" ||
           id == "firered_gba" || id == "leafgreen_gba";
}

// This first device candidate deliberately does not invent a Gen III learnset oracle.
// Existing source moves are preservable; selecting a new move remains unsupported until
// an exact game+species+form learnset provider is added and tested.
inline Exact::MoveCompatibilityResult evaluateMove(
    const Exact::MoveCompatibilityQuery& query) noexcept {
    if (!isGen3GbaId(query.exactGameId) || query.species == 0 || query.species > 386 ||
        query.move > 354)
        return Exact::MoveCompatibilityResult::Invalid;
    if (query.move == 0) return Exact::MoveCompatibilityResult::Compatible;
    return query.existingSourceMove ? Exact::MoveCompatibilityResult::PreserveExisting
                                    : Exact::MoveCompatibilityResult::Unsupported;
}

constexpr PokeVault::SaveEdit::Capabilities stagedPokemonCapabilities() noexcept {
    PokeVault::SaveEdit::Capabilities caps;
    caps.add(PokeVault::SaveEdit::Capability::BoxPokemon)
        .add(PokeVault::SaveEdit::Capability::PokemonEditing)
        .add(PokeVault::SaveEdit::Capability::PokemonCreation)
        .add(PokeVault::SaveEdit::Capability::PokemonClone);
    // PID-correlated shiny transformation is intentionally NOT granted in the
    // first Gen III milestone; Shiny stays READ_ONLY.
    return caps;
}

inline std::optional<Exact::ExactFormatEditorDescriptor> descriptorForSource(
    std::string_view sourceId, bool stagedWorkspaceAvailable) noexcept {
    if (!isGen3GbaId(sourceId)) return std::nullopt;
    const auto exact = PokeBank::UIModel::PokemonEditorFoundation::capabilitiesForSourceId(sourceId);
    if (!exact) return std::nullopt;

    return Exact::ExactFormatEditorDescriptor{
        *exact,
        Exact::StorageSemantics::SparseNative,
        Exact::gen3StatPresentation(),
        {sourceId, evaluateMove},
        {PokeVault::Safety::SourceKind::RetroArchLegacy,
         stagedPokemonCapabilities(), stagedWorkspaceAvailable},
    };
}

} // namespace PokeVault::Integration::Gen3EditorProvider
