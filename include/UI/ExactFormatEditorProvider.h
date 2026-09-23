#pragma once

#include "Safety/SourceMutationPolicy.h"
#include "Save/EditableSaveCapabilities.h"
#include "UI/ExactSaveCapabilities.h"
#include "UI/SharedPokemonEditorContract.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace PokeBank::UIModel::ExactFormatEditor {

using FieldIdentity = SharedPokemonEditor::FieldIdentity;

enum class FieldState : uint8_t { Hidden, Derived, ReadOnly, Editable };

constexpr FieldState fieldStateFrom(SharedPokemonEditor::FieldAccess access) noexcept {
    switch (access) {
        case SharedPokemonEditor::FieldAccess::Hidden: return FieldState::Hidden;
        case SharedPokemonEditor::FieldAccess::Derived: return FieldState::Derived;
        case SharedPokemonEditor::FieldAccess::ReadOnly: return FieldState::ReadOnly;
        case SharedPokemonEditor::FieldAccess::Editable: return FieldState::Editable;
    }
    return FieldState::Hidden;
}

constexpr SharedPokemonEditor::Generation sharedGeneration(
    PokemonEditorFoundation::Generation generation) noexcept {
    using G = PokemonEditorFoundation::Generation;
    using S = SharedPokemonEditor::Generation;
    switch (generation) {
        case G::Gen1: return S::Gen1;
        case G::Gen2: return S::Gen2;
        case G::Gen3: return S::Gen3;
        case G::Gen4: return S::Gen4;
        case G::Gen5: return S::Gen5;
        case G::Gen6: return S::Gen6;
        case G::Gen7: return S::Gen7;
        case G::Gen8: return S::Gen8;
        case G::Gen9: return S::Gen9;
    }
    return S::Gen1;
}

enum class StorageSemantics : uint8_t {
    PackedNative,
    SparseNative,
    AppOwnedSparse,
    AdapterDefined,
};

enum class GeneticValueModel : uint8_t { DV, IV, AdapterDefined };
enum class TrainingValueModel : uint8_t { StatExperience, EV, AdapterDefined };

struct StatPresentationSchema {
    std::array<std::string_view, 6> battleLabels{};
    uint8_t battleStatCount = 0;
    uint8_t storedGeneticValueCount = 0;
    uint8_t derivedGeneticValueCount = 0;
    uint8_t trainingValueCount = 0;
    GeneticValueModel geneticModel = GeneticValueModel::AdapterDefined;
    TrainingValueModel trainingModel = TrainingValueModel::AdapterDefined;
    bool hpGeneticValueDerived = false;
    bool hasSingleNativeSpecial = false;
    bool presentsSplitSpecialBattleStats = false;
};

constexpr StatPresentationSchema gen3StatPresentation() noexcept {
    return {{{"HP", "Attack", "Defense", "Speed", "Sp. Atk", "Sp. Def"}},
            6, 6, 0, 6, GeneticValueModel::IV, TrainingValueModel::EV,
            false, false, true};
}

enum class MoveCompatibilityResult : uint8_t {
    Compatible,
    PreserveExisting,
    Unsupported,
    Invalid,
};

struct MoveCompatibilityQuery {
    std::string_view exactGameId;
    uint16_t species = 0;
    uint8_t form = 0;
    uint16_t move = 0;
    bool existingSourceMove = false;
};

using MoveCompatibilityEvaluator =
    MoveCompatibilityResult (*)(const MoveCompatibilityQuery&) noexcept;

struct MoveCompatibilityProvider {
    std::string_view exactGameId;
    MoveCompatibilityEvaluator evaluator = nullptr;

    constexpr bool accepts(const MoveCompatibilityQuery& query) const noexcept {
        return !exactGameId.empty() && query.exactGameId == exactGameId;
    }

    MoveCompatibilityResult evaluate(const MoveCompatibilityQuery& query) const noexcept {
        if (!accepts(query)) return MoveCompatibilityResult::Invalid;
        if (!evaluator) return MoveCompatibilityResult::Unsupported;
        return evaluator(query);
    }
};

struct SourceCapabilityBridge {
    PokeVault::Safety::SourceKind sourceKind = PokeVault::Safety::SourceKind::InstalledGame;
    PokeVault::SaveEdit::Capabilities saveOperations{};
    bool stagedWorkspaceAvailable = false;

    constexpr bool canEditStagedField(FieldState state) const noexcept {
        return stagedWorkspaceAvailable && state == FieldState::Editable &&
            saveOperations.supports(PokeVault::SaveEdit::Capability::PokemonEditing);
    }

    constexpr bool canWriteOriginalSource() const noexcept { return false; }
    constexpr bool sourcePolicyWouldAllowDirectEdit() const noexcept {
        return PokeVault::Safety::canPerform(
            sourceKind, PokeVault::Safety::SourceMutation::Edit);
    }
};

struct ExactFormatEditorDescriptor {
    PokemonEditorFoundation::ExactSaveCapabilities exact;
    StorageSemantics storage = StorageSemantics::AdapterDefined;
    StatPresentationSchema stats{};
    MoveCompatibilityProvider moves{};
    SourceCapabilityBridge source{};

    constexpr FieldState fieldState(FieldIdentity field) const noexcept {
        return fieldStateFrom(SharedPokemonEditor::fieldAccessForGeneration(
            sharedGeneration(exact.identity.generation), field,
            exact.supportsCrystalCaughtData));
    }

    constexpr bool fieldVisible(FieldIdentity field) const noexcept {
        return fieldState(field) != FieldState::Hidden;
    }

    constexpr bool fieldIsEditorTarget(FieldIdentity field) const noexcept {
        return source.canEditStagedField(fieldState(field));
    }
};

} // namespace PokeBank::UIModel::ExactFormatEditor
