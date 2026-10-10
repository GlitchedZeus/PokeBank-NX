#ifndef POKEBANK_GEN5_EXACT_FORMAT_EDITOR_PROVIDER_H
#define POKEBANK_GEN5_EXACT_FORMAT_EDITOR_PROVIDER_H

#include "UI/ExactFormatEditorProvider.h"

#include <optional>
#include <string_view>

namespace PokeVault::Integration::Gen5EditorProvider {
namespace Exact = PokeBank::UIModel::ExactFormatEditor;

// Backend descriptor only, not a route to a new or separate Pokémon editor.
// Live Gen V game descriptors remain Planned, and no source SAV write exists.
[[nodiscard]] constexpr bool isGen5NdsId(std::string_view id) noexcept {
    return id=="black_nds" || id=="white_nds" ||
           id=="black2_nds" || id=="white2_nds";
}
[[nodiscard]] constexpr PokeVault::SaveEdit::Capabilities stagedFieldCapabilities() noexcept {
    PokeVault::SaveEdit::Capabilities caps;
    caps.add(PokeVault::SaveEdit::Capability::BoxPokemon)
        .add(PokeVault::SaveEdit::Capability::PokemonEditing);
    // No PokemonCreation or direct source editing until separately validated.
    return caps;
}
[[nodiscard]] inline std::optional<Exact::ExactFormatEditorDescriptor> descriptorForSource(
    std::string_view sourceId, bool validatedStagedWorkspace) noexcept {
    if(!isGen5NdsId(sourceId))return std::nullopt;
    const auto capabilities =
        PokeBank::UIModel::PokemonEditorFoundation::capabilitiesForSourceId(sourceId);
    if(!capabilities)return std::nullopt;
    return Exact::ExactFormatEditorDescriptor{
        *capabilities,
        Exact::StorageSemantics::SparseNative,
        Exact::gen3StatPresentation(),
        {sourceId, nullptr}, // Move compatibility is unsupported, never invented.
        {PokeVault::Safety::SourceKind::ExternalLegacy,
         stagedFieldCapabilities(),validatedStagedWorkspace}
    };
}
} // namespace PokeVault::Integration::Gen5EditorProvider
#endif
