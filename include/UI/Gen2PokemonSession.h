#pragma once
#include <algorithm>
#include "UI/ClassicDefaultNickname.h"
#include "UI/SpeciesChangeLevelPolicy.h"
#include "UI/PokemonEditorExitGuard.h"
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
    bool setSpecies(uint16_t species, std::string_view sourceGameId = {}) noexcept {
        if (!editable() || !Gen2::personalRecord(species)) return false;
        if (species == working.species) return true;
        const uint8_t nextLevel =
            SpeciesChangeLevelPolicy::defaultLevel(sourceGameId, species);
        working.nickname = nicknameAfterSpeciesChange(working.nickname, working.species, species);
        working.species = species;
        const auto dvs = storedDVs(working);
        working.dvs[0] = Gen2::StagedEditor::derivedHPDV(dvs);
        working.shiny = Gen2::StagedEditor::isShinyDVs(dvs);
        working.gender = static_cast<uint8_t>(Gen2::genderFromAttackDV(species, dvs[0]));
        return setLevel(nextLevel);
    }
    uint8_t maximumPP(size_t slot) const noexcept {
        return slot < 4 ? Gen2::StagedEditor::gen2MoveMaxPP(working.moves[slot],working.ppUps[slot]) : 0;
    }
    bool setMove(size_t slot, uint16_t move) noexcept {
        if (!editable() || slot >= 4 || move > 251) return false;
        working.moves[slot] = static_cast<uint8_t>(move);
        working.ppUps[slot] = 0;
        working.pp[slot] = Gen2::StagedEditor::gen2MoveBasePP(move);
        return true;
    }
    bool setPPUps(size_t slot, uint8_t ups) noexcept {
        if (!editable() || slot >= 4 || ups > 3) return false;
        working.ppUps[slot] = working.moves[slot] == 0 ? 0 : ups;
        working.pp[slot] = std::min(working.pp[slot], maximumPP(slot));
        return true;
    }
    bool setPP(size_t slot, uint8_t pp) noexcept {
        if (!editable() || slot >= 4 || pp > maximumPP(slot)) return false;
        working.pp[slot] = pp;
        if (working.moves[slot] == 0) working.ppUps[slot] = 0;
        return true;
    }
    uint32_t experienceToNext() const noexcept {
        const auto* p = Gen2::personalRecord(working.species);
        if (!p || working.level >= 100) return 0;
        const auto next = Pokemon::getExpForLevel(working.level+1,p->experienceGrowth);
        return next > working.experience ? next - working.experience : 0;
    }
    CompatibilityState moveCompatibility(Gen2::SourceGame game, size_t slot) const noexcept {
        if (slot >= 4) return CompatibilityState::NeedsCorrection;
        const bool compatible = Gen2::MoveCompatibility::canLearnMove(game, working.species, working.moves[slot]);
        if (mode == SessionMode::View) return passiveViewCompatibility(compatible);
        const bool preserved = mode == SessionMode::Edit && working.species == baseline.species &&
            working.moves[slot] == baseline.moves[slot];
        return editMoveCompatibility(compatible, preserved);
    }
    CompatibilityState moveSummary(Gen2::SourceGame game) const noexcept {
        auto result = CompatibilityState::Ok;
        for (size_t i = 0; i < 4; ++i) {
            const auto status = moveCompatibility(game, i);
            if (static_cast<int>(status) > static_cast<int>(result)) result = status;
        }
        return result;
    }
    bool movesAllowCommit(Gen2::SourceGame game) const noexcept {
        return mayCommit(moveSummary(game));
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
    // true means return to the caller; false means present Add/Keep, Discard, Continue.
    // Create and Edit always confirm, even when Edit is clean.
    bool back() noexcept {
        using Guard = PokeBank::UIModel::PokemonEditorExitGuard::SessionKind;
        const Guard kind = mode == SessionMode::Create ? Guard::Create
                         : mode == SessionMode::Edit ? Guard::Edit
                                                     : Guard::View;
        if (PokeBank::UIModel::PokemonEditorExitGuard::requiresConfirmation(kind, dirty())) {
            confirmExit = true;
            return false;
        }
        mode = SessionMode::None; confirmExit = false; return true;
    }
    void discard() { working = baseline; mode = SessionMode::None; confirmExit = false; }
    void continueEditing() noexcept { confirmExit = false; }
};
} // namespace PokeBank::UIModel::Gen2PokemonEditor
