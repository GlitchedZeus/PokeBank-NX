#ifndef POKEBANK_GEN7_EXACT_FORMAT_EDITOR_PROVIDER_H
#define POKEBANK_GEN7_EXACT_FORMAT_EDITOR_PROVIDER_H

#include "Integration/Gen7/Gen7SaveLayout.h"
#include "UI/ExactFormatEditorProvider.h"

#include <cstdint>
#include <optional>
#include <string_view>

// PK7 capability provider for the ONE shared Pokemon editor (no separate UI).
// fieldState() describes what Gen7StagedSession can stage on an app-owned copy.
// SAV7 export is explicitly unsupported until the MemeCrypto re-sign payload is
// confirmed on a game-written save (docs/gen7/README.md), so the descriptor never
// offers a staged workspace to the UI: fieldIsEditorTarget() is false for every
// field while ExportSupported is false.
namespace PokeVault::Integration::Gen7EditorProvider {
namespace Exact = PokeBank::UIModel::ExactFormatEditor;
using Field = PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;
using State = Exact::FieldState;

inline constexpr bool ExportSupported = false;

// Party PK7 records cache battle stats; edits to a stat input (IV, EV, Nature,
// Level) recompute them (Gen7Stats.h, hyper-training aware) and the verifier
// checks the exact recomputation. RecordRegion only changes presentation.
enum class RecordRegion : uint8_t { Box, Party };

[[nodiscard]] constexpr State fieldState(Field field, RecordRegion region) noexcept {
    const bool party = region == RecordRegion::Party;
    switch (field) {
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
        case Field::IV:
        case Field::EV:
        case Field::Nature:
        case Field::Level:  // staged as the growth-rate minimum EXP
            return State::Editable;
        case Field::Shiny:  // PSV == TSV (xor < 16), as in Gen VI
            return State::Derived;
        case Field::CalculatedStats:
            return party ? State::Derived : State::Hidden;
        case Field::DV:
        case Field::StatExperience:
        case Field::AV:  // Let's Go only, a different save family
        case Field::EffortLevel:
        case Field::StatNature:
            return State::Hidden;
        default:
            return State::ReadOnly;
    }
}

[[nodiscard]] constexpr bool editable(Field field, RecordRegion region) noexcept {
    return fieldState(field, region) == State::Editable;
}

// Format bounds only. PKSM-Core aa22d7a4 source/utils/VersionTables.cpp: maxItem SN/MN 920,
// US/UM 959; maxBall(Generation::SEVEN) = Ball::Beast (26). EV caps as Gen VI (PKHeX
// EffortValues.cs Max252/Max510).
struct ValueLimits {
    uint16_t maxHeldItem = 0;
    uint8_t maxBall = 26;
    uint8_t maxIV = 31;
    uint8_t maxEV = 252;
    uint16_t maxEVTotal = 510;
    uint8_t maxNature = 24;
};
[[nodiscard]] constexpr ValueLimits limitsFor(Integration::Gen7::Layout::Family family) noexcept {
    ValueLimits v;
    v.maxHeldItem = family == Integration::Gen7::Layout::Family::SunMoon ? 920 : 959;
    return v;
}

struct Gen7EditorDescriptor {
    std::string_view exactGameId;
    Exact::StorageSemantics storage = Exact::StorageSemantics::SparseNative;
    Exact::StatPresentationSchema stats = Exact::gen3StatPresentation();
    Exact::MoveCompatibilityProvider moves{};
    Exact::SourceCapabilityBridge source{};
    [[nodiscard]] constexpr State fieldState(Field f, RecordRegion r) const noexcept {
        return Gen7EditorProvider::fieldState(f, r);
    }
    [[nodiscard]] constexpr bool fieldIsEditorTarget(Field f, RecordRegion r) const noexcept {
        return ExportSupported && source.canEditStagedField(fieldState(f, r));
    }
};

[[nodiscard]] inline std::optional<Gen7EditorDescriptor> descriptorForSource(std::string_view exactGameId) noexcept {
    if (!Integration::Gen7::Layout::isGen7GameId(exactGameId)) return std::nullopt;
    PokeVault::SaveEdit::Capabilities caps;
    caps.add(PokeVault::SaveEdit::Capability::PartyPokemon).add(PokeVault::SaveEdit::Capability::BoxPokemon);
    Gen7EditorDescriptor d;
    d.exactGameId = exactGameId;
    d.moves = {exactGameId, nullptr};
    d.source = {PokeVault::Safety::SourceKind::ExternalLegacy, caps, /*stagedWorkspaceAvailable*/ ExportSupported};
    return d;
}

} // namespace PokeVault::Integration::Gen7EditorProvider

#endif
