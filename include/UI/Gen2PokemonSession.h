#pragma once
#include <algorithm>
#include "UI/Gen2PokemonEditorRules.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Pokemon/Experience.h"

namespace PokeBank::UIModel::Gen2PokemonEditor {
enum class SessionMode : uint8_t { None, Create, Edit, View };
enum class ProgressionSource : uint8_t { Unchanged, Level, Experience };
inline std::array<uint8_t,4> storedDVs(const Gen2::PokemonRecord& p) noexcept {
    return {p.dvs[1],p.dvs[2],p.dvs[3],p.dvs[4]};
}
inline bool sameEditableRecord(const Gen2::PokemonRecord& a, const Gen2::PokemonRecord& b) noexcept {
    return a.species == b.species && a.heldItem == b.heldItem && a.moves == b.moves &&
           a.trainerId == b.trainerId && a.experience == b.experience &&
           a.statExperience == b.statExperience && a.dvs == b.dvs &&
           a.pp == b.pp && a.ppUps == b.ppUps && a.friendship == b.friendship &&
           a.pokerus == b.pokerus && a.caughtData == b.caughtData && a.level == b.level &&
           a.originalTrainer == b.originalTrainer && a.nickname == b.nickname;
}

// UI-owned transaction. Only keep/add below can mutate the staged copy; field
// browsing and Back/Discard/Continue never call the backend.
struct Session {
    SessionMode mode = SessionMode::None;
    bool confirmExit = false;
    ProgressionSource progression = ProgressionSource::Unchanged;
    Gen2::PokemonRecord baseline{}, working{};
    void begin(const Gen2::PokemonRecord& p, SessionMode next) {
        baseline = working = p; mode = next; confirmExit = false;
        progression = next == SessionMode::Create ? ProgressionSource::Level : ProgressionSource::Unchanged;
    }
    bool editable() const noexcept { return mode == SessionMode::Create || mode == SessionMode::Edit; }
    bool dirty() const noexcept { return !sameEditableRecord(baseline, working); }
    uint32_t maximumExperience() const noexcept {
        const auto* p = Gen2::personalRecord(working.species);
        return p ? Pokemon::getExpForLevel(100, p->experienceGrowth) : 0;
    }
    bool setLevel(uint8_t value) noexcept {
        const auto* p = Gen2::personalRecord(working.species);
        if (!editable() || !p || value < 1 || value > 100) return false;
        working.level = value;
        working.experience = Pokemon::getExpForLevel(value,p->experienceGrowth);
        progression = ProgressionSource::Level;
        return true;
    }
    bool setExperience(uint32_t value) noexcept {
        const auto* p = Gen2::personalRecord(working.species);
        if (!editable() || !p || value > maximumExperience()) return false;
        working.experience = value;
        working.level = Pokemon::getLevelFromExp(value,p->experienceGrowth);
        progression = ProgressionSource::Experience;
        return true;
    }
    bool setSpecies(uint16_t species) noexcept {
        if (!editable() || !Gen2::personalRecord(species)) return false;
        working.species = species;
        return progression == ProgressionSource::Level ? setLevel(working.level)
            : setExperience(std::min(working.experience, maximumExperience()));
    }
    uint32_t experienceToNext() const noexcept {
        const auto* p = Gen2::personalRecord(working.species);
        if (!p || working.level >= 100) return 0;
        const auto next = Pokemon::getExpForLevel(working.level+1,p->experienceGrowth);
        return next > working.experience ? next - working.experience : 0;
    }
    bool movesAllowCommit(Gen2::SourceGame game) const noexcept {
        for (size_t i=0; i<4; ++i) {
            const bool compatible = Gen2::MoveCompatibility::canLearnMove(game,working.species,working.moves[i]);
            const bool preserved = mode == SessionMode::Edit && working.species == baseline.species && working.moves[i] == baseline.moves[i];
            if (!compatible && !preserved) return false;
        }
        return true;
    }
    Gen2::BoxPokemonEdit editRequest() const {
    const auto& before = baseline;
    const auto& after = working;
    Gen2::BoxPokemonEdit edit;
    if (before.species != after.species) edit.species = after.species;
    if (before.nickname != after.nickname) edit.nickname = after.nickname;
    if (before.level != after.level || before.experience != after.experience) {
        if (progression == ProgressionSource::Level) edit.level = after.level;
        else if (progression == ProgressionSource::Experience) edit.experience = after.experience;
    }
    if (before.heldItem != after.heldItem) edit.heldItem = after.heldItem;
    if (before.moves != after.moves) edit.moves = after.moves;
    if (before.pp != after.pp) edit.pp = after.pp;
    if (before.ppUps != after.ppUps) edit.ppUps = after.ppUps;
    if (storedDVs(before) != storedDVs(after)) edit.dvs = storedDVs(after);
    if (before.statExperience != after.statExperience) edit.statExperience = after.statExperience;
    if (before.originalTrainer != after.originalTrainer) edit.otName = after.originalTrainer;
    if (before.trainerId != after.trainerId) edit.trainerId = after.trainerId;
    if (before.friendship != after.friendship) edit.friendship = after.friendship;
    if (before.pokerus != after.pokerus) edit.pokerus = after.pokerus;
    if (before.caughtData != after.caughtData) edit.caughtData = after.caughtData;

        return edit;
    }
    Gen2::BoxPokemonCreate createRequest() const {
    Gen2::BoxPokemonCreate create;
    create.species = working.species;
    create.level = working.level;
    create.nickname = working.nickname;
    create.otName = working.originalTrainer;
    create.trainerId = working.trainerId;
    create.heldItem = working.heldItem;
    create.moves = working.moves;
    create.pp = working.pp;
    create.ppUps = working.ppUps;
    create.dvs = storedDVs(working);
    create.statExperience = working.statExperience;
    create.friendship = working.friendship;
    create.pokerus = working.pokerus;
    create.caughtData = working.caughtData;

        if (progression == ProgressionSource::Experience) create.experience = working.experience;
        return create;
    }
    bool keep(Gen2::StagedEditor& editor,size_t box,size_t slot,std::string& error) {
        if (mode != SessionMode::Edit || !movesAllowCommit(editor.metadata().sourceGame)) {
            error = "Keep unavailable or incompatible new move"; return false;
        }
        if (!editor.stageBoxPokemonEdit(box,slot,editRequest(),error)) return false;
        mode = SessionMode::None; confirmExit = false; return true;
    }
    bool add(Gen2::StagedEditor& editor,size_t box,size_t& slot,std::string& error) {
        if (mode != SessionMode::Create || !movesAllowCommit(editor.metadata().sourceGame)) {
            error = "Create unavailable or incompatible move"; return false;
        }
        if (!editor.stageAddBoxPokemon(box,createRequest(),slot,error)) return false;
        mode = SessionMode::None; return true;
    }
    // true means return to the caller; false means present Keep/Discard/Continue.
    bool back() noexcept {
        if (mode == SessionMode::Edit && dirty()) { confirmExit = true; return false; }
        mode = SessionMode::None; confirmExit = false; return true;
    }
    void discard() { working = baseline; mode = SessionMode::None; confirmExit = false; }
    void continueEditing() noexcept { confirmExit = false; }
};
} // namespace PokeBank::UIModel::Gen2PokemonEditor
