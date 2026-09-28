#pragma once

#include "UI/ExactFormatEditorProvider.h"
#include "Enums/GameVersion.h"

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
    return caps;
}

inline Enums::GameVersion groupForSourceId(std::string_view id) noexcept {
    if (id == "diamond_nds" || id == "pearl_nds") return Enums::GameVersion::DP;
    if (id == "platinum_nds") return Enums::GameVersion::PT;
    if (id == "heartgold_nds" || id == "soulsilver_nds") return Enums::GameVersion::HGSS;
    return Enums::GameVersion::Invalid;
}

inline Exact::MoveCompatibilityResult evaluateMove(
    const Exact::MoveCompatibilityQuery& query) noexcept {
    if (!isGen4NdsId(query.exactGameId) || query.species == 0 || query.species > 493)
        return Exact::MoveCompatibilityResult::Invalid;
    if (query.move == 0) return Exact::MoveCompatibilityResult::Compatible;
    const auto group = groupForSourceId(query.exactGameId);
    if (group == Enums::GameVersion::Invalid)
        return Exact::MoveCompatibilityResult::Invalid;
    // Native Generation IV move IDs are 1-467. Species encounter/learnset legality is a
    // separate advisory concern and must not be confused with structural PK4 representability.
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
