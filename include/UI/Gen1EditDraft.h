#pragma once

#include "Integration/Gen1/Gen1StagedPokemonEditor.h"

#include <array>

// The single edit that rewrites a Gen I box Pokemon to exactly `record`: used by the Gen I Edit
// working-copy draft's Keep to move the drafted Pokemon into the real staged copy in one
// verified stageEdit (every editable field; the HP DV is derived from the four stored DVs).
namespace PokeBank::UIModel::Gen1EditDraft {

inline PokeVault::Integration::Gen1::BoxPokemonEdit editFromRecord(
    const PokeVault::Integration::Gen1::PokemonRecord& record) {
    PokeVault::Integration::Gen1::BoxPokemonEdit edit;
    edit.species = record.species;
    edit.nickname = record.nickname;
    edit.level = record.level;
    edit.experience = record.experience;
    edit.trainerId = record.trainerId;
    edit.moves = record.moves;
    edit.pp = record.pp;
    edit.ppUps = record.ppUps;
    edit.dvs = std::array<uint8_t, 4>{record.dvs[1], record.dvs[2], record.dvs[3], record.dvs[4]};
    edit.statExperience = record.statExperience;
    edit.otName = record.originalTrainer;
    return edit;
}

} // namespace PokeBank::UIModel::Gen1EditDraft
