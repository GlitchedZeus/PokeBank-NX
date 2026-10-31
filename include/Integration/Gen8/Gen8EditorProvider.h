#ifndef POKEBANK_GEN8_EDITOR_PROVIDER_H
#define POKEBANK_GEN8_EDITOR_PROVIDER_H

// Backend capability provider for PK8 (Sword/Shield) inside the ONE shared Pokemon editor.
// Like Gen6EditorProvider it defines no screen: the shared editor asks it which fields are
// Editable, ReadOnly, Derived or Hidden, and Gen8StagedSession enforces exactly this table
// (its verifier derives its per-byte whitelist from it), so the UI can never offer an edit the
// staged serializer would refuse.
//
// Editable here means "a plain stored PK8 field with no derived companion that this build can
// write and verify exactly". Battle-stat inputs (IV, EV, Nature, Level) stay ReadOnly: every SWSH
// record (party and box) carries cached battle stats at 0x148..0x157 and this build has no
// pinned Sword/Shield base-stat/growth table to recompute them, so editing them would leave stale
// cached stats. That is a stated prerequisite, not a missing feature hidden behind a button.
//
// Offsets: vendor/PKSM-Core/source/pkx/PK8.cpp (pinned submodule) and
// PKHeX 6501f0ab PKHeX.Core/PKM/PK8.cs + G8PKM.cs.

#include "UI/ExactFormatEditorProvider.h"
#include "UI/SharedPokemonEditorContract.h"

#include <array>
#include <cstdint>

namespace PokeVault::Integration::Gen8EditorProvider {
namespace Exact = PokeBank::UIModel::ExactFormatEditor;
using Field = PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;
using State = Exact::FieldState;

enum class RecordRegion : uint8_t { Box, Party };

[[nodiscard]] constexpr State fieldState(Field field, RecordRegion) noexcept {
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
        // PID ^ TID ^ SID; follows TID/SID edits.
        case Field::Shiny:
            return State::Derived;
        // Cached in every SWSH record (party format is used in boxes too).
        case Field::CalculatedStats:
            return State::Derived;
        // Battle-stat inputs: shown, not edited until a pinned SWSH stat table can recompute
        // the cached stats exactly (see header comment). StatNature is the SWSH mint byte.
        case Field::IV:
        case Field::EV:
        case Field::Nature:
        case Field::StatNature:
        case Field::Level:
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
        // Not part of the PK8 format (AV/EffortLevel are Let's Go / Legends mechanics).
        case Field::DV:
        case Field::StatExperience:
        case Field::AV:
        case Field::EffortLevel:
            return State::Hidden;
    }
    return State::Hidden;
}

[[nodiscard]] constexpr bool editable(Field field, RecordRegion region) noexcept {
    return fieldState(field, region) == State::Editable;
}

// Format bounds only; legality belongs to the Legality Engine.
//   PKSM-Core VersionTables.cpp: maxBall(Generation::EIGHT) = Ball::Beast (26).
//   Held items are additionally filtered by Names::isHeldItemPresent(id, SWSH).
struct ValueLimits {
    uint16_t maxHeldItem = 1607;   // PKSM-Core VersionTables maxItem(SWSH)
    uint8_t minBall = 1;
    uint8_t maxBall = 26;
};
inline constexpr ValueLimits Limits{};

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

} // namespace PokeVault::Integration::Gen8EditorProvider

#endif
