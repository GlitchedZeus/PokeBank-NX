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

enum class FieldState : uint8_t {
    Hidden,
    Derived,
    ReadOnly,
    Editable,
};

constexpr FieldState fieldStateFrom(SharedPokemonEditor::FieldAccess access) noexcept {
    switch (access) {
        case SharedPokemonEditor::FieldAccess::Hidden: return FieldState::Hidden;
        case SharedPokemonEditor::FieldAccess::Derived: return FieldState::Derived;
        case SharedPokemonEditor::FieldAccess::ReadOnly: return FieldState::ReadOnly;
        case SharedPokemonEditor::FieldAccess::Editable: return FieldState::Editable;
    }
    return FieldState::Hidden;
}

constexpr SharedPokemonEditor::Generation sharedGeneration(PokemonEditorFoundation::Generation generation) noexcept {
    switch (generation) {
        case PokemonEditorFoundation::Generation::Gen1: return SharedPokemonEditor::Generation::Gen1;
        case PokemonEditorFoundation::Generation::Gen2: return SharedPokemonEditor::Generation::Gen2;
        case PokemonEditorFoundation::Generation::Gen3: return SharedPokemonEditor::Generation::Gen3;
        case PokemonEditorFoundation::Generation::Gen4: return SharedPokemonEditor::Generation::Gen4;
        case PokemonEditorFoundation::Generation::Gen5: return SharedPokemonEditor::Generation::Gen5;
        case PokemonEditorFoundation::Generation::Gen6: return SharedPokemonEditor::Generation::Gen6;
        case PokemonEditorFoundation::Generation::Gen7: return SharedPokemonEditor::Generation::Gen7;
        case PokemonEditorFoundation::Generation::Gen8: return SharedPokemonEditor::Generation::Gen8;
        case PokemonEditorFoundation::Generation::Gen9: return SharedPokemonEditor::Generation::Gen9;
    }
    return SharedPokemonEditor::Generation::Gen1;
}

constexpr FieldState fieldStateFor(const PokemonEditorFoundation::ExactSaveCapabilities& exact,
                                   FieldIdentity field) noexcept {
    return fieldStateFrom(SharedPokemonEditor::fieldAccessForGeneration(
        sharedGeneration(exact.identity.generation), field, exact.supportsCrystalCaughtData));
}

constexpr bool fieldVisible(FieldState state) noexcept { return state != FieldState::Hidden; }
constexpr bool fieldIsStoredEditable(FieldState state) noexcept { return state == FieldState::Editable; }

enum class StorageSemantics : uint8_t {
    PackedNative,
    SparseOwned,
    AdapterDefined,
};

enum class GeneticValueModel : uint8_t {
    DV,
    IV,
    AdapterDefined,
};

enum class TrainingValueModel : uint8_t {
    StatExperience,
    EV,
    AdapterDefined,
};

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

    constexpr std::string_view battleLabel(std::size_t index) const noexcept {
        return index < battleStatCount ? battleLabels[index] : std::string_view{};
    }
};

constexpr StatPresentationSchema gen1StatPresentation() noexcept {
    return {{{"HP", "Attack", "Defense", "Speed", "Special", ""}},
            5, 4, 1, 5, GeneticValueModel::DV, TrainingValueModel::StatExperience,
            true, true, false};
}

constexpr StatPresentationSchema gen2StatPresentation() noexcept {
    return {{{"HP", "Attack", "Defense", "Speed", "Sp. Atk", "Sp. Def"}},
            6, 4, 1, 5, GeneticValueModel::DV, TrainingValueModel::StatExperience,
            true, true, true};
}

enum class MoveCompatibilityResult : uint8_t {
    Compatible,
    PreserveExisting,
    Unsupported,
    Invalid,
};

enum class MoveCompatibilityProviderKind : uint8_t {
    None,
    Gen1ExactGame,
    Gen2ExactGame,
};

struct MoveCompatibilityQuery {
    std::string_view exactGameId;
    uint16_t species = 0;
    uint8_t form = 0;
    uint16_t move = 0;
    bool existingSourceMove = false;
};

using MoveCompatibilityEvaluator = MoveCompatibilityResult (*)(const MoveCompatibilityQuery&) noexcept;

struct MoveCompatibilityProvider {
    MoveCompatibilityProviderKind kind = MoveCompatibilityProviderKind::None;
    std::string_view exactGameId;
    MoveCompatibilityEvaluator evaluator = nullptr;

    constexpr bool accepts(const MoveCompatibilityQuery& query) const noexcept {
        return kind != MoveCompatibilityProviderKind::None && query.exactGameId == exactGameId;
    }

    MoveCompatibilityResult evaluate(const MoveCompatibilityQuery& query) const noexcept {
        if (!accepts(query)) return MoveCompatibilityResult::Invalid;
        if (!evaluator) return MoveCompatibilityResult::Unsupported;
        return evaluator(query);
    }
};

