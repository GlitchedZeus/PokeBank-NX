#ifndef POKEBANK_GEN5_SHARED_POKEMON_SESSION_H
#define POKEBANK_GEN5_SHARED_POKEMON_SESSION_H

#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "UI/PokemonEditorExitGuard.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Isolated Gen V backend for the existing shared Pokémon View/Edit surface.
// No Create mode, physical SAV write, source injection or independent Gen V UI.
// Each draft is local until explicit Keep; B/exit cannot implicitly keep edits.
class Gen5SharedPokemonSession {
public:
    using Field = StagedPokemon5Record::Field;
    using Slot = Gen5StagedPokemonWorkspace::Slot;
    using Region = Gen5StagedPokemonWorkspace::Region;
    enum class Mode : uint8_t { None, View, Edit };

    [[nodiscard]] bool begin(const Gen5StagedPokemonWorkspace& workspace,
                             Slot location, Mode next, std::string& error) {
        error.clear();
        // A second Open must never silently replace an existing local draft.
        // The shared UI must explicitly Keep or Discard before switching slots.
        if(mode_!=Mode::None) {
            error="Gen V draft already open; Keep or Discard before changing slots";
            return false;
        }
        if(next != Mode::View && next != Mode::Edit) {
            error="Gen V shared editor requires explicit View or Edit";
            return false;
        }
        const auto record=readSlot(workspace,location);
        if(!record || !record->valid() || record->empty()) {
            error="Gen V session requires an occupied, valid party/box slot";
            return false;
        }
        auto draft=StagedPokemon5Record::create(record->originalEncryptedBytes(),&error,
                                                  workspace.black2White2());
        if(!draft)return false;
        baseline_={record->originalEncryptedBytes().begin(),
                   record->originalEncryptedBytes().end()};
        working_=std::move(*draft);
        location_=location;
        mode_=next;
        confirmExit_=false;
        return true;
    }

    [[nodiscard]] Mode mode() const noexcept { return mode_; }
    [[nodiscard]] bool editable() const noexcept {
        return mode_==Mode::Edit && working_.has_value();
    }
    [[nodiscard]] bool confirmExit() const noexcept { return confirmExit_; }
    [[nodiscard]] bool dirty() const noexcept {
        return editable() && working_->dirty();
    }
    [[nodiscard]] std::optional<Pokemon5ReadOnly> current() const {
        if(!working_)return std::nullopt;
        return working_->current();
    }

    [[nodiscard]] bool stage(Field field,size_t stat,uint32_t value,std::string& error) {
        if(!editable()) {
            error="Gen V View session is strictly read-only";
            return false;
        }
        return working_->stage(field,stat,value,&error);
    }

