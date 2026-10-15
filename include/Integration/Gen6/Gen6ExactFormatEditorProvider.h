#ifndef POKEBANK_GEN6_EXACT_FORMAT_EDITOR_PROVIDER_H
#define POKEBANK_GEN6_EXACT_FORMAT_EDITOR_PROVIDER_H

#include "Integration/Gen6/Gen6SaveLayout.h"
#include "UI/ExactFormatEditorProvider.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

// Backend capability provider for PK6 inside the ONE shared Pokemon editor.
// It defines no screen, route or editor of its own: the shared HOME-style
// editor asks it, through the existing ExactFormatEditor vocabulary
// (FieldIdentity -> FieldState), which PK6 fields are Editable, ReadOnly,
// Derived or Hidden. Gen6StagedSession enforces exactly the same table, so the
// UI can never offer an edit that the staged serializer would refuse.
namespace PokeVault::Integration::Gen6EditorProvider {
namespace Exact = PokeBank::UIModel::ExactFormatEditor;
using Field = PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;
using State = Exact::FieldState;

// Party PK6 records carry cached battle stats (0xE8..0x104). Edits that change
// a battle-stat input (IV, EV, Nature, Level) recompute those cached stats from
// the cited formulas and PKHeX personal_ao base stats (Gen6Stats.h); the
// verifier checks the result exactly. RecordRegion only changes presentation.
enum class RecordRegion : uint8_t { Box, Party };

[[nodiscard]] constexpr State fieldState(Field field, RecordRegion region) noexcept {
    const bool party = region == RecordRegion::Party;
    switch (field) {
        // Plain stored PK6 fields with no derived companions.
        case Field::Nickname:
        case Field::HeldItem:
        case Field::Friendship:
        case Field::Pokerus:
        case Field::OriginalTrainer:
        case Field::OriginalTrainerGender:
        case Field::TrainerId:
        case Field::SecretId:
        case Field::Ball:
        case Field::FatefulEncounter:
            return State::Editable;
        // Battle-stat inputs; party records get recomputed cached stats.
        // Level is staged as the growth-rate minimum EXP for that level.
        case Field::IV:
        case Field::EV:
        case Field::Nature:
        case Field::Level:
            return State::Editable;
        // Computed from PID ^ TID ^ SID (G6PKM PSV/TSV); follows TID/SID edits.
        case Field::Shiny:
            return State::Derived;
        case Field::CalculatedStats:
            return party ? State::Derived : State::Hidden;
        // Correlated with PID, species personal data, encounter or move data that
        // this lane does not have verified: shown, never edited.
        case Field::Species:
        case Field::Form:
        case Field::Gender:
        case Field::Language:
        case Field::Experience:
        case Field::Ability:
        case Field::PersonalityId:
        case Field::OriginGame:
        case Field::MetLevel:
        case Field::MetLocation:
        case Field::MetDate:
        case Field::Egg:
        case Field::EggLocation:
        case Field::EggDate:
        case Field::Move:
        case Field::MovePP:
        case Field::MovePPUps:
        case Field::MoveCompatibility:
        case Field::EncounterLegality:
        case Field::Provenance:
            return State::ReadOnly;
        // Not part of the PK6 format.
        case Field::DV:
        case Field::StatExperience:
        case Field::AV:
        case Field::EffortLevel:
        case Field::StatNature:
            return State::Hidden;
    }
    return State::Hidden;
}

[[nodiscard]] constexpr bool editable(Field field, RecordRegion region) noexcept {
    return fieldState(field, region) == State::Editable;
}

// Value limits (format bounds only; legality belongs to the Legality Engine).
//   PKSM-Core aa22d7a4 source/utils/VersionTables.cpp: maxItem X/Y 717, OR/AS 775;
//   maxBall(Generation::SIX) = Ball::Dream (25).
//   PKHeX 6501f0ab PKM/Util/EffortValues.cs: Max252 per stat (Gen VI+), Max510 total.
struct ValueLimits {
    uint16_t maxHeldItem = 0;
    uint8_t maxBall = 25;
    uint8_t maxIV = 31;
    uint8_t maxEV = 252;
    uint16_t maxEVTotal = 510;
    uint8_t maxNature = 24;
};
[[nodiscard]] constexpr ValueLimits limitsFor(Integration::Gen6::Layout::Family family) noexcept {
    ValueLimits limits;
    limits.maxHeldItem = family == Integration::Gen6::Layout::Family::XY ? 717 : 775;
    return limits;
}

[[nodiscard]] constexpr PokeVault::SaveEdit::Capabilities stagedCapabilities() noexcept {
    PokeVault::SaveEdit::Capabilities caps;
    caps.add(PokeVault::SaveEdit::Capability::PartyPokemon)
        .add(PokeVault::SaveEdit::Capability::BoxPokemon)
        .add(PokeVault::SaveEdit::Capability::PokemonEditing)
        .add(PokeVault::SaveEdit::Capability::PokemonClone);
    // PokemonCreation stays absent: PK6 Create needs EC/PID, personal data,
    // met/handler/memory/geo fields that are not verifiably derivable here.
    return caps;
}

// Shared-editor action set for an occupied PK6 slot. Remove is offered for
// boxed records only (party removal would require party compaction).
[[nodiscard]] constexpr PokeBank::UIModel::SharedPokemonEditor::ActionCapabilities
actionCapabilities(RecordRegion region) noexcept {
    return {/*canClone*/ true, /*canRemove*/ region == RecordRegion::Box,
            /*hasLegalityProvenance*/ false};
}

// Gen VI descriptor in the shared vocabulary. The shared ExactSaveCapabilities
// table has no 3DS platform / PK6 format yet (owned by Thread 2), so this
// descriptor carries the same parts -- stat schema, storage semantics, move
// provider, source bridge -- and the per-region field table above. See
// docs/gen6/integration-requests/ for the requested shared registration.
struct Gen6EditorDescriptor {
    std::string_view exactGameId;
    Integration::Gen6::Layout::Family family = Integration::Gen6::Layout::Family::XY;
    Exact::StorageSemantics storage = Exact::StorageSemantics::SparseNative;
    Exact::StatPresentationSchema stats = Exact::gen3StatPresentation();
    Exact::MoveCompatibilityProvider moves{};
    Exact::SourceCapabilityBridge source{};
    ValueLimits limits{};

