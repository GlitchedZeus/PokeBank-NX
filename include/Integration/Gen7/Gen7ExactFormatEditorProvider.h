#ifndef POKEBANK_GEN7_EXACT_FORMAT_EDITOR_PROVIDER_H
#define POKEBANK_GEN7_EXACT_FORMAT_EDITOR_PROVIDER_H

#include "Integration/Gen7/Gen7SaveLayout.h"
#include "UI/ExactFormatEditorProvider.h"

#include <optional>
#include <string_view>

// PK7 capability provider for the ONE shared Pokemon editor (no separate UI).
// SAV7 staged export is explicitly unsupported until the MemeCrypto re-sign
// payload is confirmed on a real save (docs/gen7/README.md), so no PK7 field
// is Editable yet: stored fields are ReadOnly, computed ones Derived, and
// non-PK7 concepts Hidden. The shared editor shows PK7 read-only.
namespace PokeVault::Integration::Gen7EditorProvider {
namespace Exact = PokeBank::UIModel::ExactFormatEditor;
using Field = PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;
using State = Exact::FieldState;

inline constexpr bool StagedEditingSupported = false;

[[nodiscard]] constexpr State fieldState(Field field) noexcept {
    switch (field) {
        case Field::Shiny:  // PSV == TSV (xor < 16), as in Gen VI
        case Field::Level:  // EXP + growth rate (party records store the level byte)
            return State::Derived;
        case Field::DV:
        case Field::StatExperience:
        case Field::AV:            // Let's Go only, a different save family
        case Field::EffortLevel:
        case Field::StatNature:
            return State::Hidden;
        default:
            return State::ReadOnly;
    }
}

struct Gen7EditorDescriptor {
    std::string_view exactGameId;
    Exact::StorageSemantics storage = Exact::StorageSemantics::SparseNative;
    Exact::StatPresentationSchema stats = Exact::gen3StatPresentation();
    Exact::MoveCompatibilityProvider moves{};
    Exact::SourceCapabilityBridge source{};
    [[nodiscard]] constexpr State fieldState(Field f) const noexcept { return Gen7EditorProvider::fieldState(f); }
    [[nodiscard]] constexpr bool fieldIsEditorTarget(Field f) const noexcept {
        return source.canEditStagedField(fieldState(f));
    }
};

[[nodiscard]] inline std::optional<Gen7EditorDescriptor> descriptorForSource(std::string_view exactGameId) noexcept {
    if (!Integration::Gen7::Layout::isGen7GameId(exactGameId)) return std::nullopt;
    PokeVault::SaveEdit::Capabilities caps;
    caps.add(PokeVault::SaveEdit::Capability::PartyPokemon).add(PokeVault::SaveEdit::Capability::BoxPokemon);
    Gen7EditorDescriptor d;
    d.exactGameId = exactGameId;
    d.moves = {exactGameId, nullptr};
    d.source = {PokeVault::Safety::SourceKind::ExternalLegacy, caps, /*stagedWorkspaceAvailable*/ false};
    return d;
}

} // namespace PokeVault::Integration::Gen7EditorProvider

#endif
