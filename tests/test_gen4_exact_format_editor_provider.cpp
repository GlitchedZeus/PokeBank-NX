#include "Integration/Gen4/Gen4ExactFormatEditorProvider.h"
#include "UI/SharedPokemonEditorContract.h"

#include <array>
#include <cassert>
#include <iostream>
#include <string_view>

int main() {
    namespace Provider = PokeVault::Integration::Gen4EditorProvider;
    namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    namespace Exact = PokeBank::UIModel::ExactFormatEditor;

    constexpr std::array<std::string_view, 5> ids{
        "diamond_nds", "pearl_nds", "platinum_nds",
        "heartgold_nds", "soulsilver_nds"
    };

    for (const auto id : ids) {
        const auto caps = Foundation::capabilitiesForSourceId(id);
        assert(caps);
        assert(caps->identity.platform == PokeVault::Games::Platform::NintendoDS);
        assert(caps->identity.generation == Foundation::Generation::Gen4);
        assert(caps->identity.format == Foundation::SaveFormat::PK4);
        assert(caps->statModel == Foundation::StatModel::IVEV);
        assert(caps->geneticMaximum == 31);
        assert(caps->trainingMaximum == 255);
        assert(caps->fields.supportsAbility);
        assert(caps->fields.supportsHeldItem);
        assert(caps->fields.supportsNature);
        assert(caps->fields.supportsFriendship);
        assert(caps->fields.supportsEgg);
        assert(caps->fields.supportsMetLevel);
        assert(caps->fields.supportsRibbons);
        assert(caps->fields.usesIVs);
        assert(caps->fields.usesEVs);
        assert(caps->supportsPokerus);

        const auto readOnly = Provider::descriptorForSource(id, false);
        assert(readOnly);
        assert(readOnly->storage == Exact::StorageSemantics::SparseNative);
        assert(readOnly->stats.geneticModel == Exact::GeneticValueModel::IV);
        assert(readOnly->stats.trainingModel == Exact::TrainingValueModel::EV);
        assert(readOnly->stats.battleStatCount == 6);
        assert(!readOnly->source.canWriteOriginalSource());
        assert(!readOnly->source.sourcePolicyWouldAllowDirectEdit());
        assert(!readOnly->fieldIsEditorTarget(Shared::FieldIdentity::Species));

        const auto staged = Provider::descriptorForSource(id, true);
        assert(staged);
        assert(staged->source.saveOperations.supports(
            PokeVault::SaveEdit::Capability::PokemonCreation));
        assert(staged->fieldState(Shared::FieldIdentity::Species) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::Gender) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::Shiny) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::Language) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::Nature) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::Ability) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::HeldItem) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::Pokerus) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::Ball) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::MetLocation) == Exact::FieldState::Editable);
        assert(staged->fieldState(Shared::FieldIdentity::Form) == Exact::FieldState::Editable);
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::Species));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::Language));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::HeldItem));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::Pokerus));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::Ball));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::MetLocation));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::Gender));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::IV));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::EV));
        assert(staged->fieldIsEditorTarget(Shared::FieldIdentity::Form));

        const Exact::MoveCompatibilityQuery existing{id, 25, 0, 85, true};
        const Exact::MoveCompatibilityQuery newMove{id, 25, 0, 85, false};
        const Exact::MoveCompatibilityQuery lastNative{id, 25, 0, 467, false};
        const Exact::MoveCompatibilityQuery firstGen5{id, 25, 0, 468, false};
        assert(staged->moves.evaluate(existing) == Exact::MoveCompatibilityResult::Compatible);
        assert(staged->moves.evaluate(newMove) == Exact::MoveCompatibilityResult::Compatible);
        assert(staged->moves.evaluate(lastNative) == Exact::MoveCompatibilityResult::Compatible);
        assert(staged->moves.evaluate(firstGen5) == Exact::MoveCompatibilityResult::Unsupported);
    }

    const auto dp = Foundation::capabilitiesForSourceId("diamond_nds");
    const auto pt = Foundation::capabilitiesForSourceId("platinum_nds");
    const auto hg = Foundation::capabilitiesForSourceId("heartgold_nds");
    assert(dp && dp->family == Foundation::SaveFamily::DiamondPearl);
    assert(pt && pt->family == Foundation::SaveFamily::Platinum);
    assert(hg && hg->family == Foundation::SaveFamily::HeartGoldSoulSilver);

    assert(!Foundation::exactSaveCapabilities({
        "diamond_nds", PokeVault::Games::Platform::GameBoyAdvance,
        Foundation::Generation::Gen4, Foundation::SaveFormat::PK4
    }));
    assert(!Foundation::exactSaveCapabilities({
        "diamond_nds", PokeVault::Games::Platform::NintendoDS,
        Foundation::Generation::Gen3, Foundation::SaveFormat::PK4
    }));
    assert(!Foundation::exactSaveCapabilities({
        "diamond_nds", PokeVault::Games::Platform::NintendoDS,
        Foundation::Generation::Gen4, Foundation::SaveFormat::PK3GBA
    }));

    constexpr auto layout = Shared::layoutFor(Shared::Generation::Gen4);
    static_assert(layout.detailsRows == 16);
    static_assert(layout.valuesRows == 11);
    static_assert(layout.movesRows == 4);
    static_assert(layout.valueStatRows == 6);
    static_assert(Shared::genderUsesInlineToggle(Shared::Generation::Gen4));
    static_assert(!Shared::genderOpensDedicatedPicker(Shared::Generation::Gen4));
    static_assert(Shared::fieldAccessForGeneration(
        Shared::Generation::Gen4, Shared::FieldIdentity::Gender) ==
        Shared::FieldAccess::Editable);
    static_assert(Shared::fieldAccessForGeneration(
        Shared::Generation::Gen4, Shared::FieldIdentity::Form) ==
        Shared::FieldAccess::Editable);

    std::cout << "Gen IV exact shared-editor provider contract PASS\n";
    return 0;
}