enum class TrainerField : uint8_t {
    Name,
    TrainerId,
    SecretId,
    Gender,
    Money,
};

struct TrainerFieldDescriptor {
    TrainerField field = TrainerField::Name;
    FieldState state = FieldState::Hidden;
};

struct TrainerDescriptor {
    std::array<TrainerFieldDescriptor, 5> fields{};

    constexpr FieldState state(TrainerField field) const noexcept {
        for (const auto& entry : fields) {
            if (entry.field == field) return entry.state;
        }
        return FieldState::Hidden;
    }
};

constexpr TrainerDescriptor gen1TrainerDescriptor() noexcept {
    return {{{
        {TrainerField::Name, FieldState::Editable},
        {TrainerField::TrainerId, FieldState::ReadOnly},
        {TrainerField::SecretId, FieldState::Hidden},
        {TrainerField::Gender, FieldState::Hidden},
        {TrainerField::Money, FieldState::Editable},
    }}};
}

constexpr TrainerDescriptor gen2TrainerDescriptor() noexcept {
    return {{{
        {TrainerField::Name, FieldState::Editable},
        {TrainerField::TrainerId, FieldState::ReadOnly},
        {TrainerField::SecretId, FieldState::Hidden},
        {TrainerField::Gender, FieldState::ReadOnly},
        {TrainerField::Money, FieldState::Editable},
    }}};
}

struct StorageOperationCapabilities {
    bool stagedSingleReorder = false;
    bool stagedRectangleSelection = false;
    bool stagedBlockCarry = false;
    bool stagedBlockCopy = false;
    bool stagedBlockRemove = false;
};

struct SemanticActionCapabilities {
    // Field storage truth is independent from an action that transforms native backing fields.
    bool canTransformDerivedShiny = false;
};

struct SourceCapabilityBridge {
    PokeVault::Safety::SourceKind sourceKind = PokeVault::Safety::SourceKind::InstalledGame;
    PokeVault::SaveEdit::Capabilities saveOperations{};
    bool stagedWorkspaceAvailable = false;

    constexpr bool canEditStagedField(FieldState state) const noexcept {
        return stagedWorkspaceAvailable && state == FieldState::Editable &&
               saveOperations.supports(PokeVault::SaveEdit::Capability::PokemonEditing);
    }

    // Providers describe UI/staged rights only; original-source write permission is never granted here.
    constexpr bool canWriteOriginalSource() const noexcept { return false; }

    constexpr bool sourcePolicyWouldAllowDirectEdit() const noexcept {
        return PokeVault::Safety::canPerform(sourceKind, PokeVault::Safety::SourceMutation::Edit);
    }
};

struct ExactFormatEditorDescriptor {
    PokemonEditorFoundation::ExactSaveCapabilities exact;
    StorageSemantics storage = StorageSemantics::AdapterDefined;
    StatPresentationSchema stats{};
    MoveCompatibilityProvider moves{};
    TrainerDescriptor trainer{};
    SourceCapabilityBridge source{};
    StorageOperationCapabilities storageOperations{};
    SemanticActionCapabilities actions{};

    constexpr FieldState fieldState(FieldIdentity field) const noexcept {
        return fieldStateFor(exact, field);
    }

    constexpr bool fieldVisible(FieldIdentity field) const noexcept {
        return ExactFormatEditor::fieldVisible(fieldState(field));
    }

    constexpr bool fieldIsEditorTarget(FieldIdentity field) const noexcept {
        return source.canEditStagedField(fieldState(field));
    }
};

constexpr std::optional<ExactFormatEditorDescriptor> descriptorForAcceptedClassicSource(
    std::string_view sourceId,
    PokeVault::SaveEdit::Capabilities saveOperations,
    PokeVault::Safety::SourceKind sourceKind,
    bool stagedWorkspaceAvailable,
    MoveCompatibilityEvaluator moveEvaluator = nullptr) noexcept {
    const auto exact = PokemonEditorFoundation::capabilitiesForSourceId(sourceId);
    if (!exact) return std::nullopt;

    const bool gen1 = exact->identity.generation == PokemonEditorFoundation::Generation::Gen1;
    const bool gen2 = exact->identity.generation == PokemonEditorFoundation::Generation::Gen2;
    if (!gen1 && !gen2) return std::nullopt;

    return ExactFormatEditorDescriptor{
        *exact,
        StorageSemantics::PackedNative,
        gen1 ? gen1StatPresentation() : gen2StatPresentation(),
        {gen1 ? MoveCompatibilityProviderKind::Gen1ExactGame
              : MoveCompatibilityProviderKind::Gen2ExactGame,
         sourceId,
         moveEvaluator},
        gen1 ? gen1TrainerDescriptor() : gen2TrainerDescriptor(),
        {sourceKind, saveOperations, stagedWorkspaceAvailable},
        {}, // P0 vocabulary only; inherited Move/Multi extraction remains later work.
        {true},
    };
}

} // namespace PokeBank::UIModel::ExactFormatEditor
