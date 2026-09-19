#include "Integration/Gen3/Gen3ExactFormatEditorProvider.h"
#include "Safety/SourceMutationPolicy.h"
#include "UI/ExactSaveCapabilities.h"
#include "UI/SharedPokemonEditorContract.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    namespace Provider = PokeVault::Integration::Gen3EditorProvider;
    namespace Exact = PokeBank::UIModel::ExactFormatEditor;
    namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    using PokeVault::Safety::SourceKind;

    constexpr std::array<const char*,5> ids{{
        "ruby_gba", "sapphire_gba", "emerald_gba", "firered_gba", "leafgreen_gba"
    }};
    for (const auto* id : ids) {
        const auto exact = Foundation::capabilitiesForSourceId(id);
        assert(exact);
        assert(exact->identity.platform == PokeVault::Games::Platform::GameBoyAdvance);
        assert(exact->identity.generation == Foundation::Generation::Gen3);
        assert(exact->identity.format == Foundation::SaveFormat::PK3GBA);
        assert(exact->statModel == Foundation::StatModel::IVEV);

        const auto descriptor = Provider::descriptorForSource(id, true);
        assert(descriptor);
        assert(descriptor->storage == Exact::StorageSemantics::SparseNative);
        assert(descriptor->stats.battleStatCount == 6);
        assert(descriptor->stats.storedGeneticValueCount == 6);
        assert(descriptor->stats.trainingValueCount == 6);
        assert(descriptor->stats.geneticModel == Exact::GeneticValueModel::IV);
        assert(descriptor->stats.trainingModel == Exact::TrainingValueModel::EV);

        assert(descriptor->fieldState(Shared::FieldIdentity::Species) == Exact::FieldState::Editable);
        assert(descriptor->fieldState(Shared::FieldIdentity::IV) == Exact::FieldState::Editable);
        assert(descriptor->fieldState(Shared::FieldIdentity::EV) == Exact::FieldState::Editable);
        assert(descriptor->fieldState(Shared::FieldIdentity::HeldItem) == Exact::FieldState::Editable);
        assert(descriptor->fieldState(Shared::FieldIdentity::Language) == Exact::FieldState::Editable);
        assert(descriptor->fieldState(Shared::FieldIdentity::Ball) == Exact::FieldState::Editable);

        // First Gen III candidate freezes every PID / trainer-id correlated field.
        assert(descriptor->fieldState(Shared::FieldIdentity::Nature) == Exact::FieldState::ReadOnly);
        assert(descriptor->fieldState(Shared::FieldIdentity::Gender) == Exact::FieldState::ReadOnly);
        assert(descriptor->fieldState(Shared::FieldIdentity::Shiny) == Exact::FieldState::ReadOnly);
        assert(descriptor->fieldState(Shared::FieldIdentity::Ability) == Exact::FieldState::ReadOnly);
        assert(descriptor->fieldState(Shared::FieldIdentity::PersonalityId) == Exact::FieldState::ReadOnly);
        assert(descriptor->fieldState(Shared::FieldIdentity::TrainerId) == Exact::FieldState::ReadOnly);
        assert(descriptor->fieldState(Shared::FieldIdentity::SecretId) == Exact::FieldState::ReadOnly);
        assert(descriptor->fieldState(Shared::FieldIdentity::OriginGame) == Exact::FieldState::ReadOnly);

        assert(descriptor->moves.evaluate({id, 25, 0, 33, true}) ==
               Exact::MoveCompatibilityResult::PreserveExisting);
        assert(descriptor->moves.evaluate({id, 25, 0, 33, false}) ==
               Exact::MoveCompatibilityResult::Unsupported);
        assert(descriptor->moves.evaluate({"firered_switch", 25, 0, 33, true}) ==
               Exact::MoveCompatibilityResult::Invalid);

        assert(descriptor->fieldIsEditorTarget(Shared::FieldIdentity::Species));
        assert(!descriptor->fieldIsEditorTarget(Shared::FieldIdentity::Nature));
        assert(!descriptor->source.canWriteOriginalSource());
        assert(!descriptor->source.sourcePolicyWouldAllowDirectEdit());
    }

    const auto ruby = Foundation::capabilitiesForSourceId("ruby_gba");
    const auto emerald = Foundation::capabilitiesForSourceId("emerald_gba");
    const auto firered = Foundation::capabilitiesForSourceId("firered_gba");
    assert(ruby && emerald && firered);
    assert(ruby->family == Foundation::SaveFamily::RubySapphire);
    assert(emerald->family == Foundation::SaveFamily::Emerald);
    assert(firered->family == Foundation::SaveFamily::FireRedLeafGreen);

    assert(!Foundation::capabilitiesForSourceId("firered_switch"));
    assert(!PokeVault::Safety::canPerform(SourceKind::RetroArchLegacy,
                                          PokeVault::Safety::SourceMutation::Edit));
    assert(!Shared::generationOwnsSeparateEditorUI());
    assert(Shared::futureGenerationsExtendSameFoundation());

    std::cout << "Gen III exact-format shared editor provider: PASS\n";
}
