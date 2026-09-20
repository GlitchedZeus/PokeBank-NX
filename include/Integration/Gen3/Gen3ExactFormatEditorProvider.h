#pragma once

#include "UI/ExactFormatEditorProvider.h"
#include "Integration/Gen3/Gen3LearnsetTable.h"

#include <optional>
#include <string_view>

namespace PokeVault::Integration::Gen3EditorProvider {

namespace Exact = PokeBank::UIModel::ExactFormatEditor;

constexpr bool isGen3GbaId(std::string_view id) noexcept {
    return id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba" ||
           id == "firered_gba" || id == "leafgreen_gba";
}

inline std::optional<SourceGame> sourceGameForId(std::string_view id) noexcept {
    if (id == "ruby_gba") return SourceGame::RubyGBA;
    if (id == "sapphire_gba") return SourceGame::SapphireGBA;
    if (id == "emerald_gba") return SourceGame::EmeraldGBA;
    if (id == "firered_gba") return SourceGame::FireRedGBA;
    if (id == "leafgreen_gba") return SourceGame::LeafGreenGBA;
    return std::nullopt;
}

// The picker distinguishes native exact-game compatibility from a move that is
// available elsewhere in Generation III. Cross-game moves stay visible as a
// warning because they can be legitimate after a trade; an already-stored move
// outside the generated native pools is preserved rather than silently erased.
inline Exact::MoveCompatibilityResult evaluateMove(
    const Exact::MoveCompatibilityQuery& query) noexcept {
    const auto game = sourceGameForId(query.exactGameId);
    if (!game || query.species == 0 || query.species > 386 || query.move > 354)
        return Exact::MoveCompatibilityResult::Invalid;
    if (query.move == 0) return Exact::MoveCompatibilityResult::Compatible;
    if (Learnset::directlyLearnable(*game, query.species, query.move))
        return Exact::MoveCompatibilityResult::Compatible;
    if (Learnset::learnableSomewhere(query.species, query.move))
        return Exact::MoveCompatibilityResult::Unsupported;
    return query.existingSourceMove ? Exact::MoveCompatibilityResult::PreserveExisting
                                    : Exact::MoveCompatibilityResult::Invalid;
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