    [[nodiscard]] constexpr State fieldState(Field field, RecordRegion region) const noexcept {
        return Gen6EditorProvider::fieldState(field, region);
    }
    [[nodiscard]] constexpr bool fieldIsEditorTarget(Field field, RecordRegion region) const noexcept {
        return source.canEditStagedField(fieldState(field, region));
    }
};

[[nodiscard]] inline std::optional<Gen6EditorDescriptor> descriptorForSource(
    std::string_view exactGameId, bool validatedStagedWorkspace) noexcept {
    if (!Integration::Gen6::Layout::isGen6GameId(exactGameId)) return std::nullopt;
    const auto family = Integration::Gen6::Layout::familyForGameId(exactGameId);
    Gen6EditorDescriptor d;
    d.exactGameId = exactGameId;
    d.family = family;
    d.moves = {exactGameId, nullptr};  // move compatibility: unsupported, never invented
    d.source = {PokeVault::Safety::SourceKind::ExternalLegacy, stagedCapabilities(),
                validatedStagedWorkspace};
    d.limits = limitsFor(family);
    return d;
}

// Every shared field identity, for exhaustive tests and the integration patch.
inline constexpr std::array<Field, 41> AllFields = {
    Field::Species, Field::Nickname, Field::Form, Field::Gender, Field::Shiny, Field::Language,
    Field::Level, Field::Experience, Field::Friendship, Field::DV, Field::IV,
    Field::StatExperience, Field::EV, Field::AV, Field::EffortLevel, Field::Nature,
    Field::StatNature, Field::CalculatedStats, Field::Ability, Field::HeldItem, Field::Pokerus,
    Field::OriginalTrainer, Field::TrainerId, Field::SecretId, Field::PersonalityId,
    Field::OriginGame, Field::Ball, Field::MetLevel, Field::MetLocation, Field::MetDate,
    Field::Egg, Field::EggLocation, Field::EggDate, Field::MoveCompatibility,
    Field::EncounterLegality, Field::Provenance, Field::OriginalTrainerGender, Field::Move,
    Field::MovePP, Field::MovePPUps, Field::FatefulEncounter,
};

} // namespace PokeVault::Integration::Gen6EditorProvider

#endif