    // Backend Keep is a single atomic workspace update. The final PK5 must
    // match exactly the draft's encrypted bytes; an unexpected native-field
    // difference or concurrent slot change aborts the entire transaction.
    [[nodiscard]] bool keep(Gen5StagedPokemonWorkspace& workspace,
                            std::string& error) {
        if(!editable()) {
            error="Gen V Keep requires an Edit draft";
            return false;
        }
        const auto current=readSlot(workspace,location_);
        if(!current || !current->valid() || current->empty() ||
           !sameBytes(current->originalEncryptedBytes(),baseline_)) {
            error="Gen V underlying staged slot changed; reopen the editor";
            return false;
        }
        const Pokemon5ReadOnly before(baseline_);
        const auto after=working_->current();
        if(!before.valid() || !after.valid() || before.empty() || after.empty()) {
            error="Gen V draft failed strict PK5 validation";
            return false;
        }

        Gen5StagedPokemonWorkspace tentative=workspace;
        auto apply=[&](Field field,size_t stat,uint32_t value) {
            if(location_.region==Region::Party)
                return tentative.stageParty(location_.slot,field,stat,value,&error);
            return tentative.stageBox(location_.box,location_.slot,field,stat,value,&error);
        };

        if(before.nature()!=after.nature() &&
           !apply(Field::Nature,0,after.nature()))return false;
        if(before.friendship()!=after.friendship() &&
           !apply(Field::Friendship,0,after.friendship()))return false;
        // Gen IV parity tranche: these native PK5 metadata bytes are
        // independent of PID, gender, ability, and party battle stats.
        if(before.heldItem()!=after.heldItem() &&
           !apply(Field::HeldItem,0,after.heldItem()))return false;
        if(before.ball()!=after.ball() &&
           !apply(Field::Ball,0,after.ball()))return false;
        if(before.metLevel()!=after.metLevel() &&
           !apply(Field::MetLevel,0,after.metLevel()))return false;
        // Identity changes have deliberate game-side consequences: editing
        // TID/SID can change the displayed shiny calculation, but neither
        // PID nor any unrelated native PK5 field may be rewritten.
        if(before.tid()!=after.tid() &&
           !apply(Field::TrainerId,0,after.tid()))return false;
        if(before.sid()!=after.sid() &&
           !apply(Field::SecretId,0,after.sid()))return false;
        if(before.originalTrainerGender()!=after.originalTrainerGender() &&
           !apply(Field::OriginalTrainerGender,0,after.originalTrainerGender()))return false;
        if(before.fatefulEncounter()!=after.fatefulEncounter() &&
           !apply(Field::FatefulEncounter,0,after.fatefulEncounter()?1:0))return false;
        if(before.pokerusMode()!=after.pokerusMode() &&
           !apply(Field::Pokerus,0,after.pokerusMode()))return false;
        if(before.metLocation()!=after.metLocation() &&
           !apply(Field::MetLocation,0,after.metLocation()))return false;

        // Apply native form first (it may reconcile the form-specific
        // ability and forced gender), then choose exact ability slot/gender.
        // Keep and independent Review must use identical ordering.
        // A level picker writes the exact EXP growth threshold; an EXP
        // picker can retain partial progress. Replayed party level/stats
        // must match the entire original local draft byte for byte.
        if(before.experience()!=after.experience()) {
            const auto personal=personal5(after.species(),after.form(),workspace.black2White2());
            if(!personal) {error="Gen V EXP requires exact growth data";return false;}
            const uint8_t level=after.partyRecord()?after.partyLevel():
                levelForExperience5(personal.info->growthRate,after.experience());
            const bool exact=level>=1 && level<=100 &&
                after.experience()==experienceForLevel5(personal.info->growthRate,level);
            if(!apply(exact?Field::Level:Field::Experience,0,
                      exact?level:after.experience()))return false;
        }
        if(before.form()!=after.form() && !apply(Field::Form,0,after.form()))
            return false;
        const auto slot5=[](const Pokemon5ReadOnly& p)->uint32_t {
            return p.hiddenAbility()?2u:((p.pid()>>16)&1u);
        };
        if((before.ability()!=after.ability() ||
            before.hiddenAbility()!=after.hiddenAbility() ||
            slot5(before)!=slot5(after)) &&
           !apply(Field::AbilitySlot,0,slot5(after)))return false;
        if(before.gender()!=after.gender() &&
           !apply(Field::Gender,0,after.gender()))return false;

        const auto beforeIv=before.ivs();
        const auto afterIv=after.ivs();
        for(size_t i=0;i<6;++i)
            if(beforeIv[i]!=afterIv[i] && !apply(Field::IV,i,afterIv[i]))return false;

        const auto beforeEv=before.evs();
        const auto afterEv=after.evs();
        // Reduce EVs before raising any: intermediate totals must never
        // exceed 510 when the draft's final total is legal.
        for(size_t i=0;i<6;++i)
            if(afterEv[i]<beforeEv[i] && !apply(Field::EV,i,afterEv[i]))return false;
        for(size_t i=0;i<6;++i)
            if(afterEv[i]>beforeEv[i] && !apply(Field::EV,i,afterEv[i]))return false;

        // Moves are coupled to per-slot PP and PP-Ups. Apply move
        // selections first (native base-PP reset), then PP-Ups (native
        // max-PP refresh), and current PP LAST. The resulting exact PK5
        // encrypted bytes must match the local draft, or Keep fails atomically.
        const auto beforeMoves=before.moves(), afterMoves=after.moves();
        const auto beforeUps=before.ppUps(), afterUps=after.ppUps();
        const auto beforePP=before.pp(), afterPP=after.pp();
        for(size_t i=0;i<4;++i)
            if(beforeMoves[i]!=afterMoves[i] &&
               !apply(Field::Move,i,afterMoves[i]))return false;
        for(size_t i=0;i<4;++i)
            if(beforeUps[i]!=afterUps[i] &&
               !apply(Field::MovePPUps,i,afterUps[i]))return false;
        for(size_t i=0;i<4;++i)
            if(beforePP[i]!=afterPP[i] &&
               !apply(Field::MovePP,i,afterPP[i]))return false;

        const auto result=readSlot(tentative,location_);
        if(!result || !sameBytes(result->originalEncryptedBytes(),working_->stagedBytes())) {
            error="Gen V Keep changed unrelated PK5 fields or failed deterministic round-trip";
            return false;
        }
        workspace=std::move(tentative);
        close();
        error.clear();
        return true;
    }

    [[nodiscard]] bool back() noexcept {
        using Guard=PokeBank::UIModel::PokemonEditorExitGuard::SessionKind;
        const Guard kind=mode_==Mode::Edit?Guard::Edit:Guard::View;
        if(PokeBank::UIModel::PokemonEditorExitGuard::requiresConfirmation(kind,dirty())) {
            confirmExit_=true;
            return false;
        }
        close();
        return true;
    }
    void continueEditing() noexcept { confirmExit_=false; }
    void discardDraft() noexcept { close(); }

private:
    [[nodiscard]] static std::optional<Pokemon5ReadOnly> readSlot(
        const Gen5StagedPokemonWorkspace& ws,const Slot& location) {
        if(!Gen5StagedPokemonWorkspace::canonicalSlot(location))return std::nullopt;
        if(location.region==Region::Party) {
            if(location.box!=0)return std::nullopt;
            return ws.viewParty(location.slot);
        }
        return ws.viewBox(location.box,location.slot);
    }
    [[nodiscard]] static bool sameBytes(std::span<const uint8_t> a,
                                        std::span<const uint8_t> b) noexcept {
        return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin());
    }
    void close() noexcept {
        mode_=Mode::None;
        confirmExit_=false;
        working_.reset();
        baseline_.clear();
    }

    Mode mode_=Mode::None;
    bool confirmExit_=false;
    Slot location_{};
    std::vector<uint8_t> baseline_;
    std::optional<StagedPokemon5Record> working_;
};

} // namespace PokeVault::Integration::Gen5
#endif
