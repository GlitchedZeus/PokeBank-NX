#ifndef POKEBANK_GEN5_SHARED_REVIEW_H
#define POKEBANK_GEN5_SHARED_REVIEW_H

#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"

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
    const auto replayName=[&](bool nickname)->bool {
        const auto oldBytes=before.decodedBytes(),newBytes=after.decodedBytes();
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
