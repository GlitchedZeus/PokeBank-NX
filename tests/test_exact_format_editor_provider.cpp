#include "Safety/SourceMutationPolicy.h"
#include "Save/EditableSaveCapabilities.h"
#include "UI/ExactFormatEditorProvider.h"
#include "UI/PokemonEditorFoundationContract.h"
#include "UI/SharedPokemonEditorContract.h"

#include <cassert>
#include <iostream>
#include <type_traits>

namespace {
constexpr PokeVault::SaveEdit::Capabilities stagedPokemonCapabilities() noexcept {
    PokeVault::SaveEdit::Capabilities result;
    result.add(PokeVault::SaveEdit::Capability::BoxPokemon)
          .add(PokeVault::SaveEdit::Capability::PokemonEditing)
          .add(PokeVault::SaveEdit::Capability::PokemonCreation)
          .add(PokeVault::SaveEdit::Capability::PokemonClone)
          .add(PokeVault::SaveEdit::Capability::PokemonShinyToggle);
    return result;
}
}

int main() {
    namespace Exact = PokeBank::UIModel::ExactFormatEditor;
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
    using PokeVault::Safety::SourceKind;

    static_assert(!std::is_same_v<Shared::ActionCapabilities, PokeVault::SaveEdit::Capabilities>);
    static_assert(!Foundation::previewNavigationMutatesSource());
    static_assert(!Foundation::liveRetroArchWriteEnabled());
    static_assert(!Foundation::liveInstalledGameWriteEnabled());
    static_assert(!Foundation::liveOtherEmulatorWriteEnabled());

    const auto gen1 = Exact::descriptorForAcceptedClassicSource(
        "red_gb", stagedPokemonCapabilities(), SourceKind::RetroArchLegacy, true);
    assert(gen1);
    assert(gen1->storage == Exact::StorageSemantics::PackedNative);
    assert(gen1->stats.battleStatCount == 5);
    assert(gen1->stats.battleLabel(4) == "Special");
    assert(gen1->stats.storedGeneticValueCount == 4);
    assert(gen1->stats.derivedGeneticValueCount == 1);
    assert(gen1->stats.trainingValueCount == 5);
    assert(gen1->stats.geneticModel == Exact::GeneticValueModel::DV);
    assert(gen1->stats.trainingModel == Exact::TrainingValueModel::StatExperience);
    assert(gen1->stats.hpGeneticValueDerived);
    assert(gen1->stats.hasSingleNativeSpecial);
    assert(!gen1->stats.presentsSplitSpecialBattleStats);
    assert(gen1->fieldState(Shared::FieldIdentity::Species) == Exact::FieldState::Editable);
    assert(gen1->fieldState(Shared::FieldIdentity::DV) == Exact::FieldState::Editable);
    assert(gen1->fieldState(Shared::FieldIdentity::StatExperience) == Exact::FieldState::Editable);
    assert(gen1->fieldState(Shared::FieldIdentity::Shiny) == Exact::FieldState::Derived);
    assert(gen1->fieldState(Shared::FieldIdentity::HeldItem) == Exact::FieldState::Hidden);
    assert(!gen1->fieldVisible(Shared::FieldIdentity::HeldItem));
    assert(gen1->actions.canTransformDerivedShiny);
    assert(!gen1->fieldIsEditorTarget(Shared::FieldIdentity::Shiny));
    assert(gen1->fieldIsEditorTarget(Shared::FieldIdentity::Species));
    assert(gen1->moves.kind == Exact::MoveCompatibilityProviderKind::Gen1ExactGame);
    assert(gen1->moves.accepts({"red_gb", 25, 0, 85, false}));
    assert(!gen1->moves.accepts({"blue_gb", 25, 0, 85, false}));
    assert(gen1->trainer.state(Exact::TrainerField::Name) == Exact::FieldState::Editable);
    assert(gen1->trainer.state(Exact::TrainerField::TrainerId) == Exact::FieldState::ReadOnly);
    assert(gen1->trainer.state(Exact::TrainerField::SecretId) == Exact::FieldState::Hidden);
    assert(gen1->trainer.state(Exact::TrainerField::Gender) == Exact::FieldState::Hidden);
    assert(gen1->trainer.state(Exact::TrainerField::Money) == Exact::FieldState::Editable);
    assert(!gen1->source.canWriteOriginalSource());
    assert(!gen1->source.sourcePolicyWouldAllowDirectEdit());
    assert(!gen1->storageOperations.stagedSingleReorder);
    assert(!gen1->storageOperations.stagedRectangleSelection);

    const auto gen2 = Exact::descriptorForAcceptedClassicSource(
        "gold_gbc", stagedPokemonCapabilities(), SourceKind::RetroArchLegacy, true);
    assert(gen2);
    assert(gen2->storage == Exact::StorageSemantics::PackedNative);
    assert(gen2->stats.battleStatCount == 6);
    assert(gen2->stats.battleLabel(4) == "Sp. Atk");
    assert(gen2->stats.battleLabel(5) == "Sp. Def");
    assert(gen2->stats.storedGeneticValueCount == 4);
    assert(gen2->stats.derivedGeneticValueCount == 1);
    assert(gen2->stats.hpGeneticValueDerived);
    assert(gen2->stats.hasSingleNativeSpecial);
    assert(gen2->stats.presentsSplitSpecialBattleStats);
    assert(gen2->fieldState(Shared::FieldIdentity::HeldItem) == Exact::FieldState::Editable);
    assert(gen2->fieldState(Shared::FieldIdentity::Gender) == Exact::FieldState::Derived);
    assert(gen2->fieldState(Shared::FieldIdentity::Shiny) == Exact::FieldState::Derived);
    assert(gen2->fieldState(Shared::FieldIdentity::Nature) == Exact::FieldState::Hidden);
    assert(gen2->trainer.state(Exact::TrainerField::Name) == Exact::FieldState::Editable);
    assert(gen2->trainer.state(Exact::TrainerField::Money) == Exact::FieldState::Editable);
    assert(gen2->trainer.state(Exact::TrainerField::TrainerId) == Exact::FieldState::ReadOnly);
    assert(gen2->trainer.state(Exact::TrainerField::Gender) == Exact::FieldState::ReadOnly);
    assert(gen2->moves.kind == Exact::MoveCompatibilityProviderKind::Gen2ExactGame);
    assert(gen2->moves.accepts({"gold_gbc", 25, 0, 85, true}));
    assert(!gen2->source.canWriteOriginalSource());
    assert(!gen2->source.sourcePolicyWouldAllowDirectEdit());

    const auto crystal = Exact::descriptorForAcceptedClassicSource(
        "crystal_gbc", stagedPokemonCapabilities(), SourceKind::RetroArchLegacy, true);
    assert(crystal);
    assert(crystal->fieldState(Shared::FieldIdentity::MetLevel) == Exact::FieldState::Editable);
    assert(crystal->fieldState(Shared::FieldIdentity::MetLocation) == Exact::FieldState::Editable);

    // P0 deliberately does not route Generation III into an editable product provider.
    assert(!Exact::descriptorForAcceptedClassicSource(
        "ruby_gba", stagedPokemonCapabilities(), SourceKind::RetroArchLegacy, true));

    // Staged capability and original-source capability are intentionally distinct.
    assert(gen1->source.canEditStagedField(Exact::FieldState::Editable));
    assert(!gen1->source.canEditStagedField(Exact::FieldState::ReadOnly));
    assert(!gen1->source.canEditStagedField(Exact::FieldState::Derived));
    assert(!gen1->source.canEditStagedField(Exact::FieldState::Hidden));
    assert(!PokeVault::Safety::canPerform(SourceKind::InstalledGame, PokeVault::Safety::SourceMutation::Edit));
    assert(!PokeVault::Safety::canPerform(SourceKind::RetroArchLegacy, PokeVault::Safety::SourceMutation::Edit));

    // Result vocabulary preserves compatibility vs unusual-existing vs invalid distinctions.
    static_assert(Exact::MoveCompatibilityResult::Compatible != Exact::MoveCompatibilityResult::PreserveExisting);
    static_assert(Exact::MoveCompatibilityResult::Unsupported != Exact::MoveCompatibilityResult::Invalid);

    std::cout << "Exact-format editor provider P0 contract: Gen I/II semantics and source safety PASS\n";
    return 0;
}
