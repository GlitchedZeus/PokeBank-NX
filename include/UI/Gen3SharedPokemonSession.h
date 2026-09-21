#pragma once

#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Pokemon/Experience.h"
#include "UI/SpeciesChangeLevelPolicy.h"
#include "UI/PokemonEditorExitGuard.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace PokeBank::UIModel::Gen3SharedEditor {

namespace Gen3 = PokeVault::Integration::Gen3;

enum class Mode : uint8_t { None, View, Edit, Create };
enum class ProgressionSource : uint8_t { Unchanged, Level, Experience };

inline bool sameEditableRecord(const Gen3::StagedPokemonRecord& a,
                               const Gen3::StagedPokemonRecord& b) noexcept {
    return a.species == b.species &&
           a.nickname == b.nickname &&
           a.level == b.level &&
           a.experience == b.experience &&
           a.heldItem == b.heldItem &&
           a.moves == b.moves &&
           a.pp == b.pp &&
           a.ppUps == b.ppUps &&
           a.ivs == b.ivs &&
           a.evs == b.evs &&
           a.otName == b.otName &&
           a.language == b.language &&
           a.friendship == b.friendship &&
           a.pokerus == b.pokerus &&
           a.ball == b.ball &&
           a.metLevel == b.metLevel &&
           a.metLocation == b.metLocation;
}

struct Session {
    Mode mode = Mode::None;
    bool confirmExit = false;
    ProgressionSource progression = ProgressionSource::Unchanged;
    Gen3::StagedPokemonRecord baseline{};
    Gen3::StagedPokemonRecord working{};

    void begin(const Gen3::StagedPokemonRecord& record, Mode next) {
        baseline = working = record;
        mode = next;
        confirmExit = false;
        progression = next == Mode::Create ? ProgressionSource::Level
                                           : ProgressionSource::Unchanged;
    }

    void beginCreate(const Gen3::TrainerRecord& trainer) {
        Gen3::StagedPokemonRecord record{};
        record.species = 25;
        record.level = 5;
        record.language = 2;
        record.friendship = 70;
        record.ball = 4;
        record.metLevel = 5;
        record.otName = trainer.name;
        record.tid = trainer.tid16;
        record.sid = trainer.sid16;
        begin(record, Mode::Create);
    }

    bool editable() const noexcept { return mode == Mode::Edit || mode == Mode::Create; }
    bool dirty() const noexcept { return !sameEditableRecord(baseline, working); }

    bool setSpecies(uint16_t species, std::string_view sourceGameId = {}) noexcept {
        if (!editable() || species == 0 || species > 386) return false;
        if (species == working.species) return true;
        working.species = species;
        return setLevel(SpeciesChangeLevelPolicy::defaultLevel(sourceGameId, species));
    }

    bool setLevel(uint8_t level) noexcept {
        if (!editable() || level < 1 || level > 100 || working.species == 0) return false;
        working.level = level;
        working.experience = Pokemon::getExpForLevel(level, Pokemon::getGrowthRate(working.species));
        progression = ProgressionSource::Level;
        return true;
    }

    bool setExperience(uint32_t experience) noexcept {
        if (!editable() || working.species == 0) return false;
        const uint32_t maximum =
            Pokemon::getExpForLevel(100, Pokemon::getGrowthRate(working.species));
        if (experience > maximum) return false;
        working.experience = experience;
        working.level = Pokemon::getLevelFromExp(experience, Pokemon::getGrowthRate(working.species));
        progression = ProgressionSource::Experience;
        return true;
    }

    Gen3::BoxPokemonEdit editRequest() const {
        Gen3::BoxPokemonEdit edit;
        if (baseline.species != working.species) edit.species = working.species;
        if (baseline.nickname != working.nickname) edit.nickname = working.nickname;
        if (baseline.level != working.level || baseline.experience != working.experience) {
            if (progression == ProgressionSource::Experience) edit.experience = working.experience;
            else edit.level = working.level;
        }
        if (baseline.heldItem != working.heldItem) edit.heldItem = working.heldItem;
        if (baseline.moves != working.moves) edit.moves = working.moves;
        if (baseline.pp != working.pp) edit.pp = working.pp;
        if (baseline.ppUps != working.ppUps) edit.ppUps = working.ppUps;
        if (baseline.ivs != working.ivs) edit.ivs = working.ivs;
        if (baseline.evs != working.evs) edit.evs = working.evs;
        if (baseline.otName != working.otName) edit.otName = working.otName;
        if (baseline.language != working.language) edit.language = working.language;
        if (baseline.friendship != working.friendship) edit.friendship = working.friendship;
        if (baseline.pokerus != working.pokerus) edit.pokerus = working.pokerus;
        if (baseline.ball != working.ball) edit.ball = working.ball;
        if (baseline.metLevel != working.metLevel) edit.metLevel = working.metLevel;
        if (baseline.metLocation != working.metLocation) edit.metLocation = working.metLocation;
        return edit;
    }

    Gen3::BoxPokemonCreate createRequest() const {
        Gen3::BoxPokemonCreate create;
        create.species = working.species;
        create.level = working.level;
        create.nickname = working.nickname;
        create.heldItem = working.heldItem;
        create.moves = working.moves;
        create.pp = working.pp;
        create.ppUps = working.ppUps;
        create.language = working.language;
        create.friendship = working.friendship;
        create.ball = working.ball;
        create.metLocation = working.metLocation;
        return create;
    }

    bool keep(Gen3::StagedPokemonEditor& editor, std::size_t box, std::size_t slot,
              std::string& error) {
        if (mode != Mode::Edit) {
            error = "Generation III Keep is only available for an edit draft";
            return false;
        }
        if (!editor.stageBoxPokemonEdit(box, slot, editRequest(), error)) return false;
        mode = Mode::None;
        confirmExit = false;
        return true;
    }

    bool add(Gen3::StagedPokemonEditor& editor, std::size_t box, std::size_t slot,
             std::string& error) {
        if (mode != Mode::Create) {
            error = "Generation III Add is only available for a create draft";
            return false;
        }
        if (!editor.stageAddBoxPokemon(box, slot, createRequest(), error)) return false;
        mode = Mode::None;
        confirmExit = false;
        return true;
    }

    bool back() noexcept {
        using Guard = PokeBank::UIModel::PokemonEditorExitGuard::SessionKind;
        const Guard kind = mode == Mode::Create ? Guard::Create
                         : mode == Mode::Edit ? Guard::Edit
                                              : Guard::View;
        if (PokeBank::UIModel::PokemonEditorExitGuard::requiresConfirmation(kind, dirty())) {
            confirmExit = true;
            return false;
        }
        mode = Mode::None;
        confirmExit = false;
        return true;
    }

    void discardDraft() noexcept {
        working = baseline;
        mode = Mode::None;
        confirmExit = false;
    }

    void continueEditing() noexcept { confirmExit = false; }
};

} // namespace PokeBank::UIModel::Gen3SharedEditor
