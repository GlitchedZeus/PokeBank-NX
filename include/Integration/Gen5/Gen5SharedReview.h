#ifndef POKEBANK_GEN5_SHARED_REVIEW_H
#define POKEBANK_GEN5_SHARED_REVIEW_H

#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5BlankSlot.h"
#include "Integration/Gen5/Gen5ImportRecord.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Read-only evidence for the one shared Review Pending Changes surface.
// Independently reconstruct each staged PK5 through ONLY the explicitly authorized
// field transactions. An unsupported byte difference produces no verified
// diff; it is never silently labeled legal or applied to any source SAV5.
struct VerifiedFieldChange {
    StagedPokemon5Record::Field field = StagedPokemon5Record::Field::Nature;
    size_t stat = 0;
    uint32_t before = 0;
    uint32_t after = 0;
};
struct VerifiedSlotReview {
    Gen5StagedPokemonWorkspace::Slot location;
    std::vector<VerifiedFieldChange> fields;
    // CG74 (owner Oct 25 P3): for a created Pokemon, the values of the certified staged record
    // (after = value, before unused), read only for Review. `fields` stays the verified edit list.
    std::vector<VerifiedFieldChange> created;
};

[[nodiscard]] inline std::optional<VerifiedSlotReview> verifyStagedReview(
    const Gen5StagedPokemonWorkspace::PendingChange& change,
    std::string* error = nullptr) {
    using Field = StagedPokemon5Record::Field;
    const auto fail = [&](const char* why) -> std::optional<VerifiedSlotReview> {
        if(error)*error=why;
        return std::nullopt;
    };
    if(!Gen5StagedPokemonWorkspace::canonicalSlot(change.location))
        return fail("Gen V review slot identity is noncanonical");
    // Box replacement operations have no ordinary field replay: certify
    // byte-exact clone of an original occupied PK5, or all-zero box removal.
    // The exporter independently checks clone-source bytes against SAV5.
    if(change.kind!=Gen5StagedPokemonWorkspace::ChangeKind::FieldEdit) {
        using Kind=Gen5StagedPokemonWorkspace::ChangeKind;
        using Region=Gen5StagedPokemonWorkspace::Region;
        if(change.location.region!=Region::Box ||
           change.before.size()!=Crypto::StoredSize ||
           change.after.size()!=Crypto::StoredSize)
            return fail("Gen V box operation has invalid geometry");
        const Pokemon5ReadOnly beforeBox(std::span<const uint8_t>(change.before));
        const Pokemon5ReadOnly afterBox(std::span<const uint8_t>(change.after));
        VerifiedSlotReview result;
        result.location=change.location;
        // Only an Import carries an imported record; it never rides along with another kind.
        if(change.kind!=Kind::ImportBox && !change.importedRecord.empty())
            return fail("Gen V box operation carried an unrelated imported record");
        if(change.kind==Kind::CreateBox || change.kind==Kind::ImportBox) {
            if(change.kind==Kind::CreateBox) {
                if(!change.createRequest ||
                   !change.cloneSourceOriginal.empty() ||
                   !beforeBox.valid() || !beforeBox.empty() ||
                   !nativeBlankSlot5(change.before) ||
                   !afterBox.valid() || afterBox.empty() || afterBox.partyRecord())
                    return fail("Gen V Create needs an empty original and explicit native request");
                const auto rebuilt=createNative5(*change.createRequest);
                if(!rebuilt || rebuilt.encrypted!=change.replacementOriginal ||
                   afterBox.originVersion()!=static_cast<uint32_t>(change.createRequest->game))
                    return fail("Gen V Create certificate disagrees with the native PK5 baseline");
            } else {
                // Feature A Import: the slot was a native blank, and the baseline is exactly the
                // imported file's stored record re-encrypted, byte for byte (decrypting it gives the
                // file's plaintext back). The record is re-judged against the title here, independently
                // of the workspace that staged it.
                if(change.createRequest || !change.cloneSourceOriginal.empty() ||
                   !beforeBox.valid() || !beforeBox.empty() ||
                   !nativeBlankSlot5(change.before) ||
                   !afterBox.valid() || afterBox.empty() || afterBox.partyRecord())
                    return fail("Gen V Import needs an empty original slot and only its imported record");
                if(change.importedRecord.size()!=Crypto::StoredSize ||
                   importRecordRefusal5(change.importedRecord,change.black2White2))
                    return fail("Gen V Import record is not a valid PK5 for this title");
                if(Crypto::encryptCandidate(change.importedRecord)!=change.replacementOriginal ||
                   Crypto::decrypt(change.replacementOriginal)!=change.importedRecord)
                    return fail("Gen V Import baseline differs from the imported record");
            }
            result.fields.push_back({change.kind==Kind::ImportBox?Field::ImportBox:Field::CreateBox,
                                     0,0,afterBox.species()});
            // Created (or imported) values, one per row: moves, held item and Ball, OT IDs, met data, nature,
            // gender, ability, language; IVs / EVs / Pokerus / Friendship only when not default.
            auto& created=result.created;
            const auto moves=afterBox.moves();
            for(size_t i=0;i<moves.size();++i)
                if(moves[i])created.push_back({Field::Move,i,0,moves[i]});
            if(afterBox.heldItem())created.push_back({Field::HeldItem,0,0,afterBox.heldItem()});
            created.push_back({Field::Ball,0,0,afterBox.ball()});
            created.push_back({Field::TrainerId,0,0,afterBox.tid()});
            created.push_back({Field::SecretId,0,0,afterBox.sid()});
            created.push_back({Field::MetLocation,0,0,afterBox.metLocation()});
            created.push_back({Field::MetLevel,0,0,afterBox.metLevel()});
            created.push_back({Field::Nature,0,0,afterBox.nature()});
            created.push_back({Field::Gender,0,0,afterBox.gender()});
            if(afterBox.shiny())created.push_back({Field::Shiny,0,0,1});
            created.push_back({Field::AbilitySlot,0,0,afterBox.ability()});
            created.push_back({Field::Language,0,0,afterBox.language()});
            const auto ivs=afterBox.ivs(),evs=afterBox.evs();
            for(size_t i=0;i<6;++i)if(ivs[i])created.push_back({Field::IV,i,0,ivs[i]});
            for(size_t i=0;i<6;++i)if(evs[i])created.push_back({Field::EV,i,0,evs[i]});
            if(afterBox.pokerusMode())created.push_back({Field::Pokerus,0,0,afterBox.pokerusMode()});
            if(afterBox.friendship()!=70)created.push_back({Field::Friendship,0,0,afterBox.friendship()});
        } else if(change.kind==Kind::RemoveBox) {
            if(change.createRequest)
                return fail("Gen V Remove carried an unrelated creation request");
            if(!beforeBox.valid() || beforeBox.empty() ||
               !afterBox.valid() || !afterBox.empty() ||
               change.after!=encryptedBlankBoxSlot5() ||
               !change.cloneSourceOriginal.empty())
                return fail("Gen V removal must blank exactly one occupied box PK5");
            result.fields.push_back({Field::RemoveBox,0,beforeBox.species(),0});
        } else if(change.kind==Kind::CloneBox) {
            if(change.createRequest || !beforeBox.valid() || !beforeBox.empty() ||
               !nativeBlankSlot5(change.before) ||
               !afterBox.valid() || afterBox.empty() ||
               !Gen5StagedPokemonWorkspace::canonicalSlot(change.cloneSource) ||
               (change.cloneSource.region==Region::Box &&
                change.cloneSource.box==change.location.box &&
                change.cloneSource.slot==change.location.slot))
                return fail("Gen V clone needs an empty target and distinct source");
            const Pokemon5ReadOnly source(std::span<const uint8_t>(
                change.cloneSourceOriginal));
            if(!source.valid() || source.empty() ||
               source.partyRecord()!=(change.cloneSource.region==Region::Party))
                return fail("Gen V clone source certificate is not native PK5");
            auto d=Crypto::decrypt(change.cloneSourceOriginal);
            d.resize(Crypto::StoredSize);
            if(Crypto::encryptCandidate(d)!=change.replacementOriginal)
                return fail("Gen V clone certificate differs from exact source snapshot");
            result.fields.push_back({Field::CloneBox,0,0,afterBox.species()});
        } else return fail("Gen V review box-operation kind is unknown");
        // A newly cloned/created Pokemon can be edited through the SAME
        // shared field model. Keep its immutable origin certificate, then
        // independently replay every additional field from that baseline.
        if(change.kind==Kind::CreateBox || change.kind==Kind::CloneBox ||
           change.kind==Kind::ImportBox) {
            if(change.replacementOriginal.size()!=Crypto::StoredSize)
                return fail("Gen V box operation lacks native baseline");
            if(change.after!=change.replacementOriginal) {
                auto fieldsOnly=change;
                fieldsOnly.kind=Kind::FieldEdit;
                fieldsOnly.before=change.replacementOriginal;
                fieldsOnly.cloneSourceOriginal.clear();
                fieldsOnly.createRequest.reset();
                fieldsOnly.replacementOriginal.clear();
                fieldsOnly.importedRecord.clear();
                const auto verified=verifyStagedReview(fieldsOnly,error);
                if(!verified)
                    return fail("Gen V staged box edits failed independent field replay");
                result.fields.insert(result.fields.end(),
                    verified->fields.begin(),verified->fields.end());
            }
        } else if(!change.replacementOriginal.empty())
            return fail("Gen V Remove carried unrelated native box baseline");
        if(error)error->clear();
        return result;
    }
    if(change.createRequest || !change.cloneSourceOriginal.empty() ||
       !change.replacementOriginal.empty() || !change.importedRecord.empty())
        return fail("Gen V field edit contained unrelated Create/Clone/Import evidence");
    const Pokemon5ReadOnly before(std::span<const uint8_t>(change.before));
    const Pokemon5ReadOnly after(std::span<const uint8_t>(change.after));
    if(!before.valid() || before.empty() || !after.valid() || after.empty() ||
       before.partyRecord()!=(change.location.region==
                              Gen5StagedPokemonWorkspace::Region::Party) ||
       after.partyRecord()!=before.partyRecord()) {
        return fail("Gen V pending review contains unvalidated or wrong-format PK5");
    }
    auto candidate=StagedPokemon5Record::create(
        std::span<const uint8_t>(change.before),error,change.black2White2);
    if(!candidate)return std::nullopt;
    VerifiedSlotReview result;
    result.location=change.location;
    const auto apply=[&](Field field,size_t stat,uint32_t from,uint32_t to) {
        if(from==to)return true;
        if(!candidate->stage(field,stat,to,error))return false;
        result.fields.push_back({field,stat,from,to});
        return true;
    };
    // Language first, matching Keep: on an unnicknamed record it also derives the
    // default name in the new language (M1 D3), which is not a separate name edit.
    if(!apply(Field::Language,0,before.language(),after.language()))
        return std::nullopt;
    const auto base=candidate->current();
    const auto replayName=[&](bool nickname)->bool {
        const auto oldBytes=base.decodedBytes(),newBytes=after.decodedBytes();
        const bool flagChange=nickname &&
            ((Crypto::read32(oldBytes,0x38)^Crypto::read32(newBytes,0x38))&
             0x80000000u);
        if(!nativeNameBytesDiffer5(oldBytes,newBytes,nickname) && !flagChange)
            return true;
        const auto name=asciiName5(newBytes,nickname);
        if(!name || name->empty() ||
           !candidate->stageName(nickname,*name,error))return false;
        result.fields.push_back({nickname?Field::NicknameText:
            Field::OriginalTrainerText,0,0,1});
        return true;
    };
    if(!replayName(true) || !replayName(false))
        return fail("Gen V pending review includes invalid native text changes");
    if(!apply(Field::Species,0,before.species(),after.species()))
        return std::nullopt;
    if(!apply(Field::Nature,0,before.nature(),after.nature()) ||
       !apply(Field::Friendship,0,before.friendship(),after.friendship()) ||
       !apply(Field::HeldItem,0,before.heldItem(),after.heldItem()) ||
       !apply(Field::Ball,0,before.ball(),after.ball()) ||
       !apply(Field::MetLevel,0,before.metLevel(),after.metLevel()) ||
       !apply(Field::TrainerId,0,before.tid(),after.tid()) ||
       !apply(Field::SecretId,0,before.sid(),after.sid()) ||
       !apply(Field::OriginalTrainerGender,0,before.originalTrainerGender(),
              after.originalTrainerGender()) ||
       !apply(Field::FatefulEncounter,0,before.fatefulEncounter()?1:0,
              after.fatefulEncounter()?1:0) ||
       !apply(Field::Pokerus,0,before.pokerusMode(),after.pokerusMode()) ||
       !apply(Field::MetLocation,0,before.metLocation(),after.metLocation()))
        return std::nullopt;
    // EXP and Level form a single native transaction. Use Level only for
    // exact growth-curve thresholds, otherwise replay raw bounded EXP.
    if(before.experience()!=after.experience()) {
        const auto personal=personal5(after.species(),after.form(),change.black2White2);
        if(!personal)return fail("Gen V review lacks exact EXP growth data");
        const uint8_t level=after.partyRecord()?after.partyLevel():
            levelForExperience5(personal.info->growthRate,after.experience());
        const bool exact=level>=1 && level<=100 &&
            after.experience()==experienceForLevel5(personal.info->growthRate,level);
        if(!apply(exact?Field::Level:Field::Experience,0,
                  exact?(before.partyRecord()?before.partyLevel():
                    levelForExperience5(personal.info->growthRate,before.experience()))
                       :before.experience(),
                  exact?level:after.experience()))
            return std::nullopt;
    }
    if(!apply(Field::Form,0,before.form(),after.form()))
        return std::nullopt;
    const auto slot5=[](const Pokemon5ReadOnly& p)->uint32_t {
        return p.hiddenAbility()?2u:((p.pid()>>16)&1u);
    };
    if(before.ability()!=after.ability() ||
       before.hiddenAbility()!=after.hiddenAbility() ||
       slot5(before)!=slot5(after)) {
        if(!apply(Field::AbilitySlot,0,slot5(before),slot5(after)))
            return std::nullopt;
    }
    if(!apply(Field::Gender,0,before.gender(),after.gender()))
        return std::nullopt;
    // Coordinated Gender / Shiny / Ability / ID edits end in one exact PID.
    if(!apply(Field::Personality,0,before.pid(),after.pid()))
        return std::nullopt;
    const auto bi=before.ivs(),ai=after.ivs();
    for(size_t i=0;i<6;++i)
        if(!apply(Field::IV,i,bi[i],ai[i]))return std::nullopt;
    const auto be=before.evs(),ae=after.evs();
    // Match the atomic Keep order so a fully legal final EV distribution is
    // not falsely rejected by an intermediate value above the 510 cap.
    for(size_t i=0;i<6;++i)
        if(ae[i]<be[i] && !apply(Field::EV,i,be[i],ae[i]))
            return std::nullopt;
    for(size_t i=0;i<6;++i)
        if(ae[i]>be[i] && !apply(Field::EV,i,be[i],ae[i]))
            return std::nullopt;
    const auto beforeMoves=before.moves(), afterMoves=after.moves();
    const auto beforeUps=before.ppUps(), afterUps=after.ppUps();
    const auto beforePP=before.pp(), afterPP=after.pp();
    // Deterministic native coupling order must mirror Session::keep().
    for(size_t i=0;i<4;++i)
        if(!apply(Field::Move,i,beforeMoves[i],afterMoves[i]))
            return std::nullopt;
    for(size_t i=0;i<4;++i)
        if(!apply(Field::MovePPUps,i,beforeUps[i],afterUps[i]))
            return std::nullopt;
    for(size_t i=0;i<4;++i)
        if(!apply(Field::MovePP,i,beforePP[i],afterPP[i]))
            return std::nullopt;
    if(result.fields.empty())
        return fail("Gen V pending review contains no verified field changes");
    const auto replay=candidate->stagedBytes();
    if(replay.size()!=change.after.size() ||
       !std::equal(replay.begin(),replay.end(),change.after.begin()))
        return fail("Gen V pending review includes unrelated PK5 byte changes");
    if(error)error->clear();
    return result;
}

} // namespace PokeVault::Integration::Gen5
#endif
