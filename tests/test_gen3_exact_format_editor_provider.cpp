#include "Integration/Gen3/Gen3ExactFormatEditorProvider.h"
#include "Safety/SourceMutationPolicy.h"
#include "UI/ExactSaveCapabilities.h"
#include "UI/SharedPokemonEditorContract.h"

#include <array>
#include <algorithm>
#include <cassert>
#include <iostream>

namespace {
namespace Learnset = PokeVault::Integration::Gen3::Learnset;
constexpr uint64_t payloadHash(const auto& bytes) {
    uint64_t hash = 14695981039346656037ULL;
    for (uint8_t byte : bytes) hash = (hash ^ byte) * 1099511628211ULL;
    return hash;
}
static_assert(Learnset::kSpeciesCount == 387 && Learnset::kMoveCount == 355);
static_assert(Learnset::kBytesPerSpecies == 45);
static_assert(Learnset::kRubySapphire.size() == 17415);
static_assert(Learnset::kEmerald.size() == 17415);
static_assert(Learnset::kFireRedLeafGreen.size() == 17415);
// Pin the byte payloads from the original pret-derived table commit d3e18f8.
// A truncated std::array initializer silently zero-fills: size alone cannot catch it.
static_assert(payloadHash(Learnset::kRubySapphire) == 0x2515bc3223387330ULL);
static_assert(payloadHash(Learnset::kEmerald) == 0x07e774895773e313ULL);
static_assert(payloadHash(Learnset::kFireRedLeafGreen) == 0xa885c8386bd1cb9fULL);
}

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
               Exact::MoveCompatibilityResult::Invalid);
        assert(descriptor->moves.evaluate({"firered_switch", 25, 0, 33, true}) ==
               Exact::MoveCompatibilityResult::Invalid);

        const auto game = Provider::sourceGameForId(id);
        assert(game);
        const bool rubySapphire = std::string_view(id) == "ruby_gba" || std::string_view(id) == "sapphire_gba";
        // Thunderbolt is native for Pikachu. Soft-Boiled Clefairy requires a
        // transfer into Ruby/Sapphire, but is native to Emerald/FR/LG tutors.
        assert(descriptor->moves.evaluate({id, 25, 0, 85, false}) == Exact::MoveCompatibilityResult::Compatible);
        assert(descriptor->moves.evaluate({id, 35, 0, 135, false}) ==
            (rubySapphire ? Exact::MoveCompatibilityResult::Unsupported : Exact::MoveCompatibilityResult::Compatible));
        assert(descriptor->moves.evaluate({id, 25, 0, 355, true}) == Exact::MoveCompatibilityResult::Invalid);
        assert(descriptor->moves.evaluate({id, 0, 0, 85, false}) == Exact::MoveCompatibilityResult::Invalid);
        const auto fresh = Learnset::selectableMoves(*game, 25, {});
        const auto preserved = Learnset::selectableMoves(*game, 25, {33,0,0,0});
        assert(std::binary_search(fresh.begin(), fresh.end(), 85));
        assert(!std::binary_search(fresh.begin(), fresh.end(), 33));
        assert(std::binary_search(preserved.begin(), preserved.end(), 33));
        assert(std::binary_search(preserved.begin(), preserved.end(), 0));
        for (uint16_t species = 1; species < Learnset::kSpeciesCount; ++species) {
            const auto& table = Learnset::table(*game);
            assert((table[species * Learnset::kBytesPerSpecies + 44] & 0xF8) == 0);
            assert(!Learnset::directlyLearnable(*game, species, 355));
        }

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
